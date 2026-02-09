/*
 * armlib - ARM Object Library Format (ALF) librarian
 *
 * A drop-in replacement for the ARM SDT armlib tool. Creates and
 * maintains ALF libraries containing AOF object files.
 *
 * Usage: armlib options library [file_list | member_list]
 *
 * References:
 *   - ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
 *   - ARM Library Format (ALF) Specification
 *   - ARM Object Format (AOF) Specification
 */

#define _DEFAULT_SOURCE  /* for strdup() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#define PATH_SEP '\\'
#else
#include <sys/types.h>
#define PATH_SEP '/'
#endif

/* ===== Chunk File Format Constants ===== */

#define CHUNK_FILE_ID            0xC3CBC6C5
#define CHUNK_FILE_ID_REVERSED   0xC5C6CBC3

/* ===== ALF Constants ===== */

#define ALF_VERSION              1

/* ===== AOF Constants (for symbol extraction) ===== */

#define AOF_FILE_TYPE            0xC5E2D080
#define AOF_SYM_DEFINED          0x00000001
#define AOF_SYM_GLOBAL           0x00000002

/* ===== Limits ===== */

#define MAX_MEMBERS              4096
#define MAX_SYMBOLS              65536
#define MAX_FILES                4096
#define MAX_LINE                 4096

/* ===== RISC OS Timestamp ===== */

/* Seconds between 1900-01-01 and 1970-01-01 (Unix epoch) */
#define RISCOS_UNIX_EPOCH_DIFF   2208988800ULL

/* ===== Endianness ===== */

static int g_big_endian = 1;  /* default: big-endian (3DO target) */

static uint32_t to_target(uint32_t v)
{
	/* We always work in host order internally, convert for file output */
	if (g_big_endian) {
		/* Write as big-endian */
		uint8_t b[4];
		b[0] = (v >> 24) & 0xFF;
		b[1] = (v >> 16) & 0xFF;
		b[2] = (v >>  8) & 0xFF;
		b[3] =  v        & 0xFF;
		memcpy(&v, b, 4);
		return v;
	}
	/* Little-endian: native on x86 */
	return v;
}

static uint32_t from_target(uint32_t v)
{
	if (g_big_endian) {
		uint8_t *b = (uint8_t *)&v;
		return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) |
		       ((uint32_t)b[2] << 8)  |  (uint32_t)b[3];
	}
	return v;
}

static uint32_t read_target32(const uint8_t *p)
{
	uint32_t v;
	memcpy(&v, p, 4);
	return from_target(v);
}

/* ===== Utility: word-align ===== */

static uint32_t align4(uint32_t v)
{
	return (v + 3) & ~3u;
}

/* ===== Timestamp Encoding ===== */

static void encode_timestamp(time_t t, uint32_t *w0, uint32_t *w1)
{
	uint64_t secs_since_1900 = (uint64_t)t + RISCOS_UNIX_EPOCH_DIFF;
	uint64_t centisecs = secs_since_1900 * 100;

	*w0 = (uint32_t)(centisecs >> 16);
	*w1 = (uint32_t)((centisecs & 0xFFFF) << 16); /* microsecs = 0 */
}

static time_t decode_timestamp(uint32_t w0, uint32_t w1)
{
	uint64_t centisecs = ((uint64_t)w0 << 16) | ((w1 >> 16) & 0xFFFF);
	uint64_t secs_since_1900 = centisecs / 100;
	time_t result;

	if (secs_since_1900 < RISCOS_UNIX_EPOCH_DIFF)
		return 0;
	result = (time_t)(secs_since_1900 - RISCOS_UNIX_EPOCH_DIFF);
	/* Clamp to 32-bit range for compatibility with original 32-bit armlib */
	if (result < 0 || result > (time_t)0x7FFFFFFF)
		return 0;
	return result;
}

static void format_timestamp(time_t t, char *buf, size_t bufsz)
{
	struct tm *tm = localtime(&t);
	if (tm)
		strftime(buf, bufsz, "%a %b %e %H:%M:%S %Y", tm);
	else
		snprintf(buf, bufsz, "(unknown)");
}

/* ===== Path Handling ===== */

static const char *basename_only(const char *path)
{
	const char *p;

	p = strrchr(path, '/');
	if (p) path = p + 1;
	p = strrchr(path, '\\');
	if (p) path = p + 1;
	/* Also strip drive letters (e.g., "C:") */
	p = strrchr(path, ':');
	if (p) path = p + 1;
	return path;
}

static const char *strip_drive(const char *path)
{
	const char *p = strrchr(path, ':');
	if (p) return p + 1;
	return path;
}

/* ===== Wildcard Matching ===== */

static int wildcard_match(const char *pattern, const char *str)
{
	while (*pattern) {
		if (*pattern == '*') {
			pattern++;
			if (*pattern == '\0')
				return 1;
			while (*str) {
				if (wildcard_match(pattern, str))
					return 1;
				str++;
			}
			return 0;
		} else if (*pattern == '?') {
			if (*str == '\0')
				return 0;
			pattern++;
			str++;
		} else {
			if (*pattern != *str)
				return 0;
			pattern++;
			str++;
		}
	}
	return *str == '\0';
}

/* ===== Library Member ===== */

typedef struct {
	char    *name;          /* member name (allocated) */
	uint8_t *data;          /* member data (allocated, or NULL if deleted) */
	uint32_t data_size;     /* size of data */
	time_t   timestamp;     /* file modification time */
} LibMember;

/* ===== Exported Symbol ===== */

typedef struct {
	char    *name;          /* symbol name (allocated) */
	int      member_idx;    /* index into members array */
} LibSymbol;

/* ===== Library State ===== */

typedef struct {
	LibMember  members[MAX_MEMBERS];
	int        num_members;

	LibSymbol  symbols[MAX_SYMBOLS];
	int        num_symbols;

	int        has_symtab;  /* whether library had OFL_SYMT */
} Library;

/* ===== Read a File into Memory ===== */

static uint8_t *read_file(const char *path, uint32_t *out_size)
{
	FILE *f;
	long sz;
	uint8_t *data;

	f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "AOF Librarian: (Error)   Can't find file %s.\n", path);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (sz < 0) {
		fclose(f);
		fprintf(stderr, "AOF Librarian: (Error)   Can't read file %s.\n", path);
		return NULL;
	}

	data = malloc((size_t)sz);
	if (!data) {
		fclose(f);
		fprintf(stderr, "AOF Librarian: (Error)   Out of memory reading %s.\n", path);
		return NULL;
	}

	if (sz > 0 && fread(data, 1, (size_t)sz, f) != (size_t)sz) {
		free(data);
		fclose(f);
		fprintf(stderr, "AOF Librarian: (Error)   Can't read file %s.\n", path);
		return NULL;
	}

	fclose(f);
	*out_size = (uint32_t)sz;
	return data;
}

/* ===== Get File Modification Time ===== */

static time_t get_file_mtime(const char *path)
{
	struct stat st;
	if (stat(path, &st) == 0)
		return st.st_mtime;
	return time(NULL);
}

/* ===== Detect Endianness from AOF/ALF Data ===== */

static int detect_endianness(const uint8_t *data, uint32_t size)
{
	uint32_t magic;

	if (size < 4)
		return -1;

	memcpy(&magic, data, 4);

	if (magic == CHUNK_FILE_ID)
		return 0; /* native (little-endian on x86) */
	if (magic == CHUNK_FILE_ID_REVERSED)
		return 1; /* reversed */

	return -1;
}

/* ===== Build Symbol Table from AOF Member Data ===== */

/*
 * Extract exported (defined + global) symbols from an AOF member.
 * Returns number of symbols found, fills sym_names array.
 */
static int extract_aof_symbols(const uint8_t *data, uint32_t size,
                               char **sym_names, int max_syms)
{
	uint32_t magic;
	int is_be;
	uint32_t max_chunks, i;
	const uint8_t *obj_symt = NULL, *obj_strt = NULL;
	uint32_t obj_symt_size = 0, obj_strt_size = 0;
	const uint8_t *obj_head = NULL;
	uint32_t obj_head_size = 0;
	uint32_t num_symbols, j;
	int count = 0;

	if (size < 12)
		return 0;

	memcpy(&magic, data, 4);
	if (magic == CHUNK_FILE_ID)
		is_be = 0;
	else if (magic == CHUNK_FILE_ID_REVERSED)
		is_be = 1;
	else
		return 0;

	/* Temporarily override global for reading this member */
	{
		int saved = g_big_endian;
		uint32_t dir_end;

		g_big_endian = is_be;
		max_chunks = read_target32(data + 4);
		dir_end = 12 + max_chunks * 16;

		if (dir_end > size) {
			g_big_endian = saved;
			return 0;
		}

		for (i = 0; i < max_chunks; i++) {
			const uint8_t *p = data + 12 + i * 16;
			char cid[8];
			uint32_t c_off, c_sz;

			memcpy(cid, p, 8);
			c_off = read_target32(p + 8);
			c_sz  = read_target32(p + 12);

			if (c_off == 0 || c_off + c_sz > size)
				continue;

			if (memcmp(cid, "OBJ_HEAD", 8) == 0) {
				obj_head = data + c_off;
				obj_head_size = c_sz;
			} else if (memcmp(cid, "OBJ_SYMT", 8) == 0) {
				obj_symt = data + c_off;
				obj_symt_size = c_sz;
			} else if (memcmp(cid, "OBJ_STRT", 8) == 0) {
				obj_strt = data + c_off;
				obj_strt_size = c_sz;
			}
		}

		if (!obj_head || obj_head_size < 24 || !obj_symt || !obj_strt) {
			g_big_endian = saved;
			return 0;
		}

		/* Verify it's AOF */
		if (read_target32(obj_head) != AOF_FILE_TYPE) {
			g_big_endian = saved;
			return 0;
		}

		num_symbols = read_target32(obj_head + 12);
		if (num_symbols * 16 > obj_symt_size)
			num_symbols = obj_symt_size / 16;

		for (j = 0; j < num_symbols && count < max_syms; j++) {
			const uint8_t *sp = obj_symt + j * 16;
			uint32_t s_name = read_target32(sp + 0);
			uint32_t s_attr = read_target32(sp + 4);

			if ((s_attr & AOF_SYM_DEFINED) && (s_attr & AOF_SYM_GLOBAL)) {
				if (s_name >= 4 && s_name < obj_strt_size) {
					const char *name = (const char *)(obj_strt + s_name);
					sym_names[count++] = strdup(name);
				}
			}
		}

		g_big_endian = saved;
	}

	return count;
}

/* ===== Read Existing Library ===== */

static int read_library(const char *path, Library *lib)
{
	uint8_t *data;
	uint32_t file_size;
	uint32_t magic;
	int is_be;
	uint32_t max_chunks, i;
	uint32_t dir_end;

	/* Located chunks */
	const uint8_t *lib_diry = NULL;  uint32_t lib_diry_size = 0;
	const uint8_t *ofl_symt = NULL;  uint32_t ofl_symt_size = 0;

	memset(lib, 0, sizeof(*lib));

	data = read_file(path, &file_size);
	if (!data)
		return -1;

	if (file_size < 12) {
		fprintf(stderr, "AOF Librarian: (Error)   %s is not a valid library.\n", path);
		free(data);
		return -1;
	}

	memcpy(&magic, data, 4);
	if (magic == CHUNK_FILE_ID)
		is_be = 0;
	else if (magic == CHUNK_FILE_ID_REVERSED)
		is_be = 1;
	else {
		fprintf(stderr, "AOF Librarian: (Error)   %s is not a valid library.\n", path);
		free(data);
		return -1;
	}

	g_big_endian = is_be;

	max_chunks = read_target32(data + 4);
	dir_end = 12 + max_chunks * 16;

	if (dir_end > file_size) {
		fprintf(stderr, "AOF Librarian: (Error)   %s is not a valid library.\n", path);
		free(data);
		return -1;
	}

	/* Scan chunk directory for LIB_DIRY and other chunks */
	for (i = 0; i < max_chunks; i++) {
		const uint8_t *p = data + 12 + i * 16;
		char cid[8];
		uint32_t c_off, c_sz;

		memcpy(cid, p, 8);
		c_off = read_target32(p + 8);
		c_sz  = read_target32(p + 12);

		if (c_off == 0)
			continue;
		if (c_off + c_sz > file_size)
			continue;

		if (memcmp(cid, "LIB_DIRY", 8) == 0) {
			lib_diry = data + c_off;
			lib_diry_size = c_sz;
		} else if (memcmp(cid, "OFL_SYMT", 8) == 0) {
			ofl_symt = data + c_off;
			ofl_symt_size = c_sz;
			lib->has_symtab = 1;
		}
	}

	if (!lib_diry) {
		fprintf(stderr, "AOF Librarian: (Error)   %s has no directory.\n", path);
		free(data);
		return -1;
	}

	/* Parse directory entries to extract members */
	{
		uint32_t offset = 0;
		while (offset + 12 <= lib_diry_size && lib->num_members < MAX_MEMBERS) {
			const uint8_t *ep = lib_diry + offset;
			uint32_t chunk_index  = read_target32(ep + 0);
			uint32_t entry_length = read_target32(ep + 4);
			uint32_t data_length  = read_target32(ep + 8);

			if (entry_length < 12 || offset + entry_length > lib_diry_size)
				break;

			if (chunk_index != 0 && chunk_index < max_chunks) {
				const uint8_t *cp = data + 12 + chunk_index * 16;
				uint32_t c_off = read_target32(cp + 8);
				uint32_t c_sz  = read_target32(cp + 12);
				const char *name;
				LibMember *m;

				if (c_off == 0 || c_off + c_sz > file_size) {
					offset += entry_length;
					continue;
				}

				name = (data_length > 0) ? (const char *)(ep + 12) : "unnamed";

				m = &lib->members[lib->num_members];
				m->name = strdup(name);
				m->data_size = c_sz;
				m->data = malloc(c_sz);
				if (m->data)
					memcpy(m->data, data + c_off, c_sz);

				/* Decode timestamp from directory entry */
				if (data_length >= 8) {
					uint32_t ts_off = 12 + data_length - 8;
					if (ts_off + 8 <= entry_length &&
					    offset + ts_off + 8 <= lib_diry_size) {
						uint32_t tw0 = read_target32(ep + ts_off);
						uint32_t tw1 = read_target32(ep + ts_off + 4);
						m->timestamp = decode_timestamp(tw0, tw1);
					}
				}

				lib->num_members++;
			}

			offset += entry_length;
		}
	}

	(void)ofl_symt;
	(void)ofl_symt_size;

	free(data);
	return 0;
}

/* ===== Build Symbol Table for Library ===== */

static void build_symbol_table(Library *lib)
{
	int i;
	char *sym_names[MAX_SYMBOLS];

	lib->num_symbols = 0;

	for (i = 0; i < lib->num_members; i++) {
		LibMember *m = &lib->members[i];
		int nsyms, j;

		if (!m->data || m->data_size == 0)
			continue;

		nsyms = extract_aof_symbols(m->data, m->data_size,
		                            sym_names, MAX_SYMBOLS - lib->num_symbols);

		for (j = 0; j < nsyms && lib->num_symbols < MAX_SYMBOLS; j++) {
			lib->symbols[lib->num_symbols].name = sym_names[j];
			lib->symbols[lib->num_symbols].member_idx = i;
			lib->num_symbols++;
		}
	}
}

/* ===== Write Library File ===== */

static int write_library(const char *path, Library *lib, int add_symtab)
{
	FILE *f;
	uint32_t num_members = (uint32_t)lib->num_members;
	uint32_t max_chunks;
	uint32_t header_size;
	uint32_t offset;
	uint32_t i;
	time_t now = time(NULL);
	uint32_t ts_w0, ts_w1;

	/* Chunk layout:
	 *  0: LIB_TIME
	 *  1: LIB_VRSN
	 *  2: LIB_DIRY
	 *  3..3+N-1: LIB_DATA (one per member)
	 *  3+N: OFL_TIME  (if add_symtab)
	 *  3+N+1: OFL_SYMT (if add_symtab)
	 */
	if (add_symtab)
		max_chunks = 3 + num_members + 2;
	else
		max_chunks = 3 + num_members;

	header_size = 12 + max_chunks * 16;

	/* Build symbol table if needed */
	if (add_symtab)
		build_symbol_table(lib);

	/* Compute directory size */
	uint32_t diry_size = 0;
	for (i = 0; i < num_members; i++) {
		uint32_t name_len = (uint32_t)strlen(lib->members[i].name) + 1;
		uint32_t data_len = align4(name_len) + 8; /* name + pad + timestamp */
		uint32_t entry_len = 12 + data_len;
		diry_size += entry_len;
	}

	/* Compute OFL_SYMT size */
	uint32_t symt_size = 0;
	if (add_symtab) {
		for (i = 0; i < (uint32_t)lib->num_symbols; i++) {
			uint32_t name_len = (uint32_t)strlen(lib->symbols[i].name) + 1;
			uint32_t data_len = align4(name_len);
			uint32_t entry_len = 12 + data_len;
			symt_size += entry_len;
		}
	}

	/* Compute file offsets */
	offset = header_size;

	/* Chunk 0: LIB_TIME at offset */
	uint32_t lib_time_offset = offset;
	offset += 8;

	/* Chunk 1: LIB_VRSN */
	uint32_t lib_vrsn_offset = offset;
	offset += 4;

	/* Chunk 2: LIB_DIRY */
	uint32_t lib_diry_offset = offset;
	offset += align4(diry_size);

	/* Chunks 3..3+N-1: LIB_DATA */
	uint32_t *data_offsets = calloc(num_members, sizeof(uint32_t));
	if (!data_offsets && num_members > 0) {
		fprintf(stderr, "AOF Librarian: (Error)   Out of memory.\n");
		return -1;
	}
	for (i = 0; i < num_members; i++) {
		data_offsets[i] = offset;
		offset += align4(lib->members[i].data_size);
	}

	/* OFL_TIME and OFL_SYMT */
	uint32_t ofl_time_offset = 0, ofl_symt_offset = 0;
	if (add_symtab) {
		ofl_time_offset = offset;
		offset += 8;
		ofl_symt_offset = offset;
		offset += align4(symt_size);
	}

	/* Encode current timestamp */
	encode_timestamp(now, &ts_w0, &ts_w1);

	/* Open output */
	f = fopen(path, "wb");
	if (!f) {
		fprintf(stderr, "AOF Librarian: (Error)   Can't create %s.\n", path);
		free(data_offsets);
		return -1;
	}

	/* Write chunk file header (12 bytes) */
	{
		uint32_t w;
		w = to_target(CHUNK_FILE_ID);
		fwrite(&w, 4, 1, f);
		w = to_target(max_chunks);
		fwrite(&w, 4, 1, f);
		w = to_target(max_chunks); /* numChunks = maxChunks */
		fwrite(&w, 4, 1, f);
	}

	/* Write chunk directory entries */
	for (i = 0; i < max_chunks; i++) {
		char cid[8];
		uint32_t c_off, c_sz;

		memset(cid, 0, 8);

		if (i == 0) {
			memcpy(cid, "LIB_TIME", 8);
			c_off = lib_time_offset;
			c_sz = 8;
		} else if (i == 1) {
			memcpy(cid, "LIB_VRSN", 8);
			c_off = lib_vrsn_offset;
			c_sz = 4;
		} else if (i == 2) {
			memcpy(cid, "LIB_DIRY", 8);
			c_off = lib_diry_offset;
			c_sz = diry_size;
		} else if (i >= 3 && i < 3 + num_members) {
			memcpy(cid, "LIB_DATA", 8);
			c_off = data_offsets[i - 3];
			c_sz = lib->members[i - 3].data_size;
		} else if (add_symtab && i == 3 + num_members) {
			memcpy(cid, "OFL_TIME", 8);
			c_off = ofl_time_offset;
			c_sz = 8;
		} else if (add_symtab && i == 3 + num_members + 1) {
			memcpy(cid, "OFL_SYMT", 8);
			c_off = ofl_symt_offset;
			c_sz = symt_size;
		} else {
			/* unused slot */
			c_off = 0;
			c_sz = 0;
		}

		fwrite(cid, 1, 8, f);
		{
			uint32_t w;
			w = to_target(c_off);
			fwrite(&w, 4, 1, f);
			w = to_target(c_sz);
			fwrite(&w, 4, 1, f);
		}
	}

	/* Write LIB_TIME */
	{
		uint32_t w;
		w = to_target(ts_w0);
		fwrite(&w, 4, 1, f);
		w = to_target(ts_w1);
		fwrite(&w, 4, 1, f);
	}

	/* Write LIB_VRSN */
	{
		uint32_t w = to_target(ALF_VERSION);
		fwrite(&w, 4, 1, f);
	}

	/* Write LIB_DIRY */
	for (i = 0; i < num_members; i++) {
		LibMember *m = &lib->members[i];
		uint32_t name_len = (uint32_t)strlen(m->name) + 1;
		uint32_t padded_name = align4(name_len);
		uint32_t data_length = padded_name + 8; /* name+pad + timestamp */
		uint32_t entry_length = 12 + data_length;
		uint32_t chunk_index = 3 + i;
		uint32_t m_ts_w0, m_ts_w1;
		uint32_t w;
		uint8_t pad[4] = {0, 0, 0, 0};

		w = to_target(chunk_index);
		fwrite(&w, 4, 1, f);
		w = to_target(entry_length);
		fwrite(&w, 4, 1, f);
		w = to_target(data_length);
		fwrite(&w, 4, 1, f);

		/* Write member name + padding */
		fwrite(m->name, 1, name_len, f);
		if (padded_name > name_len)
			fwrite(pad, 1, padded_name - name_len, f);

		/* Write member timestamp */
		encode_timestamp(m->timestamp, &m_ts_w0, &m_ts_w1);
		w = to_target(m_ts_w0);
		fwrite(&w, 4, 1, f);
		w = to_target(m_ts_w1);
		fwrite(&w, 4, 1, f);
	}

	/* Pad directory to word alignment */
	{
		uint32_t diry_written = 0;
		for (i = 0; i < num_members; i++) {
			uint32_t name_len = (uint32_t)strlen(lib->members[i].name) + 1;
			uint32_t padded_name = align4(name_len);
			diry_written += 12 + padded_name + 8;
		}
		uint32_t pad_needed = align4(diry_written) - diry_written;
		if (pad_needed > 0) {
			uint8_t pad[4] = {0, 0, 0, 0};
			fwrite(pad, 1, pad_needed, f);
		}
	}

	/* Write LIB_DATA chunks */
	for (i = 0; i < num_members; i++) {
		LibMember *m = &lib->members[i];
		if (m->data && m->data_size > 0)
			fwrite(m->data, 1, m->data_size, f);
		/* Pad to word alignment */
		{
			uint32_t pad_needed = align4(m->data_size) - m->data_size;
			if (pad_needed > 0) {
				uint8_t pad[4] = {0, 0, 0, 0};
				fwrite(pad, 1, pad_needed, f);
			}
		}
	}

	/* Write OFL_TIME and OFL_SYMT */
	if (add_symtab) {
		/* OFL_TIME */
		{
			uint32_t w;
			w = to_target(ts_w0);
			fwrite(&w, 4, 1, f);
			w = to_target(ts_w1);
			fwrite(&w, 4, 1, f);
		}

		/* OFL_SYMT */
		for (i = 0; i < (uint32_t)lib->num_symbols; i++) {
			LibSymbol *s = &lib->symbols[i];
			uint32_t name_len = (uint32_t)strlen(s->name) + 1;
			uint32_t data_length = align4(name_len);
			uint32_t entry_length = 12 + data_length;
			uint32_t chunk_index = 3 + (uint32_t)s->member_idx;
			uint32_t w;
			uint8_t pad[4] = {0, 0, 0, 0};

			w = to_target(chunk_index);
			fwrite(&w, 4, 1, f);
			w = to_target(entry_length);
			fwrite(&w, 4, 1, f);
			w = to_target(data_length);
			fwrite(&w, 4, 1, f);

			fwrite(s->name, 1, name_len, f);
			if (data_length > name_len)
				fwrite(pad, 1, data_length - name_len, f);
		}

		/* Pad OFL_SYMT to word alignment */
		{
			uint32_t symt_written = 0;
			for (i = 0; i < (uint32_t)lib->num_symbols; i++) {
				uint32_t name_len = (uint32_t)strlen(lib->symbols[i].name) + 1;
				symt_written += 12 + align4(name_len);
			}
			uint32_t pad_needed = align4(symt_written) - symt_written;
			if (pad_needed > 0) {
				uint8_t pad[4] = {0, 0, 0, 0};
				fwrite(pad, 1, pad_needed, f);
			}
		}
	}

	fclose(f);
	free(data_offsets);

	return 0;
}

/* ===== List Library Contents ===== */

static void list_library(Library *lib, const char *path)
{
	int i;
	uint32_t magic;
	uint32_t ver = ALF_VERSION;
	time_t lib_time = time(NULL);
	char tbuf[128];

	/* Try to read version and timestamp from the file itself */
	{
		uint8_t *data;
		uint32_t fsize;

		data = read_file(path, &fsize);
		if (data && fsize >= 12) {
			int is_be;
			uint32_t mc, j;
			int saved = g_big_endian;

			memcpy(&magic, data, 4);
			is_be = (magic == CHUNK_FILE_ID) ? 0 : 1;
			g_big_endian = is_be;

			mc = read_target32(data + 4);
			if (12 + mc * 16 <= fsize) {
				for (j = 0; j < mc; j++) {
					const uint8_t *p = data + 12 + j * 16;
					char cid[8];
					uint32_t c_off, c_sz;

					memcpy(cid, p, 8);
					c_off = read_target32(p + 8);
					c_sz  = read_target32(p + 12);

					if (c_off == 0 || c_off + c_sz > fsize)
						continue;

					if (memcmp(cid, "LIB_VRSN", 8) == 0 && c_sz >= 4)
						ver = read_target32(data + c_off);
					else if (memcmp(cid, "LIB_TIME", 8) == 0 && c_sz >= 8) {
						uint32_t w0 = read_target32(data + c_off);
						uint32_t w1 = read_target32(data + c_off + 4);
						lib_time = decode_timestamp(w0, w1);
					}
				}
			}
			g_big_endian = saved;
			free(data);
		}
	}

	format_timestamp(lib_time, tbuf, sizeof(tbuf));
	printf("\nFormat version: %u\n", ver);
	printf("Last Modification: %s\n", tbuf);
	printf("\nContents:\n\n");

	for (i = 0; i < lib->num_members; i++) {
		LibMember *m = &lib->members[i];
		char ts[128];

		format_timestamp(m->timestamp, ts, sizeof(ts));
		printf("  %-46s%5u   %s\n", m->name, m->data_size, ts);
	}

	printf("\nEnd of Library\n");
}

/* ===== List Symbol Table ===== */

static void list_symtab(Library *lib, const char *path)
{
	int i;
	time_t ofl_time = time(NULL);
	char tbuf[128];

	/* Try to read OFL_TIME from file */
	{
		uint8_t *data;
		uint32_t fsize;

		data = read_file(path, &fsize);
		if (data && fsize >= 12) {
			uint32_t magic_raw;
			int is_be;
			uint32_t mc, j;
			int saved = g_big_endian;

			memcpy(&magic_raw, data, 4);
			is_be = (magic_raw == CHUNK_FILE_ID) ? 0 : 1;
			g_big_endian = is_be;

			mc = read_target32(data + 4);
			if (12 + mc * 16 <= fsize) {
				for (j = 0; j < mc; j++) {
					const uint8_t *p = data + 12 + j * 16;
					char cid[8];
					uint32_t c_off, c_sz;

					memcpy(cid, p, 8);
					c_off = read_target32(p + 8);
					c_sz  = read_target32(p + 12);

					if (c_off == 0 || c_off + c_sz > fsize)
						continue;

					if (memcmp(cid, "OFL_TIME", 8) == 0 && c_sz >= 8) {
						uint32_t w0 = read_target32(data + c_off);
						uint32_t w1 = read_target32(data + c_off + 4);
						ofl_time = decode_timestamp(w0, w1);
					}
				}
			}
			g_big_endian = saved;
			free(data);
		}
	}

	/* Build symbol table from members */
	build_symbol_table(lib);

	format_timestamp(ofl_time, tbuf, sizeof(tbuf));
	printf("External Symbol Table, generated: %s\n\n", tbuf);

	for (i = 0; i < lib->num_symbols; i++) {
		LibSymbol *s = &lib->symbols[i];
		const char *member_name = "";

		if (s->member_idx >= 0 && s->member_idx < lib->num_members)
			member_name = lib->members[s->member_idx].name;

		printf("  %-40s from   %s\n", s->name, member_name);
	}

	printf("\nEnd of Table\n");
}

/* ===== Free Library ===== */

static void free_library(Library *lib)
{
	int i;

	for (i = 0; i < lib->num_members; i++) {
		free(lib->members[i].name);
		free(lib->members[i].data);
	}
	for (i = 0; i < lib->num_symbols; i++) {
		free(lib->symbols[i].name);
	}
	memset(lib, 0, sizeof(*lib));
}

/* ===== Read Via File ===== */

static int read_via_file(const char *via_path, char **args, int *nargs, int max_args)
{
	FILE *f;
	char line[MAX_LINE];

	f = fopen(via_path, "r");
	if (!f) {
		fprintf(stderr, "AOF Librarian: (Error)   Can't open via file %s.\n", via_path);
		return -1;
	}

	while (fgets(line, sizeof(line), f)) {
		char *p = line;
		char *token;

		/* Strip trailing whitespace */
		{
			size_t len = strlen(p);
			while (len > 0 && (p[len-1] == '\n' || p[len-1] == '\r' ||
			       p[len-1] == ' ' || p[len-1] == '\t'))
				p[--len] = '\0';
		}

		/* Skip empty lines and comments */
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == '\0' || *p == ';' || *p == '#')
			continue;

		/* Tokenize by whitespace */
		token = strtok(p, " \t");
		while (token && *nargs < max_args) {
			args[*nargs] = strdup(token);
			(*nargs)++;
			token = strtok(NULL, " \t");
		}
	}

	fclose(f);
	return 0;
}

/* ===== Usage ===== */

static void print_usage(void)
{
	printf("AOF Librarian (armlib replacement)\n"
	       "       - AOF library creation and maintenance tool\n"
	       "\n"
	       "Command format:\n"
	       "\n"
	       "armlib options library [ file_list | member_list ]\n"
	       "\n"
	       "Wildcards '?' and '*' may be used in <member_list>\n"
	       "\n"
	       "Options:-\n"
	       "\n"
	       "-c      Create a new library containing files in <file_list>.\n"
	       "-i      Insert files in <file_list>, replace existing members of the same name.\n"
	       "-d      Delete the members in <member_list>.\n"
	       "-e      Extract members in <member_list> placing in files of the same name.\n"
	       "-o      Add an external symbol table to an object library (DEFAULT).\n"
	       "-n      Do not add an external symbol table to an object library.\n"
	       "-p      Respect paths of files and objects.\n"
	       "-l      List library, may be specified with any other option.\n"
	       "-s      List symbol table, may be specified with any other option.\n"
	       "-t dir  Extract files to <dir> directory.\n"
	       "-v file Take additional arguments from via file.\n"
	       "\n"
	       "Examples:-\n"
	       "\n"
	       "        armlib -c mylib obj1 obj2 obj3...\n"
	       "        armlib -e mylib ?sort*\n"
	       "        armlib -d mylib hash.o\n"
	       "        armlib -i mylib quick_sort.o quick_hash1.o\n"
	       "        armlib -l -s ansilib\n"
	       "\n");
}

/* ===== Main ===== */

int main(int argc, char *argv[])
{
	int op_create = 0, op_insert = 0, op_delete = 0, op_extract = 0;
	int opt_symtab = 1;  /* -o default on */
	int opt_paths = 0;   /* -p off by default */
	int opt_list = 0;    /* -l */
	int opt_listsym = 0; /* -s */
	const char *opt_dir = NULL;  /* -t dir */
	const char *lib_path = NULL;
	char *file_args[MAX_FILES];
	int num_files = 0;
	Library lib;
	int i;
	int ret = 0;

	/* Collect all arguments (including via file expansions) */
	char *all_args[MAX_FILES + 256];
	int num_all_args = 0;

	for (i = 1; i < argc && num_all_args < MAX_FILES + 256; i++) {
		all_args[num_all_args++] = strdup(argv[i]);
	}

	/* First pass: find -v and expand */
	{
		char *expanded[MAX_FILES + 256];
		int num_expanded = 0;

		for (i = 0; i < num_all_args; i++) {
			if (strcmp(all_args[i], "-v") == 0 && i + 1 < num_all_args) {
				read_via_file(all_args[i + 1], expanded, &num_expanded,
				              MAX_FILES + 256);
				i++; /* skip via filename */
			} else {
				expanded[num_expanded++] = strdup(all_args[i]);
			}
		}

		/* Free original, replace */
		for (i = 0; i < num_all_args; i++)
			free(all_args[i]);

		num_all_args = num_expanded;
		for (i = 0; i < num_expanded; i++)
			all_args[i] = expanded[i];
	}

	if (num_all_args == 0) {
		print_usage();
		return 1;
	}

	/* Parse options */
	i = 0;
	while (i < num_all_args && all_args[i][0] == '-') {
		const char *arg = all_args[i];

		if (strcmp(arg, "-h") == 0 || strcmp(arg, "-help") == 0 ||
		    strcmp(arg, "--help") == 0) {
			print_usage();
			ret = 0;
			goto cleanup_args;
		} else if (strcmp(arg, "-c") == 0) {
			op_create = 1;
		} else if (strcmp(arg, "-i") == 0) {
			op_insert = 1;
		} else if (strcmp(arg, "-d") == 0) {
			op_delete = 1;
		} else if (strcmp(arg, "-e") == 0) {
			op_extract = 1;
		} else if (strcmp(arg, "-o") == 0) {
			opt_symtab = 1;
		} else if (strcmp(arg, "-n") == 0) {
			opt_symtab = 0;
		} else if (strcmp(arg, "-p") == 0) {
			opt_paths = 1;
		} else if (strcmp(arg, "-l") == 0) {
			opt_list = 1;
		} else if (strcmp(arg, "-s") == 0) {
			opt_listsym = 1;
		} else if (strcmp(arg, "-t") == 0) {
			i++;
			if (i >= num_all_args) {
				fprintf(stderr, "AOF Librarian: (Error)   -t requires a directory argument.\n");
				ret = 1;
				goto cleanup_args;
			}
			opt_dir = all_args[i];
		} else if (strcmp(arg, "-v") == 0) {
			/* Already handled, but skip in case of double */
			i++;
		} else {
			/* Check for combined options like -cl, -ls etc */
			const char *p = arg + 1;
			int consumed_next = 0;
			while (*p && !consumed_next) {
				switch (*p) {
				case 'c': op_create = 1; break;
				case 'i': op_insert = 1; break;
				case 'd': op_delete = 1; break;
				case 'e': op_extract = 1; break;
				case 'o': opt_symtab = 1; break;
				case 'n': opt_symtab = 0; break;
				case 'p': opt_paths = 1; break;
				case 'l': opt_list = 1; break;
				case 's': opt_listsym = 1; break;
				case 'h':
					print_usage();
					ret = 0;
					goto cleanup_args;
				case 't':
					i++;
					if (i >= num_all_args) {
						fprintf(stderr, "AOF Librarian: (Error)   -t requires a directory argument.\n");
						ret = 1;
						goto cleanup_args;
					}
					opt_dir = all_args[i];
					consumed_next = 1;
					break;
				case 'v':
					/* Already handled */
					i++;
					consumed_next = 1;
					break;
				default:
					fprintf(stderr, "AOF Librarian: (Warning) Unknown option '-%c'.\n", *p);
					break;
				}
				p++;
			}
		}
		i++;
	}

	/* Next non-option argument is the library name */
	if (i >= num_all_args) {
		if (opt_list || opt_listsym) {
			fprintf(stderr, "AOF Librarian: (Error)   No library specified.\n");
		} else {
			print_usage();
		}
		ret = 1;
		goto cleanup_args;
	}

	lib_path = all_args[i++];

	/* Remaining arguments are file/member names */
	while (i < num_all_args && num_files < MAX_FILES) {
		file_args[num_files++] = all_args[i++];
	}

	memset(&lib, 0, sizeof(lib));

	/* ===== Operation: Create ===== */
	if (op_create) {
		/* Detect endianness from first input file */
		if (num_files > 0) {
			uint8_t *first_data;
			uint32_t first_size;

			first_data = read_file(file_args[0], &first_size);
			if (first_data) {
				int det = detect_endianness(first_data, first_size);
				if (det == 0)
					g_big_endian = 0;
				else if (det == 1)
					g_big_endian = 1;
				/* else keep default */
				free(first_data);
			}
		}

		for (i = 0; i < num_files && lib.num_members < MAX_MEMBERS; i++) {
			const char *filepath = file_args[i];
			const char *member_name;
			uint8_t *data;
			uint32_t data_size;
			LibMember *m;

			data = read_file(filepath, &data_size);
			if (!data) {
				ret = 1;
				goto cleanup;
			}

			if (opt_paths)
				member_name = strip_drive(filepath);
			else
				member_name = basename_only(filepath);

			m = &lib.members[lib.num_members];
			m->name = strdup(member_name);
			m->data = data;
			m->data_size = data_size;
			m->timestamp = get_file_mtime(filepath);
			lib.num_members++;
		}

		if (write_library(lib_path, &lib, opt_symtab) != 0) {
			ret = 1;
			goto cleanup;
		}
	}

	/* ===== Operation: Insert ===== */
	else if (op_insert) {
		if (read_library(lib_path, &lib) != 0) {
			ret = 1;
			goto cleanup;
		}

		for (i = 0; i < num_files; i++) {
			const char *filepath = file_args[i];
			const char *member_name;
			uint8_t *data;
			uint32_t data_size;
			int replaced = 0;
			int j;

			data = read_file(filepath, &data_size);
			if (!data) {
				ret = 1;
				goto cleanup;
			}

			if (opt_paths)
				member_name = strip_drive(filepath);
			else
				member_name = basename_only(filepath);

			/* Check if member already exists and replace */
			for (j = 0; j < lib.num_members; j++) {
				if (strcmp(lib.members[j].name, member_name) == 0) {
					free(lib.members[j].data);
					lib.members[j].data = data;
					lib.members[j].data_size = data_size;
					lib.members[j].timestamp = get_file_mtime(filepath);
					replaced = 1;
					break;
				}
			}

			if (!replaced) {
				if (lib.num_members >= MAX_MEMBERS) {
					fprintf(stderr, "AOF Librarian: (Error)   Library full.\n");
					free(data);
					ret = 1;
					goto cleanup;
				}
				/* Insert at beginning (original armlib puts new members first) */
				memmove(&lib.members[1], &lib.members[0],
				        (size_t)lib.num_members * sizeof(LibMember));
				lib.members[0].name = strdup(member_name);
				lib.members[0].data = data;
				lib.members[0].data_size = data_size;
				lib.members[0].timestamp = get_file_mtime(filepath);
				lib.num_members++;
			}
		}

		if (write_library(lib_path, &lib, opt_symtab) != 0) {
			ret = 1;
			goto cleanup;
		}
	}

	/* ===== Operation: Delete ===== */
	else if (op_delete) {
		if (read_library(lib_path, &lib) != 0) {
			ret = 1;
			goto cleanup;
		}

		for (i = 0; i < num_files; i++) {
			const char *pattern = file_args[i];
			int j;
			int found = 0;

			for (j = lib.num_members - 1; j >= 0; j--) {
				if (wildcard_match(pattern, lib.members[j].name)) {
					free(lib.members[j].name);
					free(lib.members[j].data);
					memmove(&lib.members[j], &lib.members[j+1],
					        (size_t)(lib.num_members - j - 1) * sizeof(LibMember));
					lib.num_members--;
					found = 1;
				}
			}

			if (!found) {
				fprintf(stderr, "AOF Librarian: (Warning) %s not found in library.\n",
				        pattern);
			}
		}

		if (write_library(lib_path, &lib, opt_symtab) != 0) {
			ret = 1;
			goto cleanup;
		}
	}

	/* ===== Operation: Extract ===== */
	else if (op_extract) {
		if (read_library(lib_path, &lib) != 0) {
			ret = 1;
			goto cleanup;
		}

		/* If no members specified, do nothing (original behavior) */
		for (i = 0; i < num_files; i++) {
			const char *pattern = file_args[i];
			int j;
			int found = 0;

			for (j = 0; j < lib.num_members; j++) {
				if (wildcard_match(pattern, lib.members[j].name)) {
					LibMember *m = &lib.members[j];
					const char *out_name = basename_only(m->name);
					char out_path[4096];
					FILE *outf;

					if (opt_dir)
						snprintf(out_path, sizeof(out_path), "%s/%s",
						         opt_dir, out_name);
					else
						snprintf(out_path, sizeof(out_path), "%s", out_name);

					outf = fopen(out_path, "wb");
					if (!outf) {
						fprintf(stderr, "AOF Librarian: (Error)   Can't create %s.\n",
						        out_path);
						ret = 1;
						goto cleanup;
					}

					if (m->data && m->data_size > 0)
						fwrite(m->data, 1, m->data_size, outf);
					fclose(outf);
					found = 1;
				}
			}

			if (!found) {
				fprintf(stderr, "AOF Librarian: (Warning) %s not found in library.\n",
				        pattern);
			}
		}
	}

	/* ===== No primary operation: just list/symtab or rebuild symtab ===== */
	else if (!opt_list && !opt_listsym) {
		print_usage();
		ret = 1;
		goto cleanup_args;
	} else {
		/* -l or -s only: read the library */
		if (read_library(lib_path, &lib) != 0) {
			ret = 1;
			goto cleanup;
		}
	}

	/* ===== Listing ===== */
	if (opt_list) {
		/* If we did a write operation, re-read the written file for accurate timestamps */
		if (op_create || op_insert || op_delete) {
			Library list_lib;
			memset(&list_lib, 0, sizeof(list_lib));
			if (read_library(lib_path, &list_lib) == 0) {
				list_library(&list_lib, lib_path);
				free_library(&list_lib);
			}
		} else {
			list_library(&lib, lib_path);
		}
	}

	if (opt_list && opt_listsym)
		printf("\n");

	if (opt_listsym) {
		/* If we did a write op, re-read */
		if (op_create || op_insert || op_delete) {
			Library sym_lib;
			memset(&sym_lib, 0, sizeof(sym_lib));
			if (read_library(lib_path, &sym_lib) == 0) {
				list_symtab(&sym_lib, lib_path);
				free_library(&sym_lib);
			}
		} else {
			list_symtab(&lib, lib_path);
		}
	}

cleanup:
	free_library(&lib);

cleanup_args:
	for (i = 0; i < num_all_args; i++)
		free(all_args[i]);

	return ret;
}

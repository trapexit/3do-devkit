/*
 * alfdump - ARM Library Format (ALF) file information tool
 *
 * Parses and prints details of ALF static library files as described
 * in the ARM Library Format specification, including chunk file structure,
 * library directory, member data (with embedded AOF introspection),
 * external symbol table, timestamps, and version information.
 *
 * References:
 *   - ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
 *   - RISC OS Programmer's Reference Manuals
 *   - ARM Software Development Toolkit User Guide
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

/* ===== Chunk File Format Constants ===== */

#define CHUNK_FILE_ID            0xC3CBC6C5
#define CHUNK_FILE_ID_REVERSED   0xC5C6CBC3

/* ===== ALF Constants ===== */

#define ALF_VERSION_CURRENT      1

/* ===== AOF Constants (for embedded member inspection) ===== */

#define AOF_FILE_TYPE            0xC5E2D080

/* AOF symbol attribute flags */
#define AOF_SYM_DEFINED          0x00000001
#define AOF_SYM_GLOBAL           0x00000002
#define AOF_SYM_ABSOLUTE         0x00000004
#define AOF_SYM_WEAK             0x00000010
#define AOF_SYM_STRONG           0x00000020
#define AOF_SYM_COMMON           0x00000040
#define AOF_SYM_LEAF             0x00000800
#define AOF_SYM_THUMB            0x00001000

/* AOF area attribute flags */
#define AOF_ATTR_CODE            0x00000200
#define AOF_ATTR_ZEROINIT        0x00001000
#define AOF_ATTR_READONLY        0x00002000
#define AOF_ATTR_DEBUG           0x00008000

/* ===== Structures ===== */

typedef struct {
	uint32_t chunk_file_id;
	uint32_t max_chunks;
	uint32_t num_chunks;
} ChunkFileHeader;  /* 12 bytes */

typedef struct {
	char     chunk_id[8];
	uint32_t file_offset;  /* 0 = unused entry */
	uint32_t size;
} ChunkDirEntry;  /* 16 bytes */

/* ===== Parsed ALF file state ===== */

typedef struct {
	uint8_t *file_data;
	long     file_size;
	int      swap_endian;

	/* Chunk directory */
	ChunkFileHeader cf_header;
	ChunkDirEntry  *chunks;

	/* Located chunk pointers and sizes */
	uint8_t *lib_diry;   uint32_t lib_diry_size;
	uint8_t *lib_time;   uint32_t lib_time_size;
	uint8_t *lib_vrsn;   uint32_t lib_vrsn_size;
	uint8_t *ofl_symt;   uint32_t ofl_symt_size;
	uint8_t *ofl_time;   uint32_t ofl_time_size;

	/* LIB_DATA chunk indices (for quick lookup) */
	uint32_t  num_data_chunks;
	uint32_t *data_chunk_indices;  /* indices into chunks[] */
} ALFFile;

/* ===== Options ===== */

typedef struct {
	int show_members;    /* -m: show AOF member details (default on) */
	int show_symbols;    /* -s: show symbol table (default on) */
	int show_hex;        /* -x: hex dump of member header data */
	int force_endian;    /* -1=auto, 0=LE, 1=BE */
	int quiet;           /* -q: skip validation */
} Options;

/* ===== Endianness Utilities ===== */

static uint32_t swap32(uint32_t v)
{
	return ((v >> 24) & 0x000000FF) |
	       ((v >>  8) & 0x0000FF00) |
	       ((v <<  8) & 0x00FF0000) |
	       ((v << 24) & 0xFF000000);
}

static uint32_t read32(int do_swap, const uint8_t *p)
{
	uint32_t v;
	memcpy(&v, p, 4);
	return do_swap ? swap32(v) : v;
}

/* ===== Printing Helpers ===== */

static void print_separator(void)
{
	printf("--------------------------------------------------------------\n");
}

static void hexdump(const uint8_t *data, uint32_t len, uint32_t base_offset)
{
	uint32_t i, j;

	printf("  Offset   00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F  ASCII\n");
	print_separator();

	for (i = 0; i < len; i += 16) {
		uint32_t row_len = len - i;
		if (row_len > 16)
			row_len = 16;

		printf("  0x%04X:  ", base_offset + i);
		for (j = 0; j < 16; j++) {
			if (j == 8)
				printf(" ");
			if (j < row_len)
				printf("%02X ", data[i + j]);
			else
				printf("   ");
		}
		printf(" ");
		for (j = 0; j < row_len; j++) {
			uint8_t c = data[i + j];
			printf("%c", (c >= 0x20 && c < 0x7F) ? c : '.');
		}
		printf("\n");
	}
}

/* ===== Timestamp Decoding ===== */

/*
 * ALF timestamps are 6-byte centisecond counts since 1900-01-01 00:00:00,
 * packed into 2 words:
 *   Word 0: MS 4 bytes of the 6-byte centisecond count
 *   Word 1: bits 31-16 = LS 2 bytes of centisecond count
 *            bits 15-0  = microsecond count (usually 0)
 *
 * Epoch: 1900-01-01 00:00:00
 */

/* Seconds between 1900-01-01 and 1970-01-01 (Unix epoch) */
#define RISCOS_UNIX_EPOCH_DIFF  2208988800ULL

static void decode_timestamp(int do_swap, const uint8_t *p, char *buf, size_t bufsz)
{
	uint32_t w0 = read32(do_swap, p);
	uint32_t w1 = read32(do_swap, p + 4);
	uint64_t centisecs;
	uint16_t microsecs;
	uint64_t secs_since_1900;
	time_t unix_time;
	struct tm *tm;

	centisecs = ((uint64_t)w0 << 16) | ((w1 >> 16) & 0xFFFF);
	microsecs = w1 & 0xFFFF;

	if (centisecs == 0) {
		snprintf(buf, bufsz, "(zero / not set)");
		return;
	}

	secs_since_1900 = centisecs / 100;

	if (secs_since_1900 < RISCOS_UNIX_EPOCH_DIFF) {
		snprintf(buf, bufsz, "pre-1970 (centisecs=%llu)",
		         (unsigned long long)centisecs);
		return;
	}

	unix_time = (time_t)(secs_since_1900 - RISCOS_UNIX_EPOCH_DIFF);
	tm = gmtime(&unix_time);
	if (tm) {
		int pos = (int)strftime(buf, bufsz, "%Y-%m-%d %H:%M:%S UTC", tm);
		if (microsecs != 0 && pos > 0 && (size_t)pos < bufsz - 20)
			snprintf(buf + pos, bufsz - (size_t)pos, " (+%u us)", microsecs);
	} else {
		snprintf(buf, bufsz, "centisecs=%llu us=%u",
		         (unsigned long long)centisecs, microsecs);
	}
}

static void print_timestamp_raw(int do_swap, const uint8_t *p)
{
	uint32_t w0 = read32(do_swap, p);
	uint32_t w1 = read32(do_swap, p + 4);

	printf("  Raw words:         0x%08X 0x%08X\n", w0, w1);
}

/* ===== Chunk File Parsing ===== */

static int parse_chunk_file(ALFFile *af)
{
	uint32_t i;
	uint32_t dir_end;

	if (af->file_size < 12) {
		fprintf(stderr, "Error: file too small for chunk file header (%ld bytes)\n",
		        af->file_size);
		return -1;
	}

	/* Read chunk file header */
	memcpy(&af->cf_header, af->file_data, 12);

	/* Detect endianness */
	if (af->cf_header.chunk_file_id == CHUNK_FILE_ID) {
		af->swap_endian = 0;
	} else if (af->cf_header.chunk_file_id == CHUNK_FILE_ID_REVERSED) {
		af->swap_endian = 1;
	} else {
		fprintf(stderr, "Error: not a chunk file (magic 0x%08X, expected 0x%08X)\n",
		        af->cf_header.chunk_file_id, CHUNK_FILE_ID);
		return -1;
	}

	af->cf_header.chunk_file_id = read32(af->swap_endian, af->file_data);
	af->cf_header.max_chunks    = read32(af->swap_endian, af->file_data + 4);
	af->cf_header.num_chunks    = read32(af->swap_endian, af->file_data + 8);

	/* Validate directory fits in file */
	dir_end = 12 + af->cf_header.max_chunks * 16;
	if ((long)dir_end > af->file_size) {
		fprintf(stderr, "Error: chunk directory extends beyond file "
		        "(need %u bytes, file is %ld)\n", dir_end, af->file_size);
		return -1;
	}

	/* Parse chunk directory entries */
	af->chunks = calloc(af->cf_header.max_chunks, sizeof(ChunkDirEntry));
	if (!af->chunks) {
		fprintf(stderr, "Error: out of memory\n");
		return -1;
	}

	/* Temporary: count LIB_DATA chunks */
	af->data_chunk_indices = calloc(af->cf_header.max_chunks, sizeof(uint32_t));
	if (!af->data_chunk_indices) {
		fprintf(stderr, "Error: out of memory\n");
		return -1;
	}
	af->num_data_chunks = 0;

	for (i = 0; i < af->cf_header.max_chunks; i++) {
		const uint8_t *p = af->file_data + 12 + i * 16;
		ChunkDirEntry *e = &af->chunks[i];

		memcpy(e->chunk_id, p, 8);
		e->file_offset = read32(af->swap_endian, p + 8);
		e->size        = read32(af->swap_endian, p + 12);

		if (e->file_offset == 0)
			continue;

		if ((long)(e->file_offset + e->size) > af->file_size) {
			fprintf(stderr, "Warning: chunk '%8.8s' at offset 0x%X size %u "
			        "exceeds file bounds\n", e->chunk_id, e->file_offset,
			        e->size);
			continue;
		}

		if (memcmp(e->chunk_id, "LIB_DIRY", 8) == 0) {
			af->lib_diry = af->file_data + e->file_offset;
			af->lib_diry_size = e->size;
		} else if (memcmp(e->chunk_id, "LIB_TIME", 8) == 0) {
			af->lib_time = af->file_data + e->file_offset;
			af->lib_time_size = e->size;
		} else if (memcmp(e->chunk_id, "LIB_VRSN", 8) == 0) {
			af->lib_vrsn = af->file_data + e->file_offset;
			af->lib_vrsn_size = e->size;
		} else if (memcmp(e->chunk_id, "OFL_SYMT", 8) == 0) {
			af->ofl_symt = af->file_data + e->file_offset;
			af->ofl_symt_size = e->size;
		} else if (memcmp(e->chunk_id, "OFL_TIME", 8) == 0) {
			af->ofl_time = af->file_data + e->file_offset;
			af->ofl_time_size = e->size;
		} else if (memcmp(e->chunk_id, "LIB_DATA", 8) == 0) {
			af->data_chunk_indices[af->num_data_chunks++] = i;
		}
	}

	return 0;
}

/* ===== Print Chunk Directory ===== */

static void print_chunk_directory(const ALFFile *af)
{
	uint32_t i;

	printf("=== Chunk File Header ===\n");
	print_separator();
	printf("  ChunkFileId:     0x%08X\n", af->cf_header.chunk_file_id);
	printf("  maxChunks:       %u\n", af->cf_header.max_chunks);
	printf("  numChunks:       %u\n", af->cf_header.num_chunks);
	printf("  Endianness:      %s\n",
	       af->swap_endian ? "big-endian (byte-swapped)" : "little-endian (native)");
	print_separator();

	printf("\n=== Chunk Directory (%u slots) ===\n", af->cf_header.max_chunks);
	print_separator();
	printf("  %-4s  %-10s  %-10s  %-10s\n", "Slot", "Chunk ID", "Offset", "Size");
	print_separator();

	for (i = 0; i < af->cf_header.max_chunks; i++) {
		const ChunkDirEntry *e = &af->chunks[i];

		if (e->file_offset == 0) {
			printf("  %-4u  (unused)\n", i);
		} else {
			printf("  %-4u  %8.8s    0x%08X  %u bytes\n",
			       i, e->chunk_id, e->file_offset, e->size);
		}
	}
	print_separator();
}

/* ===== Print LIB_VRSN ===== */

static void print_version(const ALFFile *af)
{
	printf("\n=== Library Version (LIB_VRSN) ===\n");
	print_separator();

	if (!af->lib_vrsn || af->lib_vrsn_size < 4) {
		printf("  (not present or too small)\n");
		print_separator();
		return;
	}

	{
		uint32_t ver = read32(af->swap_endian, af->lib_vrsn);
		printf("  Version:   %u", ver);
		if (ver == ALF_VERSION_CURRENT)
			printf(" (current)");
		else
			printf(" (unexpected, expected %u)", ALF_VERSION_CURRENT);
		printf("\n");
	}
	print_separator();
}

/* ===== Print Timestamps ===== */

static void print_timestamps(const ALFFile *af)
{
	char tbuf[128];

	printf("\n=== Library Timestamp (LIB_TIME) ===\n");
	print_separator();

	if (!af->lib_time || af->lib_time_size < 8) {
		printf("  (not present or too small)\n");
	} else {
		print_timestamp_raw(af->swap_endian, af->lib_time);
		decode_timestamp(af->swap_endian, af->lib_time, tbuf, sizeof(tbuf));
		printf("  Decoded:           %s\n", tbuf);
	}
	print_separator();

	printf("\n=== Symbol Table Timestamp (OFL_TIME) ===\n");
	print_separator();

	if (!af->ofl_time || af->ofl_time_size < 8) {
		printf("  (not present)\n");
	} else {
		print_timestamp_raw(af->swap_endian, af->ofl_time);
		decode_timestamp(af->swap_endian, af->ofl_time, tbuf, sizeof(tbuf));
		printf("  Decoded:           %s\n", tbuf);

		/* Check staleness */
		if (af->lib_time && af->lib_time_size >= 8) {
			uint64_t lib_cs = ((uint64_t)read32(af->swap_endian, af->lib_time) << 16) |
			                  ((read32(af->swap_endian, af->lib_time + 4) >> 16) & 0xFFFF);
			uint64_t ofl_cs = ((uint64_t)read32(af->swap_endian, af->ofl_time) << 16) |
			                  ((read32(af->swap_endian, af->ofl_time + 4) >> 16) & 0xFFFF);
			if (ofl_cs < lib_cs)
				printf("  WARNING: OFL_TIME < LIB_TIME -- symbol table may be stale\n");
		}
	}
	print_separator();
}

/* ===== Print LIB_DIRY ===== */

static void print_directory(const ALFFile *af)
{
	uint32_t offset = 0;
	uint32_t entry_num = 0;
	char tbuf[128];

	printf("\n=== Library Directory (LIB_DIRY) ===\n");
	print_separator();

	if (!af->lib_diry || af->lib_diry_size < 12) {
		printf("  (not present or too small)\n");
		print_separator();
		return;
	}

	printf("  Directory size:    %u bytes\n", af->lib_diry_size);
	print_separator();

	while (offset + 12 <= af->lib_diry_size) {
		const uint8_t *ep = af->lib_diry + offset;
		uint32_t chunk_index  = read32(af->swap_endian, ep + 0);
		uint32_t entry_length = read32(af->swap_endian, ep + 4);
		uint32_t data_length  = read32(af->swap_endian, ep + 8);
		const char *name;

		if (entry_length < 12 || offset + entry_length > af->lib_diry_size) {
			printf("  Entry %u: invalid entry_length %u at offset %u, stopping\n",
			       entry_num, entry_length, offset);
			break;
		}

		/* Extract member name from data section */
		if (data_length > 0 && offset + 12 < af->lib_diry_size)
			name = (const char *)(ep + 12);
		else
			name = "(no name)";

		if (chunk_index == 0) {
			printf("  Entry %-3u  [DELETED/UNUSED]\n", entry_num);
			printf("    EntryLength:     %u\n", entry_length);
			printf("    DataLength:      %u\n", data_length);
			if (data_length > 0)
				printf("    Name:            \"%s\"\n", name);
		} else {
			printf("  Entry %-3u  ChunkIndex=%u", entry_num, chunk_index);

			/* Resolve the chunk */
			if (chunk_index < af->cf_header.max_chunks) {
				const ChunkDirEntry *ce = &af->chunks[chunk_index];
				if (ce->file_offset != 0)
					printf("  -> %8.8s at 0x%X (%u bytes)",
					       ce->chunk_id, ce->file_offset, ce->size);
				else
					printf("  -> (unused chunk slot)");
			} else {
				printf("  -> (out of range)");
			}
			printf("\n");

			printf("    Name:            \"%s\"\n", name);
			printf("    EntryLength:     %u\n", entry_length);
			printf("    DataLength:      %u\n", data_length);

			/* Try to decode timestamp at end of data section */
			if (data_length >= 8) {
				/*
				 * Timestamp is the last 8 bytes of the data section,
				 * word-aligned. Walk past the NUL-terminated name,
				 * then to the timestamp at the end of data_length.
				 */
				uint32_t ts_offset = 12 + data_length - 8;
				if (ts_offset + 8 <= entry_length &&
				    offset + ts_offset + 8 <= af->lib_diry_size) {
					decode_timestamp(af->swap_endian,
					                 ep + ts_offset, tbuf, sizeof(tbuf));
					printf("    Timestamp:       %s\n", tbuf);
				}
			}
		}

		offset += entry_length;
		entry_num++;
		printf("\n");
	}

	printf("  Total entries:     %u (%u active)\n", entry_num,
	       af->num_data_chunks);
	print_separator();
}

/* ===== Print OFL_SYMT ===== */

static void print_symbol_table(const ALFFile *af)
{
	uint32_t offset = 0;
	uint32_t entry_num = 0;
	uint32_t active_count = 0;

	printf("\n=== External Symbol Table (OFL_SYMT) ===\n");
	print_separator();

	if (!af->ofl_symt || af->ofl_symt_size < 12) {
		printf("  (not present)\n");
		print_separator();
		return;
	}

	printf("  Symbol table size: %u bytes\n", af->ofl_symt_size);
	print_separator();

	printf("  %-5s  %-6s  %-40s  %s\n",
	       "Index", "Chunk", "Symbol Name", "Member");
	print_separator();

	while (offset + 12 <= af->ofl_symt_size) {
		const uint8_t *ep = af->ofl_symt + offset;
		uint32_t chunk_index  = read32(af->swap_endian, ep + 0);
		uint32_t entry_length = read32(af->swap_endian, ep + 4);
		uint32_t data_length  = read32(af->swap_endian, ep + 8);
		const char *sym_name;
		const char *member_name = "";

		if (entry_length < 12 || offset + entry_length > af->ofl_symt_size) {
			printf("  Entry %u: invalid entry_length %u at offset %u, stopping\n",
			       entry_num, entry_length, offset);
			break;
		}

		if (data_length > 0 && offset + 12 < af->ofl_symt_size)
			sym_name = (const char *)(ep + 12);
		else
			sym_name = "(no name)";

		if (chunk_index == 0) {
			/* Unused entry */
			offset += entry_length;
			entry_num++;
			continue;
		}

		active_count++;

		/*
		 * Resolve chunk_index to member name by scanning LIB_DIRY
		 * for an entry with matching chunk_index.
		 */
		if (af->lib_diry) {
			uint32_t diry_off = 0;
			while (diry_off + 12 <= af->lib_diry_size) {
				const uint8_t *dp = af->lib_diry + diry_off;
				uint32_t d_ci = read32(af->swap_endian, dp + 0);
				uint32_t d_el = read32(af->swap_endian, dp + 4);
				uint32_t d_dl = read32(af->swap_endian, dp + 8);

				if (d_el < 12 || diry_off + d_el > af->lib_diry_size)
					break;

				if (d_ci == chunk_index && d_dl > 0) {
					member_name = (const char *)(dp + 12);
					break;
				}
				diry_off += d_el;
			}
		}

		printf("  %-5u  %-6u  %-40s  %s\n",
		       entry_num, chunk_index, sym_name, member_name);

		offset += entry_length;
		entry_num++;
	}

	print_separator();
	printf("  Total entries:     %u (%u active)\n", entry_num, active_count);
	print_separator();
}

/* ===== Print LIB_DATA Member Details (embedded AOF introspection) ===== */

/*
 * For each LIB_DATA chunk that contains an embedded AOF file, parse
 * its OBJ_HEAD and OBJ_STRT to print area and symbol summaries.
 */
static void print_member_aof_summary(const ALFFile *af, uint32_t chunk_idx,
                                     const char *member_name, int show_hex)
{
	const ChunkDirEntry *ce;
	const uint8_t *data;
	uint32_t data_size;
	uint32_t m_magic;
	int m_swap;
	uint32_t m_max;
	uint32_t m_dir_end;
	uint32_t i;

	/* AOF inner chunk pointers */
	const uint8_t *m_head = NULL;  uint32_t m_head_size = 0;
	const uint8_t *m_strt = NULL;  uint32_t m_strt_size = 0;
	const uint8_t *m_idfn = NULL;  uint32_t m_idfn_size = 0;
	const uint8_t *m_symt = NULL;  uint32_t m_symt_size = 0;

	if (chunk_idx >= af->cf_header.max_chunks)
		return;

	ce = &af->chunks[chunk_idx];
	if (ce->file_offset == 0 || ce->size < 12)
		return;

	data = af->file_data + ce->file_offset;
	data_size = ce->size;

	printf("  --- Member: \"%s\" (chunk %u, %u bytes) ---\n",
	       member_name, chunk_idx, data_size);

	/* Check if this is an embedded chunk file (AOF) */
	memcpy(&m_magic, data, 4);
	if (m_magic == CHUNK_FILE_ID) {
		m_swap = 0;
	} else if (m_magic == CHUNK_FILE_ID_REVERSED) {
		m_swap = 1;
	} else {
		printf("    Not an AOF file (magic 0x%08X)\n", m_magic);
		if (show_hex) {
			uint32_t dump = data_size > 128 ? 128 : data_size;
			printf("    First %u bytes:\n", dump);
			hexdump(data, dump, 0);
		}
		return;
	}

	m_max = read32(m_swap, data + 4);
	m_dir_end = 12 + m_max * 16;

	if (m_dir_end > data_size) {
		printf("    AOF chunk directory extends beyond member data\n");
		return;
	}

	/* Scan inner chunk directory */
	for (i = 0; i < m_max; i++) {
		const uint8_t *p = data + 12 + i * 16;
		char cid[8];
		uint32_t c_off, c_sz;

		memcpy(cid, p, 8);
		c_off = read32(m_swap, p + 8);
		c_sz  = read32(m_swap, p + 12);

		if (c_off == 0)
			continue;

		/* Offsets are relative to the member file start (which is data) */
		if (c_off + c_sz > data_size)
			continue;

		if (memcmp(cid, "OBJ_HEAD", 8) == 0) {
			m_head = data + c_off;
			m_head_size = c_sz;
		} else if (memcmp(cid, "OBJ_STRT", 8) == 0) {
			m_strt = data + c_off;
			m_strt_size = c_sz;
		} else if (memcmp(cid, "OBJ_IDFN", 8) == 0) {
			m_idfn = data + c_off;
			m_idfn_size = c_sz;
		} else if (memcmp(cid, "OBJ_SYMT", 8) == 0) {
			m_symt = data + c_off;
			m_symt_size = c_sz;
		}
	}

	/* Print identification */
	if (m_idfn && m_idfn_size > 0) {
		char *id_buf = malloc(m_idfn_size + 1);
		if (id_buf) {
			memcpy(id_buf, m_idfn, m_idfn_size);
			id_buf[m_idfn_size] = '\0';
			printf("    Tool:            \"%s\"\n", id_buf);
			free(id_buf);
		}
	}

	/* Parse OBJ_HEAD */
	if (!m_head || m_head_size < 24) {
		printf("    (no valid OBJ_HEAD)\n");
		return;
	}

	{
		uint32_t obj_type   = read32(m_swap, m_head + 0);
		uint32_t version    = read32(m_swap, m_head + 4);
		uint32_t num_areas  = read32(m_swap, m_head + 8);
		uint32_t num_syms   = read32(m_swap, m_head + 12);
		uint32_t entry_area = read32(m_swap, m_head + 16);
		uint32_t j;

		if (obj_type != AOF_FILE_TYPE) {
			printf("    Not AOF (type 0x%08X)\n", obj_type);
			return;
		}

		printf("    AOF version:     %u\n", version);
		printf("    Areas:           %u\n", num_areas);
		printf("    Symbols:         %u\n", num_syms);
		if (entry_area != 0)
			printf("    Entry area:      %u\n", entry_area);

		/* Print area summary */
		if (m_head_size >= 24 + num_areas * 20) {
			for (j = 0; j < num_areas; j++) {
				const uint8_t *ah = m_head + 24 + j * 20;
				uint32_t a_name  = read32(m_swap, ah + 0);
				uint32_t a_attr  = read32(m_swap, ah + 4);
				uint32_t a_size  = read32(m_swap, ah + 8);
				uint32_t a_reloc = read32(m_swap, ah + 12);
				const char *a_name_str = "<unknown>";

				if (m_strt && a_name >= 4 && a_name < m_strt_size)
					a_name_str = (const char *)(m_strt + a_name);

				printf("      Area %u: %-20s %5u bytes  %s%s%s  relocs=%u\n",
				       j, a_name_str, a_size,
				       (a_attr & AOF_ATTR_CODE) ? "Code" : "Data",
				       (a_attr & AOF_ATTR_READONLY) ? " RO" : "",
				       (a_attr & AOF_ATTR_ZEROINIT) ? " ZI" : "",
				       a_reloc);
			}
		}

		/* Print exported symbols summary */
		if (m_symt && m_strt && num_syms > 0 && m_symt_size >= num_syms * 16) {
			uint32_t export_count = 0;

			for (j = 0; j < num_syms; j++) {
				uint32_t s_attr = read32(m_swap, m_symt + j * 16 + 4);
				if ((s_attr & AOF_SYM_DEFINED) && (s_attr & AOF_SYM_GLOBAL))
					export_count++;
			}

			printf("    Exported:        %u symbol(s)\n", export_count);

			/* List exports */
			for (j = 0; j < num_syms; j++) {
				uint32_t s_name = read32(m_swap, m_symt + j * 16 + 0);
				uint32_t s_attr = read32(m_swap, m_symt + j * 16 + 4);
				uint32_t s_val  = read32(m_swap, m_symt + j * 16 + 8);
				const char *s_name_str;
				int first_flag;

				if (!((s_attr & AOF_SYM_DEFINED) && (s_attr & AOF_SYM_GLOBAL)))
					continue;

				s_name_str = "<unknown>";
				if (s_name >= 4 && s_name < m_strt_size)
					s_name_str = (const char *)(m_strt + s_name);

				printf("      [export] %-30s  value=0x%08X", s_name_str, s_val);

				first_flag = 1;
				if (s_attr & AOF_SYM_WEAK) {
					printf("%s Weak", first_flag ? " |" : ",");
					first_flag = 0;
				}
				if (s_attr & AOF_SYM_STRONG) {
					printf("%s Strong", first_flag ? " |" : ",");
					first_flag = 0;
				}
				if (s_attr & AOF_SYM_ABSOLUTE) {
					printf("%s Absolute", first_flag ? " |" : ",");
					first_flag = 0;
				}
				if (s_attr & AOF_SYM_LEAF) {
					printf("%s Leaf", first_flag ? " |" : ",");
					first_flag = 0;
				}
				if (s_attr & AOF_SYM_THUMB) {
					printf("%s Thumb", first_flag ? " |" : ",");
					first_flag = 0;
				}
				(void)first_flag;
				printf("\n");
			}
		}
	}
}

static void print_members(const ALFFile *af, int show_hex)
{
	uint32_t offset;
	uint32_t entry_num = 0;

	printf("\n=== Library Members (LIB_DATA) ===\n");
	print_separator();

	if (!af->lib_diry || af->lib_diry_size < 12) {
		/* No directory, just list data chunks */
		uint32_t i;
		printf("  (no directory available, listing LIB_DATA chunks)\n\n");
		for (i = 0; i < af->num_data_chunks; i++) {
			uint32_t ci = af->data_chunk_indices[i];
			printf("  Chunk %u: %u bytes\n", ci, af->chunks[ci].size);
		}
		print_separator();
		return;
	}

	printf("  %u LIB_DATA chunk(s) in file\n\n", af->num_data_chunks);

	/* Walk directory entries and print member details */
	offset = 0;
	while (offset + 12 <= af->lib_diry_size) {
		const uint8_t *ep = af->lib_diry + offset;
		uint32_t chunk_index  = read32(af->swap_endian, ep + 0);
		uint32_t entry_length = read32(af->swap_endian, ep + 4);
		uint32_t data_length  = read32(af->swap_endian, ep + 8);
		const char *name;

		if (entry_length < 12 || offset + entry_length > af->lib_diry_size)
			break;

		if (chunk_index != 0) {
			if (data_length > 0 && offset + 12 < af->lib_diry_size)
				name = (const char *)(ep + 12);
			else
				name = "(unnamed)";

			print_member_aof_summary(af, chunk_index, name, show_hex);
			printf("\n");
		}

		offset += entry_length;
		entry_num++;
	}

	print_separator();
}

/* ===== Validation ===== */

static int validate_alf(const ALFFile *af)
{
	int warnings = 0;
	uint32_t offset;

	printf("\n=== Validation ===\n");
	print_separator();

	/* Chunk file level */
	if (af->cf_header.num_chunks > af->cf_header.max_chunks) {
		printf("  WARNING: numChunks (%u) > maxChunks (%u)\n",
		       af->cf_header.num_chunks, af->cf_header.max_chunks);
		warnings++;
	}

	/* Required chunks */
	if (!af->lib_diry) {
		printf("  ERROR: LIB_DIRY chunk missing (required)\n");
		warnings++;
	}
	if (!af->lib_time) {
		printf("  ERROR: LIB_TIME chunk missing (required)\n");
		warnings++;
	}
	if (!af->lib_vrsn) {
		printf("  ERROR: LIB_VRSN chunk missing (required)\n");
		warnings++;
	}

	/* Version */
	if (af->lib_vrsn && af->lib_vrsn_size >= 4) {
		uint32_t ver = read32(af->swap_endian, af->lib_vrsn);
		if (ver != ALF_VERSION_CURRENT) {
			printf("  WARNING: LIB_VRSN is %u, expected %u\n",
			       ver, ALF_VERSION_CURRENT);
			warnings++;
		}
	}

	/* LIB_TIME size */
	if (af->lib_time && af->lib_time_size != 8) {
		printf("  WARNING: LIB_TIME size is %u, expected 8\n",
		       af->lib_time_size);
		warnings++;
	}

	/* OFL_SYMT / OFL_TIME consistency */
	if (af->ofl_symt && !af->ofl_time) {
		printf("  WARNING: OFL_SYMT present but OFL_TIME missing\n");
		warnings++;
	}
	if (!af->ofl_symt && af->ofl_time) {
		printf("  WARNING: OFL_TIME present but OFL_SYMT missing\n");
		warnings++;
	}
	if (af->ofl_time && af->ofl_time_size != 8) {
		printf("  WARNING: OFL_TIME size is %u, expected 8\n",
		       af->ofl_time_size);
		warnings++;
	}

	/* Validate chunk file offsets are 4-byte aligned */
	{
		uint32_t i;
		for (i = 0; i < af->cf_header.max_chunks; i++) {
			const ChunkDirEntry *e = &af->chunks[i];
			if (e->file_offset != 0 && (e->file_offset & 3) != 0) {
				printf("  WARNING: chunk %u (%8.8s) offset 0x%X "
				       "not 4-byte aligned\n",
				       i, e->chunk_id, e->file_offset);
				warnings++;
			}
		}
	}

	/* Validate directory entries */
	if (af->lib_diry && af->lib_diry_size >= 12) {
		offset = 0;
		while (offset + 12 <= af->lib_diry_size) {
			const uint8_t *ep = af->lib_diry + offset;
			uint32_t chunk_index  = read32(af->swap_endian, ep + 0);
			uint32_t entry_length = read32(af->swap_endian, ep + 4);
			uint32_t data_length  = read32(af->swap_endian, ep + 8);

			if (entry_length < 12) {
				printf("  ERROR: directory entry at offset %u has "
				       "entry_length %u < 12\n", offset, entry_length);
				warnings++;
				break;
			}
			if (offset + entry_length > af->lib_diry_size) {
				printf("  ERROR: directory entry at offset %u extends "
				       "beyond directory\n", offset);
				warnings++;
				break;
			}
			if (entry_length & 3) {
				printf("  WARNING: entry_length %u at offset %u "
				       "not multiple of 4\n", entry_length, offset);
				warnings++;
			}
			if (data_length & 3) {
				printf("  WARNING: data_length %u at offset %u "
				       "not multiple of 4\n", data_length, offset);
				warnings++;
			}
			if (data_length > entry_length - 12) {
				printf("  WARNING: data_length %u > entry_length-12 (%u) "
				       "at offset %u\n",
				       data_length, entry_length - 12, offset);
				warnings++;
			}

			/* Check chunk_index validity */
			if (chunk_index != 0) {
				if (chunk_index >= af->cf_header.max_chunks) {
					printf("  WARNING: directory entry at offset %u "
					       "chunk_index %u out of range\n",
					       offset, chunk_index);
					warnings++;
				} else {
					const ChunkDirEntry *ce = &af->chunks[chunk_index];
					if (ce->file_offset == 0) {
						printf("  WARNING: directory entry at offset %u "
						       "references unused chunk %u\n",
						       offset, chunk_index);
						warnings++;
					} else if (memcmp(ce->chunk_id, "LIB_DATA", 8) != 0) {
						printf("  WARNING: directory entry at offset %u "
						       "chunk %u is %8.8s, expected LIB_DATA\n",
						       offset, chunk_index, ce->chunk_id);
						warnings++;
					}
				}
			}

			offset += entry_length;
		}
	}

	/* Validate OFL_SYMT entries */
	if (af->ofl_symt && af->ofl_symt_size >= 12) {
		offset = 0;
		while (offset + 12 <= af->ofl_symt_size) {
			const uint8_t *ep = af->ofl_symt + offset;
			uint32_t chunk_index  = read32(af->swap_endian, ep + 0);
			uint32_t entry_length = read32(af->swap_endian, ep + 4);

			if (entry_length < 12) {
				printf("  ERROR: OFL_SYMT entry at offset %u has "
				       "entry_length %u < 12\n", offset, entry_length);
				warnings++;
				break;
			}
			if (offset + entry_length > af->ofl_symt_size) {
				printf("  ERROR: OFL_SYMT entry at offset %u extends "
				       "beyond chunk\n", offset);
				warnings++;
				break;
			}

			if (chunk_index != 0 && chunk_index >= af->cf_header.max_chunks) {
				printf("  WARNING: OFL_SYMT entry at offset %u "
				       "chunk_index %u out of range\n",
				       offset, chunk_index);
				warnings++;
			}

			offset += entry_length;
		}
	}

	if (warnings == 0)
		printf("  No issues found.\n");
	else
		printf("  %d warning(s)/error(s) found.\n", warnings);

	print_separator();
	return warnings;
}

/* ===== Determine if file is ALF ===== */

/*
 * Distinguish ALF from AOF: both use the same chunk file magic.
 * ALF has LIB_* chunks; AOF has OBJ_* chunks.
 * We check for the presence of LIB_DIRY or LIB_VRSN.
 */
static int is_alf_file(const ALFFile *af)
{
	uint32_t i;

	for (i = 0; i < af->cf_header.max_chunks; i++) {
		if (af->chunks[i].file_offset == 0)
			continue;
		if (memcmp(af->chunks[i].chunk_id, "LIB_", 4) == 0 ||
		    memcmp(af->chunks[i].chunk_id, "OFL_", 4) == 0)
			return 1;
	}
	return 0;
}

/* ===== Usage ===== */

static void usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s [options] <alf-file>\n"
		"\n"
		"Options:\n"
		"  -h, --help           Show this help message\n"
		"  -x, --hex            Include hex dump of non-AOF member data\n"
		"  -M, --no-members     Suppress member AOF details\n"
		"  -S, --no-symbols     Suppress external symbol table\n"
		"  -b, --big-endian     Force big-endian interpretation\n"
		"  -l, --little-endian  Force little-endian interpretation\n"
		"  -q, --quiet          Skip validation checks\n"
		"\n"
		"Prints detailed information about an ARM Library Format (ALF) file,\n"
		"including chunk directory, library directory, member details,\n"
		"external symbol table, timestamps, and version.\n",
		prog);
}

/* ===== Main ===== */

int main(int argc, char *argv[])
{
	const char *filename = NULL;
	Options opts;
	ALFFile af;
	FILE *f;
	int i;

	memset(&opts, 0, sizeof(opts));
	opts.show_members = 1;
	opts.show_symbols = 1;
	opts.force_endian = -1;

	memset(&af, 0, sizeof(af));

	/* Parse arguments */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			usage(argv[0]);
			return 0;
		} else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--hex") == 0) {
			opts.show_hex = 1;
		} else if (strcmp(argv[i], "-M") == 0 || strcmp(argv[i], "--no-members") == 0) {
			opts.show_members = 0;
		} else if (strcmp(argv[i], "-S") == 0 || strcmp(argv[i], "--no-symbols") == 0) {
			opts.show_symbols = 0;
		} else if (strcmp(argv[i], "-b") == 0 ||
		           strcmp(argv[i], "--big-endian") == 0) {
			opts.force_endian = 1;
		} else if (strcmp(argv[i], "-l") == 0 ||
		           strcmp(argv[i], "--little-endian") == 0) {
			opts.force_endian = 0;
		} else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
			opts.quiet = 1;
		} else if (argv[i][0] == '-') {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			usage(argv[0]);
			return 1;
		} else {
			if (filename != NULL) {
				fprintf(stderr, "Error: multiple input files specified\n");
				usage(argv[0]);
				return 1;
			}
			filename = argv[i];
		}
	}

	if (filename == NULL) {
		fprintf(stderr, "Error: no input file specified\n");
		usage(argv[0]);
		return 1;
	}

	/* Open and read entire file */
	f = fopen(filename, "rb");
	if (!f) {
		perror(filename);
		return 1;
	}

	fseek(f, 0, SEEK_END);
	af.file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (af.file_size < 12) {
		fprintf(stderr, "Error: file is %ld bytes, too small for chunk file header\n",
		        af.file_size);
		fclose(f);
		return 1;
	}

	af.file_data = malloc((size_t)af.file_size);
	if (!af.file_data) {
		fprintf(stderr, "Error: failed to allocate %ld bytes\n", af.file_size);
		fclose(f);
		return 1;
	}

	if (fread(af.file_data, 1, (size_t)af.file_size, f) != (size_t)af.file_size) {
		fprintf(stderr, "Error: failed to read file\n");
		free(af.file_data);
		fclose(f);
		return 1;
	}
	fclose(f);

	/* Parse chunk file structure */
	if (parse_chunk_file(&af) != 0) {
		free(af.file_data);
		free(af.chunks);
		free(af.data_chunk_indices);
		return 1;
	}

	/* Override endianness if forced */
	if (opts.force_endian >= 0)
		af.swap_endian = opts.force_endian;

	/* Check that this is actually an ALF file */
	if (!is_alf_file(&af)) {
		fprintf(stderr, "Error: file does not contain ALF chunks "
		        "(LIB_/OFL_ prefixes not found).\n"
		        "       This may be an AOF file; use aofdump instead.\n");
		free(af.file_data);
		free(af.chunks);
		free(af.data_chunk_indices);
		return 1;
	}

	/* Print banner */
	printf("alfdump: ARM Library Format (ALF) File Analyzer\n");
	print_separator();
	printf("  File:     %s\n", filename);
	printf("  Size:     %ld bytes\n", af.file_size);
	printf("  Endian:   %s%s\n",
	       af.swap_endian ? "big-endian" : "little-endian",
	       opts.force_endian >= 0 ? " (forced)" : " (detected)");
	printf("  Members:  %u LIB_DATA chunk(s)\n", af.num_data_chunks);
	print_separator();

	/* Print sections */
	print_chunk_directory(&af);
	print_version(&af);
	print_timestamps(&af);
	print_directory(&af);

	if (opts.show_symbols)
		print_symbol_table(&af);

	if (opts.show_members)
		print_members(&af, opts.show_hex);

	if (!opts.quiet)
		validate_alf(&af);

	printf("\n");

	/* Cleanup */
	free(af.data_chunk_indices);
	free(af.chunks);
	free(af.file_data);

	return 0;
}

/*
 * aofdump - ARM Object Format (AOF) file information tool
 *
 * Parses and prints details of AOF relocatable object files as described
 * in the ARM Object Format specification, including chunk file structure,
 * area declarations, symbol tables, relocation directives, and tool
 * identification strings.
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

/* ===== Chunk File Format Constants ===== */

#define CHUNK_FILE_ID            0xC3CBC6C5
#define CHUNK_FILE_ID_REVERSED   0xC5C6CBC3

/* ===== AOF Constants ===== */

#define AOF_FILE_TYPE            0xC5E2D080
#define AOF_VERSION_150          150
#define AOF_VERSION_200          200
#define AOF_VERSION_310          310  /* 0x136 -- current version */
#define AOF_VERSION_311          311  /* 0x137 -- SDT 2.51 variant */

/* ===== Area Attribute Flags (version 3.10 encoding) ===== */

#define AOF_ALIGN_MASK           0x000000FF
#define AOF_ATTR_ABSOLUTE        0x00000100  /* Bit 8  */
#define AOF_ATTR_CODE            0x00000200  /* Bit 9  */
#define AOF_ATTR_COMDEF          0x00000400  /* Bit 10 */
#define AOF_ATTR_COMREF          0x00000800  /* Bit 11 */
#define AOF_ATTR_ZEROINIT        0x00001000  /* Bit 12 */
#define AOF_ATTR_READONLY        0x00002000  /* Bit 13 */
#define AOF_ATTR_PIC             0x00004000  /* Bit 14 */
#define AOF_ATTR_DEBUG           0x00008000  /* Bit 15 */
#define AOF_ATTR_APCS32          0x00010000  /* Bit 16 */
#define AOF_ATTR_REENTRANT       0x00020000  /* Bit 17 */
#define AOF_ATTR_EXTENDEDFP      0x00040000  /* Bit 18 */
#define AOF_ATTR_NOSTACKCHECK    0x00080000  /* Bit 19 */
#define AOF_ATTR_THUMB           0x00100000  /* Bit 20 (code) */
#define AOF_ATTR_BASED           0x00100000  /* Bit 20 (data) */
#define AOF_ATTR_HALFWORD        0x00200000  /* Bit 21 (code) */
#define AOF_ATTR_STUB            0x00200000  /* Bit 21 (data) */
#define AOF_ATTR_INTERWORK       0x00400000  /* Bit 22 */
#define AOF_ATTR_BASEREG_MASK    0x0F000000  /* Bits 24-27 */
#define AOF_ATTR_BASEREG_SHIFT   24

/* ===== Relocation Flags ===== */

#define AOF_RELOC_SID_MASK       0x00FFFFFF  /* Bits 0-23  */
#define AOF_RELOC_FT_MASK        0x03000000  /* Bits 24-25 */
#define AOF_RELOC_FT_SHIFT       24
#define AOF_RELOC_R              0x04000000  /* Bit 26 */
#define AOF_RELOC_A              0x08000000  /* Bit 27 */
#define AOF_RELOC_B              0x10000000  /* Bit 28 */
#define AOF_RELOC_II_MASK        0x60000000  /* Bits 29-30 */
#define AOF_RELOC_II_SHIFT       29

#define AOF_FT_BYTE              0
#define AOF_FT_HALFWORD          1
#define AOF_FT_WORD              2
#define AOF_FT_INSTRUCTION       3

/* ===== Symbol Attribute Flags ===== */

#define AOF_SYM_DEFINED          0x00000001  /* Bit 0  */
#define AOF_SYM_GLOBAL           0x00000002  /* Bit 1  */
#define AOF_SYM_ABSOLUTE         0x00000004  /* Bit 2  */
#define AOF_SYM_CASEINSENS       0x00000008  /* Bit 3  */
#define AOF_SYM_WEAK             0x00000010  /* Bit 4  */
#define AOF_SYM_STRONG           0x00000020  /* Bit 5  */
#define AOF_SYM_COMMON           0x00000040  /* Bit 6  */
#define AOF_SYM_CODEDATUM        0x00000100  /* Bit 8  */
#define AOF_SYM_FPARGS           0x00000200  /* Bit 9  */
#define AOF_SYM_LEAF             0x00000800  /* Bit 11 */
#define AOF_SYM_THUMB            0x00001000  /* Bit 12 */
#define AOF_SYM_TYPE_MASK        0x0F000000  /* Bits 24-27 */
#define AOF_SYM_TYPE_SHIFT       24
#define AOF_SYM_DATUM_MASK       0x00FF0000  /* Bits 16-23 */
#define AOF_SYM_DATUM_SHIFT      16

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

typedef struct {
	uint32_t object_file_type;  /* 0xC5E2D080 */
	uint32_t version_id;
	uint32_t num_areas;
	uint32_t num_symbols;
	uint32_t entry_area;        /* 1-origin, 0 = no entry point */
	uint32_t entry_offset;
} AOFHeader;  /* 24 bytes */

typedef struct {
	uint32_t name;           /* offset into OBJ_STRT */
	uint32_t attributes;
	uint32_t size;
	uint32_t num_relocs;
	uint32_t base_address;
} AOFAreaHeader;  /* 20 bytes */

typedef struct {
	uint32_t offset;
	uint32_t flags;
} AOFReloc;  /* 8 bytes */

typedef struct {
	uint32_t name;           /* offset into OBJ_STRT */
	uint32_t attributes;
	uint32_t value;
	uint32_t area_name;      /* offset into OBJ_STRT */
} AOFSymbol;  /* 16 bytes */

/* ===== Parsed file state ===== */

typedef struct {
	uint8_t *file_data;
	long     file_size;
	int      swap_endian;

	/* Chunk directory */
	ChunkFileHeader cf_header;
	ChunkDirEntry  *chunks;

	/* Located chunk pointers and sizes */
	uint8_t *obj_head;   uint32_t obj_head_size;
	uint8_t *obj_area;   uint32_t obj_area_size;
	uint8_t *obj_symt;   uint32_t obj_symt_size;
	uint8_t *obj_strt;   uint32_t obj_strt_size;
	uint8_t *obj_idfn;   uint32_t obj_idfn_size;

	/* Parsed AOF header */
	AOFHeader      aof_header;
	AOFAreaHeader *area_headers;  /* num_areas entries */
} AOFFile;

/* ===== Options ===== */

typedef struct {
	int show_hex;        /* -x: hex dump area data */
	int show_relocs;     /* -r: show relocation directives (default on) */
	int show_symbols;    /* -s: show symbol table (default on) */
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

static uint32_t fix32(const AOFFile *af, uint32_t v)
{
	return af->swap_endian ? swap32(v) : v;
}

static uint32_t read32(const AOFFile *af, const uint8_t *p)
{
	uint32_t v;
	memcpy(&v, p, 4);
	return fix32(af, v);
}

/* ===== String Table Lookup ===== */

static const char *strt_lookup(const AOFFile *af, uint32_t offset)
{
	if (!af->obj_strt || offset < 4 || offset >= af->obj_strt_size)
		return "<invalid offset>";
	return (const char *)(af->obj_strt + offset);
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

/* ===== Chunk File Parsing ===== */

static int parse_chunk_file(AOFFile *af)
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

	af->cf_header.chunk_file_id = fix32(af, af->cf_header.chunk_file_id);
	af->cf_header.max_chunks    = fix32(af, af->cf_header.max_chunks);
	af->cf_header.num_chunks    = fix32(af, af->cf_header.num_chunks);

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

	for (i = 0; i < af->cf_header.max_chunks; i++) {
		const uint8_t *p = af->file_data + 12 + i * 16;
		ChunkDirEntry *e = &af->chunks[i];

		/* Chunk ID is a string, not affected by endianness */
		memcpy(e->chunk_id, p, 8);
		e->file_offset = read32(af, p + 8);
		e->size        = read32(af, p + 12);

		/* Locate known chunks */
		if (e->file_offset == 0)
			continue;  /* unused slot */

		if ((long)(e->file_offset + e->size) > af->file_size) {
			fprintf(stderr, "Warning: chunk '%8.8s' at offset 0x%X size %u "
			        "exceeds file bounds\n", e->chunk_id, e->file_offset,
			        e->size);
			continue;
		}

		if (memcmp(e->chunk_id, "OBJ_HEAD", 8) == 0) {
			af->obj_head = af->file_data + e->file_offset;
			af->obj_head_size = e->size;
		} else if (memcmp(e->chunk_id, "OBJ_AREA", 8) == 0) {
			af->obj_area = af->file_data + e->file_offset;
			af->obj_area_size = e->size;
		} else if (memcmp(e->chunk_id, "OBJ_SYMT", 8) == 0) {
			af->obj_symt = af->file_data + e->file_offset;
			af->obj_symt_size = e->size;
		} else if (memcmp(e->chunk_id, "OBJ_STRT", 8) == 0) {
			af->obj_strt = af->file_data + e->file_offset;
			af->obj_strt_size = e->size;
		} else if (memcmp(e->chunk_id, "OBJ_IDFN", 8) == 0) {
			af->obj_idfn = af->file_data + e->file_offset;
			af->obj_idfn_size = e->size;
		}
	}

	return 0;
}

/* ===== AOF Header Parsing ===== */

static int parse_aof_header(AOFFile *af)
{
	uint32_t i;
	const uint8_t *p;

	if (!af->obj_head) {
		fprintf(stderr, "Error: OBJ_HEAD chunk not found\n");
		return -1;
	}

	if (af->obj_head_size < 24) {
		fprintf(stderr, "Error: OBJ_HEAD too small (%u bytes, need at least 24)\n",
		        af->obj_head_size);
		return -1;
	}

	p = af->obj_head;
	af->aof_header.object_file_type = read32(af, p + 0);
	af->aof_header.version_id       = read32(af, p + 4);
	af->aof_header.num_areas        = read32(af, p + 8);
	af->aof_header.num_symbols      = read32(af, p + 12);
	af->aof_header.entry_area       = read32(af, p + 16);
	af->aof_header.entry_offset     = read32(af, p + 20);

	if (af->aof_header.object_file_type != AOF_FILE_TYPE) {
		fprintf(stderr, "Error: not an AOF file (type 0x%08X, expected 0x%08X)\n",
		        af->aof_header.object_file_type, AOF_FILE_TYPE);
		return -1;
	}

	/* Parse area headers */
	{
		uint32_t expected = 24 + af->aof_header.num_areas * 20;
		if (af->obj_head_size < expected) {
			fprintf(stderr, "Error: OBJ_HEAD too small for %u areas "
			        "(%u bytes, need %u)\n",
			        af->aof_header.num_areas, af->obj_head_size, expected);
			return -1;
		}
	}

	af->area_headers = calloc(af->aof_header.num_areas, sizeof(AOFAreaHeader));
	if (!af->area_headers && af->aof_header.num_areas > 0) {
		fprintf(stderr, "Error: out of memory\n");
		return -1;
	}

	for (i = 0; i < af->aof_header.num_areas; i++) {
		p = af->obj_head + 24 + i * 20;
		af->area_headers[i].name         = read32(af, p + 0);
		af->area_headers[i].attributes   = read32(af, p + 4);
		af->area_headers[i].size         = read32(af, p + 8);
		af->area_headers[i].num_relocs   = read32(af, p + 12);
		af->area_headers[i].base_address = read32(af, p + 16);
	}

	return 0;
}

/* ===== Print Chunk Directory ===== */

static void print_chunk_directory(const AOFFile *af)
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

/* ===== Print AOF Header ===== */

static const char *version_str(uint32_t ver)
{
	switch (ver) {
	case AOF_VERSION_150: return "1.50";
	case AOF_VERSION_200: return "2.00";
	case AOF_VERSION_310: return "3.10";
	case AOF_VERSION_311: return "3.11";
	default:              return "unknown";
	}
}

static void print_area_attributes(uint32_t attr, int is_v310)
{
	uint32_t align;

	if (is_v310) {
		align = attr & AOF_ALIGN_MASK;
		printf("    Alignment:           2^%u = %u bytes\n",
		       align, 1u << align);
	}

	if (attr & AOF_ATTR_ABSOLUTE)
		printf("    [0x00000100] Absolute\n");
	if (attr & AOF_ATTR_CODE)
		printf("    [0x00000200] Code\n");
	else
		printf("    [         ] Data\n");
	if (attr & AOF_ATTR_COMDEF)
		printf("    [0x00000400] Common Definition\n");
	if (attr & AOF_ATTR_COMREF)
		printf("    [0x00000800] Common Reference\n");
	if (attr & AOF_ATTR_ZEROINIT)
		printf("    [0x00001000] Zero-Initialized\n");
	if (attr & AOF_ATTR_READONLY)
		printf("    [0x00002000] Read-Only\n");
	if (attr & AOF_ATTR_PIC)
		printf("    [0x00004000] Position Independent\n");
	if (attr & AOF_ATTR_DEBUG)
		printf("    [0x00008000] Debugging Tables\n");

	/* Code-only attributes (bits 16-22) */
	if (attr & AOF_ATTR_CODE) {
		if (attr & AOF_ATTR_APCS32)
			printf("    [0x00010000] 32-bit APCS\n");
		if (attr & AOF_ATTR_REENTRANT)
			printf("    [0x00020000] Reentrant\n");
		if (attr & AOF_ATTR_EXTENDEDFP)
			printf("    [0x00040000] Extended FP (LFM/SFM)\n");
		if (attr & AOF_ATTR_NOSTACKCHECK)
			printf("    [0x00080000] No Software Stack Check\n");
		if (attr & AOF_ATTR_THUMB)
			printf("    [0x00100000] Thumb\n");
		if (attr & AOF_ATTR_HALFWORD)
			printf("    [0x00200000] Halfword/Signed Transfer\n");
		if (attr & AOF_ATTR_INTERWORK)
			printf("    [0x00400000] ARM/Thumb Interworking\n");
	} else {
		/* Data-only attributes */
		if (attr & AOF_ATTR_BASED) {
			uint32_t reg = (attr & AOF_ATTR_BASEREG_MASK)
			               >> AOF_ATTR_BASEREG_SHIFT;
			printf("    [0x00100000] Based (base register R%u)\n", reg);
		}
		if (attr & AOF_ATTR_STUB)
			printf("    [0x00200000] Shared Library Stub\n");
	}
}

static void print_aof_header(const AOFFile *af)
{
	const AOFHeader *hdr = &af->aof_header;
	uint32_t i;
	int is_v310;

	printf("\n=== AOF Header (OBJ_HEAD) ===\n");
	print_separator();
	printf("  Object File Type:  0x%08X\n", hdr->object_file_type);
	printf("  Version ID:        %u (%s)\n",
	       hdr->version_id, version_str(hdr->version_id));
	printf("  Number of Areas:   %u\n", hdr->num_areas);
	printf("  Number of Symbols: %u\n", hdr->num_symbols);

	if (hdr->entry_area == 0) {
		printf("  Entry Point:       none (entry area = 0)\n");
	} else {
		printf("  Entry Area Index:  %u (1-origin", hdr->entry_area);
		if (hdr->entry_area <= hdr->num_areas) {
			printf(", area \"%s\"",
			       strt_lookup(af, af->area_headers[hdr->entry_area - 1].name));
		}
		printf(")\n");
		printf("  Entry Offset:      %u (0x%X)\n",
		       hdr->entry_offset, hdr->entry_offset);
	}
	print_separator();

	is_v310 = (hdr->version_id >= AOF_VERSION_310);

	/* Area headers */
	for (i = 0; i < hdr->num_areas; i++) {
		const AOFAreaHeader *ah = &af->area_headers[i];

		printf("\n  --- Area %u ---\n", i);
		printf("  Name:              \"%s\" (strt offset %u)\n",
		       strt_lookup(af, ah->name), ah->name);
		printf("  Attributes:        0x%08X\n", ah->attributes);
		print_area_attributes(ah->attributes, is_v310);
		printf("  Size:              %u bytes (0x%X)\n", ah->size, ah->size);
		printf("  Relocations:       %u\n", ah->num_relocs);
		if (ah->attributes & AOF_ATTR_ABSOLUTE)
			printf("  Base Address:      0x%08X\n", ah->base_address);
		else if (ah->base_address != 0)
			printf("  Base Address:      0x%08X (non-zero, but not absolute)\n",
			       ah->base_address);
	}
	print_separator();
}

/* ===== Print Identification ===== */

static void print_identification(const AOFFile *af)
{
	if (!af->obj_idfn || af->obj_idfn_size == 0) {
		printf("\n=== Identification (OBJ_IDFN) ===\n");
		print_separator();
		printf("  (not present)\n");
		print_separator();
		return;
	}

	printf("\n=== Identification (OBJ_IDFN) ===\n");
	print_separator();

	/* Print as a NUL-terminated string, safely */
	{
		char *buf = malloc(af->obj_idfn_size + 1);
		if (buf) {
			memcpy(buf, af->obj_idfn, af->obj_idfn_size);
			buf[af->obj_idfn_size] = '\0';
			printf("  \"%s\"\n", buf);
			free(buf);
		} else {
			printf("  (memory allocation failed)\n");
		}
	}
	print_separator();
}

/* ===== Print String Table ===== */

static void print_string_table(const AOFFile *af)
{
	uint32_t table_len, off;

	printf("\n=== String Table (OBJ_STRT) ===\n");
	print_separator();

	if (!af->obj_strt || af->obj_strt_size < 4) {
		printf("  (not present or too small)\n");
		print_separator();
		return;
	}

	table_len = read32(af, af->obj_strt);
	printf("  Table length:  %u bytes (stored), chunk size: %u bytes\n",
	       table_len, af->obj_strt_size);
	print_separator();

	off = 4;
	while (off < af->obj_strt_size && off < table_len) {
		const char *s = (const char *)(af->obj_strt + off);
		uint32_t slen = (uint32_t)strlen(s);

		printf("  [%4u] \"%s\"\n", off, s);
		off += slen + 1;
	}
	print_separator();
}

/* ===== Print Symbol Table ===== */

static const char *sym_scope_str(uint32_t attr)
{
	uint32_t scope = attr & 0x03;
	switch (scope) {
	case 0x00: return "reserved(0)";
	case 0x01: return "local";
	case 0x02: return "extern";
	case 0x03: return "global";
	default:   return "?";
	}
}

static const char *sym_type_str(uint32_t type)
{
	switch (type) {
	case 0:  return "none";
	case 9:  return "FP register";
	case 10: return "scratch register";
	case 11: return "non-scratch register";
	case 12: return "coproc register (scratch)";
	case 13: return "coproc register (preserved)";
	default: return "reserved";
	}
}

static void print_symbol_table(const AOFFile *af)
{
	uint32_t i, num_syms;
	const uint8_t *p;

	printf("\n=== Symbol Table (OBJ_SYMT) ===\n");
	print_separator();

	if (!af->obj_symt || af->obj_symt_size < 16) {
		printf("  (not present)\n");
		print_separator();
		return;
	}

	num_syms = af->aof_header.num_symbols;
	if (num_syms * 16 > af->obj_symt_size) {
		printf("  WARNING: declared %u symbols but chunk only has room for %u\n",
		       num_syms, af->obj_symt_size / 16);
		num_syms = af->obj_symt_size / 16;
	}

	printf("  %u symbol(s)\n", num_syms);
	print_separator();

	printf("  %-5s  %-30s  %-8s  %-10s  %-10s  %s\n",
	       "Index", "Name", "Scope", "Attributes", "Value", "Area");
	print_separator();

	for (i = 0; i < num_syms; i++) {
		AOFSymbol sym;
		uint32_t sym_type;
		const char *name_str, *area_str;

		p = af->obj_symt + i * 16;
		sym.name       = read32(af, p + 0);
		sym.attributes = read32(af, p + 4);
		sym.value      = read32(af, p + 8);
		sym.area_name  = read32(af, p + 12);

		name_str = strt_lookup(af, sym.name);

		/* Area name: only meaningful for defined non-absolute symbols */
		if ((sym.attributes & AOF_SYM_DEFINED) &&
		    !(sym.attributes & AOF_SYM_ABSOLUTE) &&
		    sym.area_name != 0)
			area_str = strt_lookup(af, sym.area_name);
		else
			area_str = "";

		printf("  %-5u  %-30s  %-8s  0x%08X  0x%08X  %s\n",
		       i, name_str, sym_scope_str(sym.attributes),
		       sym.attributes, sym.value, area_str);

		/* Print detailed attribute flags */
		{
			int first = 1;
			printf("         Flags:");

			if (sym.attributes & AOF_SYM_ABSOLUTE) {
				printf("%s Absolute", first ? "" : ",");
				first = 0;
			}
			if (sym.attributes & AOF_SYM_CASEINSENS) {
				printf("%s CaseInsensitive", first ? "" : ",");
				first = 0;
			}
			if (sym.attributes & AOF_SYM_WEAK) {
				printf("%s Weak", first ? "" : ",");
				first = 0;
			}
			if (sym.attributes & AOF_SYM_STRONG) {
				printf("%s Strong", first ? "" : ",");
				first = 0;
			}
			if (sym.attributes & AOF_SYM_COMMON) {
				printf("%s Common (size=%u)", first ? "" : ",", sym.value);
				first = 0;
			}
			if (sym.attributes & AOF_SYM_CODEDATUM) {
				printf("%s CodeDatum", first ? "" : ",");
				first = 0;
			}
			if (sym.attributes & AOF_SYM_FPARGS) {
				printf("%s FP-args-in-FP-regs", first ? "" : ",");
				first = 0;
			}
			if (sym.attributes & AOF_SYM_LEAF) {
				printf("%s Leaf", first ? "" : ",");
				first = 0;
			}
			if (sym.attributes & AOF_SYM_THUMB) {
				printf("%s Thumb", first ? "" : ",");
				first = 0;
			}

			sym_type = (sym.attributes & AOF_SYM_TYPE_MASK) >> AOF_SYM_TYPE_SHIFT;
			if (sym_type != 0)
				printf("%s Type=%s(%u)", first ? "" : ",",
				       sym_type_str(sym_type), sym_type);

			if (first)
				printf(" (none)");
			printf("\n");
		}
	}
	print_separator();
}

/* ===== Print Relocation Directives ===== */

static const char *ft_str(uint32_t ft)
{
	switch (ft) {
	case AOF_FT_BYTE:        return "Byte";
	case AOF_FT_HALFWORD:    return "Halfword";
	case AOF_FT_WORD:        return "Word";
	case AOF_FT_INSTRUCTION: return "Instruction";
	default:                 return "?";
	}
}

static void print_relocations(const AOFFile *af)
{
	uint32_t i;
	const uint8_t *area_ptr;

	printf("\n=== Relocation Directives (OBJ_AREA) ===\n");
	print_separator();

	if (!af->obj_area) {
		printf("  (OBJ_AREA not present)\n");
		print_separator();
		return;
	}

	area_ptr = af->obj_area;

	for (i = 0; i < af->aof_header.num_areas; i++) {
		const AOFAreaHeader *ah = &af->area_headers[i];
		uint32_t data_size;
		uint32_t j;
		const uint8_t *reloc_base;

		/* Zero-initialized areas have no data in OBJ_AREA */
		data_size = (ah->attributes & AOF_ATTR_ZEROINIT) ? 0 : ah->size;

		printf("\n  --- Area %u: \"%s\" (%u relocation(s)) ---\n",
		       i, strt_lookup(af, ah->name), ah->num_relocs);

		if (ah->num_relocs == 0) {
			area_ptr += data_size;
			continue;
		}

		reloc_base = area_ptr + data_size;

		/* Check bounds */
		{
			uint32_t needed = data_size + ah->num_relocs * 8;
			uint32_t offset_in_area = (uint32_t)(area_ptr - af->obj_area);
			if (offset_in_area + needed > af->obj_area_size) {
				printf("  WARNING: relocations extend beyond OBJ_AREA chunk\n");
				area_ptr += data_size;
				continue;
			}
		}

		printf("  %-5s  %-10s  %-4s  %-12s  %-4s  %-4s  %-4s  %-3s  %s\n",
		       "Index", "Offset", "FT", "Type", "R", "A", "B", "II", "SID / Target");
		print_separator();

		for (j = 0; j < ah->num_relocs; j++) {
			const uint8_t *rp = reloc_base + j * 8;
			uint32_t r_offset = read32(af, rp + 0);
			uint32_t r_flags  = read32(af, rp + 4);

			uint32_t sid  = r_flags & AOF_RELOC_SID_MASK;
			uint32_t ft   = (r_flags & AOF_RELOC_FT_MASK) >> AOF_RELOC_FT_SHIFT;
			int pcrel     = (r_flags & AOF_RELOC_R) != 0;
			int is_symbol = (r_flags & AOF_RELOC_A) != 0;
			int is_based  = (r_flags & AOF_RELOC_B) != 0;
			uint32_t ii   = (r_flags & AOF_RELOC_II_MASK) >> AOF_RELOC_II_SHIFT;

			const char *target;
			char target_buf[128];

			if (is_symbol) {
				/* SID is symbol index */
				if (sid < af->aof_header.num_symbols) {
					uint32_t sym_name = read32(af,
						af->obj_symt + sid * 16);
					target = strt_lookup(af, sym_name);
				} else {
					snprintf(target_buf, sizeof(target_buf),
					         "sym[%u] (out of range)", sid);
					target = target_buf;
				}
			} else {
				/* SID is area index */
				if (sid < af->aof_header.num_areas) {
					target = strt_lookup(af, af->area_headers[sid].name);
				} else {
					snprintf(target_buf, sizeof(target_buf),
					         "area[%u] (out of range)", sid);
					target = target_buf;
				}
			}

			printf("  %-5u  0x%08X  %-4s  %-12s  %-4s  %-4s  %-4s  %-3u  %s\n",
			       j, r_offset, ft_str(ft),
			       is_symbol ? "Symbol" : "Area",
			       pcrel ? "yes" : "no",
			       is_symbol ? "yes" : "no",
			       is_based ? "yes" : "no",
			       ii, target);

			/* For instruction relocations, note Thumb */
			if (ft == AOF_FT_INSTRUCTION && (r_offset & 1))
				printf("         ^ Thumb instruction relocation\n");
		}

		area_ptr += data_size + ah->num_relocs * 8;
	}
	print_separator();
}

/* ===== Print Area Data Hex Dump ===== */

static void print_area_data(const AOFFile *af)
{
	uint32_t i;
	const uint8_t *area_ptr;

	printf("\n=== Area Data (OBJ_AREA) ===\n");
	print_separator();

	if (!af->obj_area) {
		printf("  (OBJ_AREA not present)\n");
		print_separator();
		return;
	}

	area_ptr = af->obj_area;

	for (i = 0; i < af->aof_header.num_areas; i++) {
		const AOFAreaHeader *ah = &af->area_headers[i];
		uint32_t data_size;

		data_size = (ah->attributes & AOF_ATTR_ZEROINIT) ? 0 : ah->size;

		printf("\n  --- Area %u: \"%s\" (%u bytes) ---\n",
		       i, strt_lookup(af, ah->name), ah->size);

		if (ah->attributes & AOF_ATTR_ZEROINIT) {
			printf("  (zero-initialized, no data in file)\n");
		} else if (data_size == 0) {
			printf("  (empty)\n");
		} else {
			/* Check bounds */
			uint32_t offset_in_area = (uint32_t)(area_ptr - af->obj_area);
			if (offset_in_area + data_size > af->obj_area_size) {
				printf("  WARNING: area data extends beyond OBJ_AREA chunk\n");
			} else {
				uint32_t dump_size = data_size;
				if (dump_size > 256) {
					printf("  (showing first 256 of %u bytes)\n", data_size);
					dump_size = 256;
				}
				hexdump(area_ptr, dump_size, 0);
				if (data_size > 256)
					printf("  ... (%u more bytes)\n", data_size - 256);
			}
		}

		area_ptr += data_size + ah->num_relocs * 8;
	}
	print_separator();
}

/* ===== Validation ===== */

static int validate_aof(const AOFFile *af)
{
	int warnings = 0;
	uint32_t i;
	const AOFHeader *hdr = &af->aof_header;

	printf("\n=== Validation ===\n");
	print_separator();

	/* Chunk file level */
	if (af->cf_header.num_chunks > af->cf_header.max_chunks) {
		printf("  WARNING: numChunks (%u) > maxChunks (%u)\n",
		       af->cf_header.num_chunks, af->cf_header.max_chunks);
		warnings++;
	}

	/* Required chunks */
	if (!af->obj_head) {
		printf("  ERROR: OBJ_HEAD chunk missing (required)\n");
		warnings++;
	}
	if (!af->obj_area) {
		printf("  ERROR: OBJ_AREA chunk missing (required)\n");
		warnings++;
	}

	/* AOF header level */
	if (hdr->version_id != AOF_VERSION_150 &&
	    hdr->version_id != AOF_VERSION_200 &&
	    hdr->version_id != AOF_VERSION_310 &&
	    hdr->version_id != AOF_VERSION_311) {
		printf("  WARNING: unrecognized version ID %u\n", hdr->version_id);
		warnings++;
	}

	if (hdr->num_areas == 0) {
		printf("  WARNING: number of areas is 0\n");
		warnings++;
	}
	if (hdr->num_areas > 255) {
		printf("  NOTE: number of areas (%u) exceeds recommended limit of 255\n",
		       hdr->num_areas);
	}

	if (hdr->entry_area > hdr->num_areas) {
		printf("  WARNING: entry area index %u out of range (1..%u)\n",
		       hdr->entry_area, hdr->num_areas);
		warnings++;
	}

	if (hdr->entry_area > 0 && hdr->entry_area <= hdr->num_areas) {
		const AOFAreaHeader *ea = &af->area_headers[hdr->entry_area - 1];
		if (hdr->entry_offset >= ea->size) {
			printf("  WARNING: entry offset %u >= entry area size %u\n",
			       hdr->entry_offset, ea->size);
			warnings++;
		}
	}

	/* Area level validation */
	for (i = 0; i < hdr->num_areas; i++) {
		const AOFAreaHeader *ah = &af->area_headers[i];
		uint32_t align;

		if (ah->size & 3) {
			printf("  WARNING: area %u (\"%s\") size %u is not a multiple of 4\n",
			       i, strt_lookup(af, ah->name), ah->size);
			warnings++;
		}

		if ((ah->attributes & AOF_ATTR_ZEROINIT) &&
		    (ah->attributes & AOF_ATTR_READONLY)) {
			printf("  WARNING: area %u (\"%s\") has both Zero-Init and Read-Only set\n",
			       i, strt_lookup(af, ah->name));
			warnings++;
		}

		if ((ah->attributes & AOF_ATTR_ABSOLUTE) && ah->base_address == 0) {
			printf("  NOTE: area %u (\"%s\") is Absolute with base address 0\n",
			       i, strt_lookup(af, ah->name));
		}

		if (hdr->version_id >= AOF_VERSION_310) {
			align = ah->attributes & AOF_ALIGN_MASK;
			if (align < 2) {
				printf("  WARNING: area %u (\"%s\") alignment %u < minimum 2\n",
				       i, strt_lookup(af, ah->name), align);
				warnings++;
			}
		}
	}

	/* Symbol level validation */
	if (af->obj_symt) {
		uint32_t num_syms = hdr->num_symbols;
		if (num_syms * 16 > af->obj_symt_size)
			num_syms = af->obj_symt_size / 16;

		for (i = 0; i < num_syms; i++) {
			const uint8_t *p = af->obj_symt + i * 16;
			uint32_t attr = read32(af, p + 4);
			uint32_t name_off = read32(af, p + 0);

			/* Check for reserved scope (bits 0-1 both 0) */
			if ((attr & 0x03) == 0) {
				printf("  WARNING: symbol %u has reserved scope (bits 0-1 = 0)\n", i);
				warnings++;
			}

			/* Check string offset bounds */
			if (af->obj_strt && (name_off < 4 || name_off >= af->obj_strt_size)) {
				printf("  WARNING: symbol %u name offset %u out of string table bounds\n",
				       i, name_off);
				warnings++;
			}
		}
	}

	if (warnings == 0)
		printf("  No issues found.\n");
	else
		printf("  %d warning(s)/error(s) found.\n", warnings);

	print_separator();
	return warnings;
}

/* ===== Usage ===== */

static void usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s [options] <aof-file>\n"
		"\n"
		"Options:\n"
		"  -h, --help           Show this help message\n"
		"  -x, --hex            Include hex dump of area data\n"
		"  -R, --no-relocs      Suppress relocation directives\n"
		"  -S, --no-symbols     Suppress symbol table\n"
		"  -b, --big-endian     Force big-endian interpretation\n"
		"  -l, --little-endian  Force little-endian interpretation\n"
		"  -q, --quiet          Skip validation checks\n"
		"\n"
		"Prints detailed information about an ARM Object Format (AOF) file,\n"
		"including chunk directory, area declarations, symbol table,\n"
		"relocation directives, and tool identification.\n",
		prog);
}

/* ===== Main ===== */

int main(int argc, char *argv[])
{
	const char *filename = NULL;
	Options opts;
	AOFFile af;
	FILE *f;
	int i;

	memset(&opts, 0, sizeof(opts));
	opts.show_relocs  = 1;
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
		} else if (strcmp(argv[i], "-R") == 0 || strcmp(argv[i], "--no-relocs") == 0) {
			opts.show_relocs = 0;
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

	/* Override endianness if forced */
	if (opts.force_endian >= 0) {
		/* Pre-set before parse_chunk_file's own detection */
		/* We'll handle this after detection by overriding */
	}

	/* Parse chunk file structure */
	if (parse_chunk_file(&af) != 0) {
		free(af.file_data);
		free(af.chunks);
		return 1;
	}

	/* Override endianness if forced */
	if (opts.force_endian >= 0)
		af.swap_endian = opts.force_endian;

	/* Parse AOF header */
	if (parse_aof_header(&af) != 0) {
		free(af.file_data);
		free(af.chunks);
		free(af.area_headers);
		return 1;
	}

	/* Print banner */
	printf("aofdump: ARM Object Format (AOF) File Analyzer\n");
	print_separator();
	printf("  File:     %s\n", filename);
	printf("  Size:     %ld bytes\n", af.file_size);
	printf("  Endian:   %s%s\n",
	       af.swap_endian ? "big-endian" : "little-endian",
	       opts.force_endian >= 0 ? " (forced)" : " (detected)");
	print_separator();

	/* Print sections */
	print_chunk_directory(&af);
	print_aof_header(&af);
	print_identification(&af);
	print_string_table(&af);

	if (opts.show_symbols)
		print_symbol_table(&af);

	if (opts.show_relocs)
		print_relocations(&af);

	if (opts.show_hex)
		print_area_data(&af);

	if (!opts.quiet)
		validate_aof(&af);

	printf("\n");

	/* Cleanup */
	free(af.area_headers);
	free(af.chunks);
	free(af.file_data);

	return 0;
}

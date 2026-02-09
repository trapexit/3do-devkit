/*
 * aifinfo - ARM Image Format (AIF) file information tool
 *
 * Parses and prints details of AIF executable images as described in
 * the ARM Image Format specification, including 3DO platform extensions
 * and Extended AIF region headers.
 *
 * References:
 *   - ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
 *   - PLG-AIF Issue 1.00: RISC OS Application Image Format (Acorn, 1989)
 *   - 3DO Portfolio OS SDK Documentation (aif.h, Revision 1.21, 1994)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

/* ===== AIF Constants ===== */
#define AIF_HEADER_SIZE         128
#define AIF_CODE_OFFSET         0x80

#define AIF_NOP                 0xE1A00000  /* MOV R0, R0 */
#define AIF_NOP_LEGACY          0xFB000000  /* BLNV 0 (pre-1989) */
#define AIF_BL_MASK             0xFF000000
#define AIF_BL_BASE             0xEB000000  /* BL instruction base */
#define AIF_SWI_EXIT            0xEF000011  /* SWI 0x11 (OS_Exit) */
#define AIF_BRANCH_SELF         0xEAFFFFFE  /* B . (infinite loop) */
#define AIF_RELOC_TERMINATOR    0xFFFFFFFF

/* Debug types */
#define AIF_DEBUG_NONE          0
#define AIF_DEBUG_LOW           1
#define AIF_DEBUG_SOURCE        2
#define AIF_DEBUG_BOTH          3

/* Address modes */
#define AIF_MODE_OLD_26BIT      0
#define AIF_MODE_26BIT          26
#define AIF_MODE_32BIT          32
#define AIF_SEPARATE_DATA_BASE  0x00000100

/* 3DO extensions */
#define AIF_3DOHEADER           0x40000000
#define AIF_3DO_STACK_MASK      0x00FFFFFF
#define _3DO_PERMSTACK          0x01
#define _3DO_PRIVILEGE          0x02
#define _3DO_USERAPP            0x04
#define _3DO_LUNCH              0x08
#define _3DO_NORESETOK          0x10
#define _3DO_DATADISCOK         0x20

/* 3DO debugger constants */
#define PORTFOLIO_DEBUG_CLEAR   0xE1A02002  /* MOV R2, R2 */
#define PORTFOLIO_DEBUG_TASK    0xEF00010A  /* SWI 0x010A */
#define PORTFOLIO_DEBUG_UNSET   0xE1A00000  /* MOV R0, R0 */

/* DDE debugger */
#define SWI_DDE_DEBUG           0xEF041D41

/* Extended AIF region header size */
#define AIF_REGION_HEADER_SIZE  44

/* 3DO epoch: 1993-01-01 00:00:00 UTC */
#define EPOCH_3DO               725846400

/* ===== Structures ===== */

typedef struct AIFHeader {
	uint32_t bl_decompress;       /* 0x00 */
	uint32_t bl_self_reloc;       /* 0x04 */
	uint32_t bl_zero_init;        /* 0x08 */
	uint32_t bl_entry;            /* 0x0C */
	uint32_t swi_exit;            /* 0x10 */
	uint32_t ro_size;             /* 0x14 */
	uint32_t rw_size;             /* 0x18 */
	uint32_t debug_size;          /* 0x1C */
	uint32_t zi_size;             /* 0x20 */
	uint32_t debug_type;          /* 0x24 */
	uint32_t image_base;          /* 0x28 */
	uint32_t workspace;           /* 0x2C */
	uint32_t address_mode;        /* 0x30 */
	uint32_t data_base;           /* 0x34 */
	uint32_t reserved[2];         /* 0x38, 0x3C */
	uint32_t debug_init;          /* 0x40 */
	uint32_t zero_init_code[15];  /* 0x44 - 0x7C */
} AIFHeader;

typedef struct AIFRegionHeader {
	uint32_t next_offset;
	uint32_t load_address;
	uint32_t size;
	char     name[32];
} AIFRegionHeader;

typedef struct _3DOBinHeader {
	uint32_t item_node[4];        /* 0x00: ItemNode */
	uint8_t  flags;               /* 0x10 */
	uint8_t  os_version;          /* 0x11 */
	uint8_t  os_revision;         /* 0x12 */
	uint8_t  reserved;            /* 0x13 */
	uint32_t stack;               /* 0x14 */
	uint32_t freespace;           /* 0x18 */
	uint32_t signature;           /* 0x1C */
	uint32_t signature_len;       /* 0x20 */
	uint32_t max_usecs;           /* 0x24 */
	uint32_t reserved0;           /* 0x28 */
	char     name[32];            /* 0x2C */
	uint32_t time;                /* 0x4C */
	uint32_t reserved1[7];        /* 0x50 */
} _3DOBinHeader;

/* ===== Endianness Utilities ===== */

static int g_swap_endian = 0;

static uint32_t swap32(uint32_t v)
{
	return ((v >> 24) & 0x000000FF) |
	       ((v >>  8) & 0x0000FF00) |
	       ((v <<  8) & 0x00FF0000) |
	       ((v << 24) & 0xFF000000);
}

static uint32_t fix32(uint32_t v)
{
	return g_swap_endian ? swap32(v) : v;
}

static void fix_header_endian(AIFHeader *hdr)
{
	uint32_t *words = (uint32_t *)hdr;
	int i;

	if (!g_swap_endian)
		return;

	for (i = 0; i < 32; i++)
		words[i] = swap32(words[i]);
}

/* ===== Detection Helpers ===== */

static int is_nop(uint32_t word)
{
	return word == AIF_NOP || word == AIF_NOP_LEGACY;
}

static int is_bl(uint32_t word)
{
	return (word & AIF_BL_MASK) == AIF_BL_BASE;
}

static int detect_endianness(const uint8_t *raw)
{
	uint32_t word0_le, word0_be;

	/* Read first word as little-endian */
	word0_le = (uint32_t)raw[0]       | ((uint32_t)raw[1] << 8) |
	           ((uint32_t)raw[2] << 16) | ((uint32_t)raw[3] << 24);

	/* Read first word as big-endian */
	word0_be = ((uint32_t)raw[0] << 24) | ((uint32_t)raw[1] << 16) |
	           ((uint32_t)raw[2] << 8)  |  (uint32_t)raw[3];

	/*
	 * Offset 0x00 should be NOP (0xE1A00000) or a BL (0xEB......).
	 * Test which endianness produces a valid instruction.
	 */
	if (is_nop(word0_le) || is_bl(word0_le))
		return 0; /* little-endian, no swap needed */
	if (is_nop(word0_be) || is_bl(word0_be))
		return 1; /* big-endian, swap needed */

	/* Also try legacy NOP BLNV 0 (0xFB000000) */
	if (word0_le == AIF_NOP_LEGACY)
		return 0;
	if (word0_be == AIF_NOP_LEGACY)
		return 1;

	/* Default: assume little-endian (most common for ARM) */
	return 0;
}

/* ===== ARM Instruction Decoding ===== */

static const char *decode_condition(uint32_t cond)
{
	static const char *conds[] = {
		"EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC",
		"HI", "LS", "GE", "LT", "GT", "LE", "",   "NV"
	};
	return conds[cond & 0xF];
}

/*
 * Decode a BL instruction's target offset.
 * The offset field is a 24-bit signed value, shifted left 2 bits,
 * added to PC+8.
 * Returns the offset relative to the instruction's own address.
 */
static int32_t bl_offset(uint32_t instr)
{
	int32_t off = (int32_t)(instr & 0x00FFFFFF);
	/* Sign-extend from 24 bits */
	if (off & 0x00800000)
		off |= (int32_t)0xFF000000;
	/* Shift left 2, then add 8 for pipeline offset */
	return (off << 2) + 8;
}

static void decode_instruction(uint32_t word, uint32_t offset, char *buf, size_t bufsz)
{
	if (word == AIF_NOP) {
		snprintf(buf, bufsz, "NOP (MOV R0, R0)");
	} else if (word == AIF_NOP_LEGACY) {
		snprintf(buf, bufsz, "NOP (BLNV 0) [legacy pre-1989]");
	} else if (word == AIF_BRANCH_SELF) {
		snprintf(buf, bufsz, "B . (infinite loop / halt)");
	} else if ((word & AIF_BL_MASK) == AIF_BL_BASE) {
		int32_t off = bl_offset(word);
		uint32_t target = offset + off;
		snprintf(buf, bufsz, "BL 0x%08X (offset %+d from 0x%02X)",
		         target, off, offset);
	} else if ((word & 0x0F000000) == 0x0F000000) {
		/* SWI instruction */
		uint32_t cond = (word >> 28) & 0xF;
		uint32_t swi_num = word & 0x00FFFFFF;
		snprintf(buf, bufsz, "SWI%s 0x%06X", decode_condition(cond), swi_num);
	} else if (word == PORTFOLIO_DEBUG_CLEAR) {
		snprintf(buf, bufsz, "MOV R2, R2 (3DO debug cleared)");
	} else {
		/* Generic ARM instruction */
		uint32_t cond = (word >> 28) & 0xF;
		snprintf(buf, bufsz, "ARM instruction (cond=%s) 0x%08X",
		         decode_condition(cond), word);
	}
}

/* ===== Printing Functions ===== */

static const char *debug_type_str(uint32_t dt)
{
	switch (dt) {
	case AIF_DEBUG_NONE:   return "None";
	case AIF_DEBUG_LOW:    return "Low-level debugging data";
	case AIF_DEBUG_SOURCE: return "Source-level (ASD) debugging data";
	case AIF_DEBUG_BOTH:   return "Both low-level and source-level";
	default:               return "Unknown/Reserved";
	}
}

static const char *address_mode_str(uint32_t mode)
{
	switch (mode & 0xFF) {
	case AIF_MODE_OLD_26BIT: return "Old-style 26-bit (pre-APCS-3)";
	case AIF_MODE_26BIT:     return "26-bit ARM mode";
	case AIF_MODE_32BIT:     return "32-bit ARM mode";
	default:                 return "Unknown";
	}
}

static void print_separator(void)
{
	printf("--------------------------------------------------------------\n");
}

static void print_aif_header(const AIFHeader *hdr, long file_size)
{
	char instr_buf[128];
	int is_exec, is_ext;
	uint32_t ro_code_size;

	/* Determine variant */
	is_exec = (hdr->bl_entry >> 24) == 0xEB;
	is_ext  = (hdr->reserved[0] != 0) && !is_exec;

	printf("=== AIF Header (128 bytes) ===\n");
	print_separator();

	/* Variant */
	if (is_exec) {
		printf("  Variant:                     Executable AIF\n");
	} else if (is_ext) {
		printf("  Variant:                     Extended AIF (scatter-loaded)\n");
	} else {
		printf("  Variant:                     Non-Executable AIF\n");
	}

	printf("  File size:                   %ld bytes (0x%lX)\n",
	       file_size, file_size);
	print_separator();

	/* Word 0: BL DecompressCode / NOP */
	decode_instruction(hdr->bl_decompress, 0x00, instr_buf, sizeof(instr_buf));
	printf("  [0x00] Decompress:           0x%08X  %s\n",
	       hdr->bl_decompress, instr_buf);
	if (!is_nop(hdr->bl_decompress) && is_bl(hdr->bl_decompress))
		printf("         -> Image is COMPRESSED\n");
	else
		printf("         -> Image is uncompressed\n");

	/* Word 1: BL SelfRelocCode / NOP */
	decode_instruction(hdr->bl_self_reloc, 0x04, instr_buf, sizeof(instr_buf));
	printf("  [0x04] Self-Relocation:      0x%08X  %s\n",
	       hdr->bl_self_reloc, instr_buf);
	if (!is_nop(hdr->bl_self_reloc) && is_bl(hdr->bl_self_reloc))
		printf("         -> Image is SELF-RELOCATING\n");
	else
		printf("         -> Image is not self-relocating\n");

	/* Word 2: BL ZeroInit / NOP */
	decode_instruction(hdr->bl_zero_init, 0x08, instr_buf, sizeof(instr_buf));
	printf("  [0x08] Zero-Init:            0x%08X  %s\n",
	       hdr->bl_zero_init, instr_buf);
	if (!is_nop(hdr->bl_zero_init) && is_bl(hdr->bl_zero_init))
		printf("         -> Image has zero-init code\n");
	else
		printf("         -> No zero-init code\n");

	/* Word 3: BL EntryPoint / Offset */
	decode_instruction(hdr->bl_entry, 0x0C, instr_buf, sizeof(instr_buf));
	printf("  [0x0C] Entry Point:          0x%08X  %s\n",
	       hdr->bl_entry, instr_buf);
	if (is_exec) {
		int32_t off = bl_offset(hdr->bl_entry);
		uint32_t target = 0x0C + (uint32_t)off;
		printf("         -> Entry at file offset 0x%X (image base + 0x%X)\n",
		       target, target);
	} else {
		uint32_t entry_off = hdr->bl_entry & 0x00FFFFFF;
		printf("         -> Entry offset from image base: 0x%X (%u bytes)\n",
		       entry_off, entry_off);
	}

	/* Word 4: Exit instruction */
	decode_instruction(hdr->swi_exit, 0x10, instr_buf, sizeof(instr_buf));
	printf("  [0x10] Exit Instruction:     0x%08X  %s\n",
	       hdr->swi_exit, instr_buf);

	print_separator();
	printf("  --- Image Sizes ---\n");

	/* ReadOnly size */
	printf("  [0x14] ReadOnly (RO) Size:   %u bytes (0x%X)\n",
	       hdr->ro_size, hdr->ro_size);
	if (is_exec) {
		ro_code_size = hdr->ro_size > AIF_HEADER_SIZE ?
		               hdr->ro_size - AIF_HEADER_SIZE : 0;
		printf("         (includes 128-byte header; code = %u bytes)\n",
		       ro_code_size);
	}

	/* ReadWrite size */
	printf("  [0x18] ReadWrite (RW) Size:  %u bytes (0x%X)\n",
	       hdr->rw_size, hdr->rw_size);

	/* Debug size */
	printf("  [0x1C] Debug Size:           %u bytes (0x%X)\n",
	       hdr->debug_size, hdr->debug_size);

	/* Zero-Init size */
	printf("  [0x20] Zero-Init (ZI) Size:  %u bytes (0x%X)\n",
	       hdr->zi_size, hdr->zi_size);
	printf("         (not stored in file; created at runtime)\n");

	/* Total runtime memory */
	{
		uint64_t runtime = (uint64_t)hdr->ro_size + hdr->rw_size + hdr->zi_size;
		printf("\n  Runtime memory footprint:    %llu bytes (RO+RW+ZI)\n",
		       (unsigned long long)runtime);
		printf("  File data footprint:         %u bytes (RO+RW",
		       hdr->ro_size + hdr->rw_size);
		if (hdr->debug_size > 0)
			printf("+Debug=%u", hdr->ro_size + hdr->rw_size + hdr->debug_size);
		printf(")\n");
	}

	print_separator();
	printf("  --- Debug Information ---\n");

	printf("  [0x24] Debug Type:           %u - %s\n",
	       hdr->debug_type, debug_type_str(hdr->debug_type));

	if (hdr->debug_size > 0) {
		uint32_t dbg_low_bits = hdr->debug_size & 0xF;
		uint32_t dbg_section_size = hdr->debug_size >> 4;
		printf("         Debug size low bits:  0x%X (debug type hint)\n",
		       dbg_low_bits);
		if (dbg_section_size > 0)
			printf("         Low-level section:    %u bytes\n",
			       dbg_section_size << 4);
	}

	print_separator();
	printf("  --- Addresses and Linking ---\n");

	printf("  [0x28] Image Base:           0x%08X\n", hdr->image_base);
	printf("  [0x30] Address Mode:         0x%08X - %s\n",
	       hdr->address_mode, address_mode_str(hdr->address_mode));

	if (hdr->address_mode & AIF_SEPARATE_DATA_BASE) {
		printf("         Separate data base:   YES\n");
		printf("  [0x34] Data Base:            0x%08X\n", hdr->data_base);
	} else {
		printf("         Separate data base:   No\n");
		printf("  [0x34] Data Base:            0x%08X (unused)\n",
		       hdr->data_base);
	}

	/* Computed addresses */
	if (is_exec) {
		uint32_t ro_base = hdr->image_base;
		uint32_t rw_base = ro_base + hdr->ro_size;
		uint32_t zi_base = rw_base + hdr->rw_size;

		if (hdr->address_mode & AIF_SEPARATE_DATA_BASE)
			rw_base = hdr->data_base;

		printf("\n  --- Computed Section Addresses ---\n");
		printf("  Image$$RO$$Base:             0x%08X\n", ro_base);
		printf("  Image$$RW$$Base:             0x%08X\n", rw_base);
		printf("  Image$$ZI$$Base:             0x%08X\n", zi_base);
		printf("  Image$$RW$$Limit:            0x%08X\n",
		       zi_base + hdr->zi_size);
	}

	print_separator();
	printf("  --- Workspace / 3DO Stack ---\n");

	printf("  [0x2C] Workspace:            0x%08X\n", hdr->workspace);
	if (hdr->workspace & AIF_3DOHEADER) {
		printf("         3DO header flag:      SET (bit 30)\n");
		printf("         -> 128-byte 3DOBinHeader follows at offset 0x80\n");
	}
	{
		uint32_t stack_val = hdr->workspace & AIF_3DO_STACK_MASK;
		if (stack_val > 0)
			printf("         Stack size (bits 0-23): %u bytes (0x%X)\n",
			       stack_val, stack_val);
	}
	if ((hdr->workspace & ~AIF_3DOHEADER & ~AIF_3DO_STACK_MASK) == 0 &&
	    !(hdr->workspace & AIF_3DOHEADER)) {
		if (hdr->workspace == 0)
			printf("         Image does not self-move (relocate in place only)\n");
		else
			printf("         Min workspace for self-move: %u bytes\n",
			       hdr->workspace);
	}

	print_separator();
	printf("  --- Reserved / Extended AIF ---\n");

	printf("  [0x38] Reserved[0]:          0x%08X", hdr->reserved[0]);
	if (hdr->reserved[0] != 0 && !is_exec)
		printf("  -> Extended AIF: first region at file offset 0x%X",
		       hdr->reserved[0]);
	else if (hdr->reserved[0] != 0)
		printf("  -> 3DO MD4 data size / non-zero");
	printf("\n");

	printf("  [0x3C] Reserved[1]:          0x%08X", hdr->reserved[1]);
	if (hdr->reserved[1] != 0)
		printf("  -> Non-zero (unexpected)");
	printf("\n");

	print_separator();
	printf("  --- Debug Init ---\n");

	decode_instruction(hdr->debug_init, 0x40, instr_buf, sizeof(instr_buf));
	printf("  [0x40] Debug Init:           0x%08X  %s\n",
	       hdr->debug_init, instr_buf);
	if (hdr->debug_init == PORTFOLIO_DEBUG_TASK)
		printf("         -> 3DO Portfolio debugger alert\n");
	else if (hdr->debug_init == SWI_DDE_DEBUG)
		printf("         -> RISC OS DDE debugger alert\n");
	else if (hdr->debug_init == PORTFOLIO_DEBUG_CLEAR)
		printf("         -> 3DO debug cleared (MOV R2, R2)\n");
	else if (is_nop(hdr->debug_init))
		printf("         -> No debugger alert\n");

	print_separator();
	printf("  --- Zero-Init Subroutine (offsets 0x44-0x7C) ---\n");
	{
		int i;
		int all_zero = 1;
		for (i = 0; i < 15; i++) {
			if (hdr->zero_init_code[i] != 0) {
				all_zero = 0;
				break;
			}
		}
		if (all_zero) {
			printf("  (all zero / empty)\n");
		} else {
			for (i = 0; i < 15; i++) {
				uint32_t off = 0x44 + (i * 4);
				decode_instruction(hdr->zero_init_code[i], off,
				                   instr_buf, sizeof(instr_buf));
				printf("  [0x%02X] Word %2d:              0x%08X  %s\n",
				       off, i, hdr->zero_init_code[i], instr_buf);
			}
		}
	}
}

static void print_3do_header(const _3DOBinHeader *hdr3do)
{
	printf("\n=== 3DO Binary Header (128 bytes at offset 0x80) ===\n");
	print_separator();

	printf("  --- ItemNode ---\n");
	printf("  [0x00] ItemNode[0]:          0x%08X\n", hdr3do->item_node[0]);
	printf("  [0x04] ItemNode[1]:          0x%08X\n", hdr3do->item_node[1]);
	printf("  [0x08] ItemNode[2]:          0x%08X\n", hdr3do->item_node[2]);
	printf("  [0x0C] ItemNode[3]:          0x%08X\n", hdr3do->item_node[3]);

	print_separator();
	printf("  --- 3DO Application Info ---\n");

	printf("  [0x10] Flags:                0x%02X\n", hdr3do->flags);
	if (hdr3do->flags & _3DO_PERMSTACK)
		printf("         [0x01] PERMSTACK:     Permanent stack\n");
	if (hdr3do->flags & _3DO_PRIVILEGE)
		printf("         [0x02] PRIVILEGE:     Privileged application\n");
	if (hdr3do->flags & _3DO_USERAPP)
		printf("         [0x04] USERAPP:       User app (requires authentication)\n");
	if (hdr3do->flags & _3DO_LUNCH)
		printf("         [0x08] LUNCH:         OS launched, chips initialized\n");
	if (hdr3do->flags & _3DO_NORESETOK)
		printf("         [0x10] NORESETOK:     Keep running on empty drawer\n");
	if (hdr3do->flags & _3DO_DATADISCOK)
		printf("         [0x20] DATADISCOK:    Accept data discs\n");

	printf("  [0x11] OS Version:           %u\n", hdr3do->os_version);
	printf("  [0x12] OS Revision:          %u\n", hdr3do->os_revision);
	printf("  [0x13] Reserved:             0x%02X", hdr3do->reserved);
	if (hdr3do->reserved != 0)
		printf(" (expected 0)");
	printf("\n");

	printf("  [0x14] Stack:                %u bytes (0x%X)\n",
	       hdr3do->stack, hdr3do->stack);
	printf("  [0x18] FreeSpace:            %u bytes (0x%X)\n",
	       hdr3do->freespace, hdr3do->freespace);
	printf("  [0x1C] Signature Offset:     0x%08X\n", hdr3do->signature);
	printf("  [0x20] Signature Length:     %u bytes\n", hdr3do->signature_len);
	printf("  [0x24] Max USecs:            %u", hdr3do->max_usecs);
	if (hdr3do->max_usecs > 0)
		printf(" (%u.%03u ms)", hdr3do->max_usecs / 1000,
		       hdr3do->max_usecs % 1000);
	printf("\n");

	printf("  [0x28] Reserved0:            0x%08X", hdr3do->reserved0);
	if (hdr3do->reserved0 != 0)
		printf(" (expected 0)");
	printf("\n");

	/* Name - ensure null termination for printing */
	{
		char name_buf[33];
		memcpy(name_buf, hdr3do->name, 32);
		name_buf[32] = '\0';
		printf("  [0x2C] Name:                 \"%s\"\n", name_buf);
	}

	/* Time */
	printf("  [0x4C] Time:                 %u", hdr3do->time);
	if (hdr3do->time > 0) {
		time_t t = (time_t)hdr3do->time + EPOCH_3DO;
		struct tm *tm = gmtime(&t);
		if (tm) {
			char tbuf[64];
			strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S UTC", tm);
			printf(" (%s)", tbuf);
		}
		printf("  [seconds since 1993-01-01]");
	}
	printf("\n");

	/* Reserved fields */
	{
		int i, any_nonzero = 0;
		for (i = 0; i < 7; i++) {
			if (hdr3do->reserved1[i] != 0)
				any_nonzero = 1;
		}
		if (any_nonzero) {
			printf("  [0x50] Reserved1:            ");
			for (i = 0; i < 7; i++)
				printf("0x%08X ", hdr3do->reserved1[i]);
			printf("\n         (expected all zero)\n");
		} else {
			printf("  [0x50] Reserved1[0-6]:       all zero (OK)\n");
		}
	}
}

static void print_extended_regions(FILE *f, uint32_t first_offset, long file_size)
{
	uint32_t offset = first_offset;
	int region_num = 1;
	uint8_t buf[AIF_REGION_HEADER_SIZE];
	AIFRegionHeader rh;
	char name_buf[33];

	printf("\n=== Extended AIF Region Headers ===\n");
	print_separator();

	while (offset != 0) {
		if ((long)offset + AIF_REGION_HEADER_SIZE > file_size) {
			printf("  [Region %d] ERROR: offset 0x%X exceeds file size\n",
			       region_num, offset);
			break;
		}

		if (fseek(f, (long)offset, SEEK_SET) != 0) {
			printf("  [Region %d] ERROR: failed to seek to offset 0x%X\n",
			       region_num, offset);
			break;
		}

		if (fread(buf, 1, AIF_REGION_HEADER_SIZE, f) != AIF_REGION_HEADER_SIZE) {
			printf("  [Region %d] ERROR: failed to read region header at 0x%X\n",
			       region_num, offset);
			break;
		}

		/* Parse region header (respecting endianness) */
		memcpy(&rh.next_offset,  buf + 0x00, 4);
		memcpy(&rh.load_address, buf + 0x04, 4);
		memcpy(&rh.size,         buf + 0x08, 4);
		memcpy(rh.name,          buf + 0x0C, 32);

		rh.next_offset  = fix32(rh.next_offset);
		rh.load_address = fix32(rh.load_address);
		rh.size         = fix32(rh.size);

		memcpy(name_buf, rh.name, 32);
		name_buf[32] = '\0';

		printf("  Region %d (file offset 0x%X):\n", region_num, offset);
		printf("    Next Region Offset:  0x%08X", rh.next_offset);
		if (rh.next_offset == 0)
			printf(" (last region)");
		printf("\n");
		printf("    Load Address:        0x%08X\n", rh.load_address);
		printf("    Size:                %u bytes (0x%X)\n", rh.size, rh.size);
		printf("    Name:                \"%s\"\n", name_buf);
		printf("    Data at:             file offset 0x%X\n",
		       offset + AIF_REGION_HEADER_SIZE);

		if (rh.next_offset != 0 && rh.next_offset <= offset) {
			printf("    WARNING: next_offset does not advance (loop detected)\n");
			break;
		}

		offset = rh.next_offset;
		region_num++;
		print_separator();
	}
}

/* ===== Validation ===== */

static int validate_aif(const AIFHeader *hdr, long file_size)
{
	int warnings = 0;

	printf("\n=== Validation ===\n");
	print_separator();

	/* Check first word */
	if (!is_nop(hdr->bl_decompress) && !is_bl(hdr->bl_decompress)) {
		printf("  WARNING: Word 0 (0x%08X) is neither NOP nor BL\n",
		       hdr->bl_decompress);
		warnings++;
	}

	/* Check exit instruction */
	if (hdr->swi_exit != AIF_SWI_EXIT && hdr->swi_exit != AIF_BRANCH_SELF) {
		printf("  NOTE: Exit instruction (0x%08X) is non-standard\n",
		       hdr->swi_exit);
	}

	/* Check sizes are multiples of 4 */
	if (hdr->ro_size & 3) {
		printf("  WARNING: RO size (%u) not a multiple of 4\n", hdr->ro_size);
		warnings++;
	}
	if (hdr->rw_size & 3) {
		printf("  WARNING: RW size (%u) not a multiple of 4\n", hdr->rw_size);
		warnings++;
	}
	if (hdr->debug_size & 3) {
		printf("  WARNING: Debug size (%u) not a multiple of 4\n",
		       hdr->debug_size);
		warnings++;
	}
	if (hdr->zi_size & 3) {
		printf("  WARNING: ZI size (%u) not a multiple of 4\n", hdr->zi_size);
		warnings++;
	}

	/* Check debug type */
	if (hdr->debug_type > 3) {
		printf("  WARNING: Debug type (%u) is reserved/unknown\n",
		       hdr->debug_type);
		warnings++;
	}

	/* Consistency: debug_size vs debug_type */
	if (hdr->debug_size == 0 && hdr->debug_type != 0) {
		printf("  WARNING: Debug type is %u but debug size is 0\n",
		       hdr->debug_type);
		warnings++;
	}
	if (hdr->debug_size > 0 && hdr->debug_type == 0) {
		printf("  WARNING: Debug size is %u but debug type is 0 (none)\n",
		       hdr->debug_size);
		warnings++;
	}

	/* File size vs declared sizes */
	{
		int is_exec = (hdr->bl_entry >> 24) == 0xEB;
		uint64_t min_data = (uint64_t)hdr->ro_size + hdr->rw_size +
		                    hdr->debug_size;
		if (!is_exec)
			min_data += AIF_HEADER_SIZE;

		if ((uint64_t)file_size < min_data) {
			printf("  WARNING: File size (%ld) < declared RO+RW+Debug (%llu)\n",
			       file_size, (unsigned long long)min_data);
			warnings++;
		}
	}

	/* Address mode reserved bits */
	if (hdr->address_mode & ~(0xFF | AIF_SEPARATE_DATA_BASE)) {
		printf("  WARNING: Address mode has reserved bits set (0x%08X)\n",
		       hdr->address_mode);
		warnings++;
	}

	/* Reserved words */
	if (hdr->reserved[1] != 0) {
		printf("  WARNING: Reserved[1] at 0x3C is non-zero (0x%08X)\n",
		       hdr->reserved[1]);
		warnings++;
	}

	if (warnings == 0)
		printf("  No issues found.\n");
	else
		printf("  %d warning(s) found.\n", warnings);

	return warnings;
}

/* ===== Hex Dump ===== */

static void hexdump_header(const uint8_t *raw)
{
	int i, j;

	printf("\n=== Raw Header Hex Dump ===\n");
	print_separator();
	printf("  Offset   00 01 02 03 04 05 06 07  08 09 0A 0B 0C 0D 0E 0F  ASCII\n");
	print_separator();

	for (i = 0; i < AIF_HEADER_SIZE; i += 16) {
		printf("  0x%04X:  ", i);
		for (j = 0; j < 16; j++) {
			if (j == 8)
				printf(" ");
			printf("%02X ", raw[i + j]);
		}
		printf(" ");
		for (j = 0; j < 16; j++) {
			uint8_t c = raw[i + j];
			printf("%c", (c >= 0x20 && c < 0x7F) ? c : '.');
		}
		printf("\n");
	}
}

/* ===== Usage ===== */

static void usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s [options] <aif-file>\n"
		"\n"
		"Options:\n"
		"  -h, --help        Show this help message\n"
		"  -x, --hex         Include raw hex dump of header\n"
		"  -b, --big-endian  Force big-endian interpretation\n"
		"  -l, --little-endian  Force little-endian interpretation\n"
		"  -q, --quiet       Skip validation checks\n"
		"\n"
		"Prints detailed information about an ARM Image Format (AIF) file,\n"
		"including 3DO Platform Extensions and Extended AIF regions.\n",
		prog);
}

/* ===== Main ===== */

int main(int argc, char *argv[])
{
	const char *filename = NULL;
	int show_hex = 0;
	int force_endian = -1; /* -1=auto, 0=LE, 1=BE */
	int quiet = 0;
	int i;
	FILE *f;
	long file_size;
	uint8_t raw_header[AIF_HEADER_SIZE];
	AIFHeader hdr;
	int is_exec, has_3do;

	/* Parse arguments */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
			usage(argv[0]);
			return 0;
		} else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--hex") == 0) {
			show_hex = 1;
		} else if (strcmp(argv[i], "-b") == 0 ||
		           strcmp(argv[i], "--big-endian") == 0) {
			force_endian = 1;
		} else if (strcmp(argv[i], "-l") == 0 ||
		           strcmp(argv[i], "--little-endian") == 0) {
			force_endian = 0;
		} else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0) {
			quiet = 1;
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

	/* Open file */
	f = fopen(filename, "rb");
	if (!f) {
		perror(filename);
		return 1;
	}

	/* Get file size */
	fseek(f, 0, SEEK_END);
	file_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	if (file_size < AIF_HEADER_SIZE) {
		fprintf(stderr,
			"Error: file is %ld bytes, minimum AIF header is %d bytes\n",
			file_size, AIF_HEADER_SIZE);
		fclose(f);
		return 1;
	}

	/* Read raw header */
	if (fread(raw_header, 1, AIF_HEADER_SIZE, f) != AIF_HEADER_SIZE) {
		fprintf(stderr, "Error: failed to read AIF header\n");
		fclose(f);
		return 1;
	}

	/* Detect or set endianness */
	if (force_endian >= 0) {
		g_swap_endian = force_endian;
	} else {
		g_swap_endian = detect_endianness(raw_header);
	}

	/* Copy and fix endianness */
	memcpy(&hdr, raw_header, AIF_HEADER_SIZE);
	fix_header_endian(&hdr);

	/* Print banner */
	printf("aifinfo: ARM Image Format (AIF) File Analyzer\n");
	print_separator();
	printf("  File:     %s\n", filename);
	printf("  Endian:   %s%s\n",
	       g_swap_endian ? "big-endian" : "little-endian",
	       force_endian >= 0 ? " (forced)" : " (detected)");
	print_separator();

	/* Print main header */
	print_aif_header(&hdr, file_size);

	/* Determine if 3DO header present */
	is_exec = (hdr.bl_entry >> 24) == 0xEB;
	has_3do = (hdr.workspace & AIF_3DOHEADER) != 0;

	/* Print 3DO header if present */
	if (has_3do) {
		uint8_t raw_3do[128];
		_3DOBinHeader hdr3do;

		if (file_size < AIF_HEADER_SIZE + 128) {
			printf("\n  WARNING: 3DO header flag set but file too small "
			       "(%ld bytes, need %d)\n", file_size,
			       AIF_HEADER_SIZE + 128);
		} else {
			fseek(f, AIF_HEADER_SIZE, SEEK_SET);
			if (fread(raw_3do, 1, 128, f) != 128) {
				printf("\n  WARNING: failed to read 3DO binary header\n");
			} else {
				/* Fix endianness for 3DO header */
				memcpy(&hdr3do, raw_3do, sizeof(hdr3do));
				if (g_swap_endian) {
					int j;
					for (j = 0; j < 4; j++)
						hdr3do.item_node[j] = swap32(hdr3do.item_node[j]);
					/* flags, os_version, os_revision, reserved are bytes - no swap */
					hdr3do.stack         = swap32(hdr3do.stack);
					hdr3do.freespace     = swap32(hdr3do.freespace);
					hdr3do.signature     = swap32(hdr3do.signature);
					hdr3do.signature_len = swap32(hdr3do.signature_len);
					hdr3do.max_usecs     = swap32(hdr3do.max_usecs);
					hdr3do.reserved0     = swap32(hdr3do.reserved0);
					/* name is bytes - no swap */
					hdr3do.time          = swap32(hdr3do.time);
					for (j = 0; j < 7; j++)
						hdr3do.reserved1[j] = swap32(hdr3do.reserved1[j]);
				}
				print_3do_header(&hdr3do);
			}
		}
	}

	/* Print Extended AIF regions */
	if (!is_exec && hdr.reserved[0] != 0) {
		print_extended_regions(f, hdr.reserved[0], file_size);
	}

	/* Hex dump */
	if (show_hex)
		hexdump_header(raw_header);

	/* Validation */
	if (!quiet)
		validate_aif(&hdr, file_size);

	printf("\n");
	fclose(f);
	return 0;
}

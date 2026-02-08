# Format of the Areas Chunk

The areas chunk (OBJ_AREA) contains the actual area contents (code, data, debugging data, etc.) together with their associated relocation data. Its *chunkId* is `"OBJ_AREA"`. Pictorially, an area's layout is:

```
Area 1
Area 1 Relocation
. . .
Area n
Area n Relocation
```

An area is simply a sequence of byte values. The endian-ness of the words and half-words within it shall agree with that of the containing AOF file.

An area is followed by its associated table of relocation directives (if any). An area is either completely initialised by the values from the file or is initialised to zero, as specified by bit 12 of its area attributes.

Both the area contents and the table of relocation directives are aligned to 4-byte boundaries.

## Relocation Directives

A relocation directive describes a value which is computed at link time or load time, but which cannot be fixed when the object module is created.

In the absence of applicable relocation directives, the value of a byte, halfword, word or instruction from the preceding area is exactly the value that will appear in the final image.

A field may be subject to more than one relocation.

Pictorially, a relocation directive looks like:

```
----------------------------------------
offset
----------------------------------------
1   |II  |B   |A   |R   |F T |24-bit SID
----------------------------------------
```

Offset is the byte offset in the preceding area of the subject field to be relocated by a value calculated as described below.

The interpretation of the 24-bit *SID* field depends on the *A* bit.

- If *A* (bit 27) is 1, the subject field is relocated (as further described below) by the value of the symbol of which *SID* is the 0-origin index in the symbol table chunk.
- If *A* (bit 27) is 0, the subject field is relocated (as further described below) by the base of the area of which *SID* is the 0-origin index in the array of areas, (or, equivalently, in the array of area headers).

The 2-bit field type *FT* (bits 25, 24) describes the subject field:

- `00`: the field to be relocated is a byte
- `01`: the field to be relocated is a half-word (2 bytes)
- `10`: the field to be relocated is a word (4 bytes)
- `11`: the field to be relocated is an instruction or instruction sequence

Bytes, halfwords and instructions may only be relocated by values of suitably small size. Overflow is faulted by the linker.

An ARM branch, or branch-with-link instruction is always a suitable subject for a relocation directive of field type instruction. For details of other relocatable instruction sequences, refer to The handling of relocation directives in the linker documentation.

If the subject field is an instruction sequence, then Offset addresses the first instruction of the sequence and the *II* field (bits 29 and 30) constrains how many instructions may be modified by this directive:

- `00`: no constraint (the linker may modify as many contiguous instructions as it needs to)
- `01`: the linker will modify at most 1 instruction
- `10`: the linker will modify at most 2 instructions
- `11`: the linker will modify at most 3 instructions

The way the relocation value is used to modify the subject field is determined by the *R* (PC-relative) bit, modified by the *B* (based) bit.

### PC-Relative Relocation

*R* (bit 26) = 1 and *B* (bit 28) = 0 specifies PC-relative relocation: to the subject field is added the difference between the relocation value and the base of the area containing the subject field. In pseudo C:

```c
subject_field = subject_field + (relocation_value -
            base_of_area_containing(subject_field))
```

As a special case, if *A* is 0, and the relocation value is specified as the base of the area containing the subject field, then it is not added and:

```c
subject_field = subject_field - base_of_area_containing(subject_field)
```

This caters for relocatable PC-relative branches to fixed target addresses.

If *R* is 1, *B* is usually 0. A *B* value of 1 is used to denote that the inter-link-unit value of a branch destination is to be used, rather than the more usual intra-link-unit value.

(Aside: this allows compilers to perform the tail-call optimisation on reentrant code.)

### Plain Additive Relocation

*R* (bit 26) = 0 and *B* (bit 28) = 0, specifies plain additive relocation: the relocation value is added to the subject field. In pseudo C:

```c
subject_field = subject_field + relocation_value
```

### Based Area Relocation

*R* (bit 26) = 0 and *B* (bit 28) = 1, specifies based area relocation. The relocation value must be an address within a based data area. The subject field is incremented by the difference between this value and the base address of the consolidated based area group (the linker consolidates all areas based on the same base register into a single, contiguous region of the output image). In pseudo C:

```c
subject_field = subject_field + (relocation_value -
            base_of_area_group_containing(relocation_value))
```

For example, when generating reentrant code, the C compiler will place address constants in an adcon area based on register *sb*, and load them using *sb* relative LDRs. At link time, separate adcon areas will be merged and *sb* will no longer point where presumed at compile time. *B* type relocation of the LDR instructions corrects for this.

Bits 29 and 30 of the relocation flags word shall be 0; bit 31 shall be 1.

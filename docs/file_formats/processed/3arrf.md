# ARM Object Format

An object file written in ARM Object Format (AOF) consists of any number of named, attributed *areas*. Attributes include: read-only; reentrant; code; data; position independent; etc. (for details see the Attributes + Alignment section of the Technical Specifications). Typically, a compiled AOF file contains a read-only code area, and a read-write data area (a 0-initialised data area is also common, and reentrant code uses a separate based area for address constants).

## Relocation Directives

Associated with each area is a (possibly empty) list of relocation directives which describe locations that the linker will have to update when:

- A non-zero base address is assigned to the area.
- A symbolic reference is resolved.

Each relocation directive may be given relative to the (not yet assigned) base address of an area in the same AOF file, or relative to a symbol in the symbol table.

## Symbol Visibility

Each symbol may:

- Have a definition within its containing object file which is local to the object file.
- Have a definition within the object file which is visible globally (to all object files in the link step).
- Be a reference to a symbol defined in some other object file.

## AOF as Output Format

When AOF is used as an output format, the linker does the following with its input object files:

- Merges similarly named and attributed areas.
- Performs PC-relative relocations between merged areas.
- Re-writes symbol-relative relocation directives between merged areas, as area-based relocation directives belonging to the merged area.
- Minimises the symbol table.

Unresolved references remain unresolved, and the output AOF file may be used as the input to a further link step.

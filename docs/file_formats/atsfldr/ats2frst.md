# ARM Object Format

This document defines a file format called *ARM Object Format* or *AOF*, which is used by language processors for ARM-based systems.

The ARM linker accepts input files in this format and can generate output in the same format, as well as in a variety of image formats. The ARM linker is described in The ARM Linker (armlink) in the User Manual.

ARM Object Format directly supports the ARM Procedure Call standard (APCS), which is described in [ARM Procedure Call Standard](ats4frst.md).

## Contents

- [About AOF](2atsa.md)
- [The Overall Structure of an AOF File](2atsb.md)
- [ARM Object Format](2atsc.md)
- [Format of the Areas Chunk](2atsd.md)
- [Format of the Symbol Table Chunk (OBJ_SYMT)](2atse.md)
- [The String Table Chunk (OBJ_STRT)](2atsf.md)
- [The Identification Chunk (OBJ_IDFN)](2atsg.md)

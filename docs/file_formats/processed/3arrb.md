# Library Module Inclusion

An object file may contain references to external objects (functions and variables), which the linker will attempt to resolve by matching them to definitions found in other object files and libraries.

Usually, at least one library file is specified in the input list. A library is just a collection of AOF files stored in an ARM Object Library Format file. The important difference between object files and libraries is:

- Each object file in the input list appears in the output unconditionally, whether or not anything refers to it (although unused areas will be eliminated from outputs of type AIF).
- A module from a library is included in the output if, and only if, some object file or some already-included library module makes a *non-weak* reference to it.

## Processing Order

The linker processes its input list as follows:

1. First, the object files are linked together, ignoring the libraries. Usually there will be a resultant set of references to as yet undefined symbols. Some of these may be *weak*: references which are allowed to remain unsatisfied, and which do not cause a library member to be loaded.

2. Then, in the order that they appear in the input file list, the libraries are processed as follows:
   - The library is searched for members containing symbol definitions which match currently unsatisfied, non-weak references.
   - Each such member is loaded, satisfying some unsatisfied references (including possibly *weak* ones), and maybe creating new unsatisfied references (again, maybe including *weak* ones).
   - The search is repeated until no further members are loaded.

Each library is processed in turn, so a reference from a member of a later library to a member of an earlier library cannot be satisfied. As a result, circular dependencies between libraries are forbidden.

It is an error if any *non-weak* reference remains unsatisfied at the end of a linking operation, other than one which generates partially-linked, relocatable AOF.

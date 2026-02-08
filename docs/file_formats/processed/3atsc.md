# Object Code Libraries

An object code library is a library file whose members are files in ARM Object Format (see [ARM Object Format](ats2frst.md) for details).

An object code library contains two additional chunks: an external symbol table chunk named OFL_SYMT; and a time stamp chunk named OFL_TIME.

## OFL_SYMT

The external symbol table contains an entry for each external symbol defined by members of the library, together with the index of the chunk containing the member defining that symbol.

The OFL_SYMT chunk has exactly the same format as the LIB_DIRY chunk except that the Data section of each entry contains only a string, the name of an external symbol, and between 1 and 4 bytes of NUL padding, as follows:

| Field                | Description                                                    |
|----------------------|----------------------------------------------------------------|
| ChunkIndex           |                                                                |
| EntryLength          | The size of this OFL_SYMT chunk (an integral number of words)  |
| DataLength           | The size of the External Symbol Name and Padding (an integral number of words) |
| External Symbol Name |                                                                |
| Padding              |                                                                |

OFL_SYMT entries do not contain time stamps.

## OFL_TIME

The OFL_TIME chunk records when the OFL_SYMT chunk was last modified and has the same format as the LIB_TIME chunk (see [Time Stamps](3atsb.md#time-stamps)).

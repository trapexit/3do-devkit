#pragma once
#pragma force_top_level

#include "types.h"
#include "folio.h"

/*
  This value needs to be unique in the SWI space. It will be
  bitshifted left 16 and bitwise or'ed with the function offset.
  Known values used by kernel:
    - DebuggerFolio = 0
    - GraphicsFolio = 2
    - FileFolio = 3
    - AudioFolio = 4
    - MathFolio = 5
    - InternationalFolio = 9
 */
#define EXAMPLE_FOLIO 'E'
#define EXAMPLE_FOLIO_SWI (EXAMPLE_FOLIO << 16)

#define USER_ADDI32 -1
#define USER_SUBI32 -2

#define SWI_ADDI32   (EXAMPLE_FOLIO_SWI + 0)
#define SWI_SUBI32   (EXAMPLE_FOLIO_SWI + 1)
#define SWI_MADAMREV (EXAMPLE_FOLIO_SWI + 2)
#define SWI_CLIOREV  (EXAMPLE_FOLIO_SWI + 3)

typedef struct ExampleFolio ExampleFolio;

struct ExampleFolio
{
  Folio folio;
};

#ifdef __cplusplus
extern "C" {
#endif

Err OpenExampleFolio(void);
Err CloseExampleFolio(void);

i32 AddI32(i32 n0, i32 n1);
i32 SubI32(i32 n0, i32 n1);

i32 __swi(SWI_ADDI32)   SWIAddI32(i32 n0, i32 n1);
i32 __swi(SWI_SUBI32)   SWISubI32(i32 n0, i32 n1);
u32 __swi(SWI_MADAMREV) SWIMADAMRev(void);
u32 __swi(SWI_CLIOREV)  SWICLIORev(void);

#ifdef __cplusplus
}
#endif

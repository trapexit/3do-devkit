/*
  Rotating cube demo for the 3DO cel engine.

  The program renders a perspective-projected cube by mapping one cel onto each
  visible face, sorting faces back-to-front, and redrawing the animation each
  VBL. Press A, B, or C to cycle between 3DO logo colors, solid red, solid blue,
  solid yellow, and a textured 3DO logo cube.
*/

#include "abort.h"
#include "celutils.h"
#include "controlpad.h"
#include "displayutils.h"
#include "event.h"
#include "graphics.h"
#include "io.h"
#include "mem.h"
#include "operamath.h"
#include "string.h"

#define FRACBITS_16  16
#define ONE_F16      Convert32_F16(1)
#define ANG_256_F16  Convert32_F16(256)
#define VERT_COUNT   8
#define FACE_COUNT   6
#define FACE_VERTS   4
#define VIEW_SHIFT   9
#define SWITCH_BUTTONS (ControlA | ControlB | ControlC)

#define CALC_WOFFSET_BPP8(width) (((width) % 4 == 0) ? ((width) / 4) : ((width) / 4 + 1))
#define CLEAR_CEL_PREAMBLES(ccb) ((ccb)->ccb_PRE0 = (ccb)->ccb_PRE1 = 0)
#define SET_CEL_BPP8(ccb) ((ccb)->ccb_PRE0 |= (PRE0_BPP_8 << PRE0_BPP_SHIFT))
#define SET_CEL_WOFFSET10(ccb, width) ((ccb)->ccb_PRE1 |= (((width) - PRE1_WOFFSET_PREFETCH) << PRE1_WOFFSET10_SHIFT))
#define SET_CEL_TLHPCNT(ccb, width) ((ccb)->ccb_PRE1 |= (((width) - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT))
#define SET_CEL_VCNT(ccb, height) ((ccb)->ccb_PRE0 |= (((height) - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT))

#define PMV_0_MASK  0x1C00
#define PDV_0_MASK  0x0300
#define PMV_0_SHIFT 10
#define PDV_0_SHIFT 8

typedef frac16 mat4x4[4][4];
typedef frac16 vector4[4];
typedef frac16 vector3[3];

typedef struct Vertex
{
  frac16 x;
  frac16 y;
  frac16 z;
} Vertex;

typedef struct Face
{
  u32    indices[FACE_VERTS];
  Vertex world[FACE_VERTS];
  Point  screen[FACE_VERTS];
  CCB   *ccb;
  u32    pmv;
  frac16 z;
} Face;

typedef struct Cube
{
  Face faces[FACE_COUNT];
} Cube;

/*
  Cube vertices are in 16.16 fixed-point format. The camera is fixed at the
  origin looking down +Z, so the cube is moved forward by world_z each frame.
*/
static const Vertex cube_vertices[] =
  {
    { -ONE_F16, -ONE_F16, -ONE_F16 },
    {  ONE_F16, -ONE_F16, -ONE_F16 },
    {  ONE_F16,  ONE_F16, -ONE_F16 },
    { -ONE_F16,  ONE_F16, -ONE_F16 },
    { -ONE_F16, -ONE_F16,  ONE_F16 },
    {  ONE_F16, -ONE_F16,  ONE_F16 },
    {  ONE_F16,  ONE_F16,  ONE_F16 },
    { -ONE_F16,  ONE_F16,  ONE_F16 }
  };

static const u32 cube_faces[FACE_COUNT][FACE_VERTS] =
  {
    { 1, 0, 3, 2 },
    { 4, 5, 6, 7 },
    { 0, 4, 7, 3 },
    { 5, 1, 2, 6 },
    { 2, 3, 7, 6 },
    { 5, 4, 0, 1 }
  };

enum CubeColorScheme
{
  CUBE_COLORS_3DO_LOGO,
  CUBE_COLORS_ALL_RED,
  CUBE_COLORS_ALL_BLUE,
  CUBE_COLORS_ALL_YELLOW,
  CUBE_TEXTURED_3DO_LOGO,
  CUBE_COLOR_SCHEME_COUNT
};

/* Opposite faces share the same color in the 3DO logo scheme. */
static const u16 face_colors_3do_logo[FACE_COUNT] =
  {
    (u16)MakeRGB15(31, 0, 0),
    (u16)MakeRGB15(31, 0, 0),
    (u16)MakeRGB15(0, 0, 31),
    (u16)MakeRGB15(0, 0, 31),
    (u16)MakeRGB15(31, 31, 0),
    (u16)MakeRGB15(31, 31, 0)
  };

static const u16 solid_cube_colors[CUBE_COLOR_SCHEME_COUNT] =
  {
    0,
    (u16)MakeRGB15(31, 0, 0),
    (u16)MakeRGB15(0, 0, 31),
    (u16)MakeRGB15(31, 31, 0),
    0
  };

/* PMV values shade single-color and textured cubes so each side is visible. */
static const u32 solid_face_pmv[FACE_COUNT] = { 7, 4, 5, 6, 3, 2 };

static Cube cubes[CUBE_COLOR_SCHEME_COUNT];
static Vertex world_vertices[VERT_COUNT];
static Face *renderables[FACE_COUNT];
static frac16 renderable_z[FACE_COUNT];
static u32 renderable_count;
static u32 active_cube;

static ScreenContext *sc;
static Item vram_ioreq;
static Item vbl_ioreq;
static int current_screen;

static
void
display_init(void)
{
  Err err;

  err = OpenGraphicsFolio();
  if(err < 0)
    abort_err(err);

  sc = (ScreenContext*)AllocMem(sizeof(ScreenContext), MEMTYPE_ANY);
  if(sc == NULL)
    abort("unable to allocate ScreenContext");

  err = CreateBasicDisplay(sc, DI_TYPE_DEFAULT, 2);
  if(err < 0)
    abort_err(err);

  DisableHAVG(sc->sc_Screens[0]);
  DisableVAVG(sc->sc_Screens[0]);
  DisableHAVG(sc->sc_Screens[1]);
  DisableVAVG(sc->sc_Screens[1]);

  vram_ioreq = CreateVRAMIOReq();
  vbl_ioreq = CreateVBLIOReq();
  current_screen = 0;
}

static
void
display_cleanup(void)
{
  if(vbl_ioreq > 0)
    DeleteVBLIOReq(vbl_ioreq);
  if(vram_ioreq > 0)
    DeleteVRAMIOReq(vram_ioreq);
  if(sc != NULL)
    {
      DeleteBasicDisplay(sc);
      FreeMem(sc, sizeof(ScreenContext));
    }

  vbl_ioreq = 0;
  vram_ioreq = 0;
  sc = NULL;
}

static
void
control_pad_cleanup(void)
{
  KillControlPad();
}

static
void
control_pad_init(void)
{
  Err err;

  err = InitControlPad(1);
  if(err < 0)
    abort_err(err);
}

static
void
display_clear(int color)
{
  SetVRAMPages(vram_ioreq,
               sc->sc_Bitmaps[current_screen]->bm_Buffer,
               color,
               sc->sc_NumBitmapPages,
               0xFFFFFFFF);
}

static
void
display_swap(void)
{
  current_screen = !current_screen;
}

static
void
display_display_and_swap(void)
{
  DisplayScreen(sc->sc_Screens[current_screen], 0);
  display_swap();
}

static
void
display_waitvbl(void)
{
  WaitVBL(vbl_ioreq, 1);
}

static
void
display_draw_cels(CCB *ccb)
{
  DrawCels(sc->sc_BitmapItems[current_screen], ccb);
}

static
void
set_pixc_pmv(CCB *ccb,
             u32  pmv)
{
  ccb->ccb_PIXC &= ~PMV_0_MASK;
  ccb->ccb_PIXC |= (pmv << PMV_0_SHIFT);
}

static
void
set_pixc_pdv(CCB *ccb,
             u32  pdv)
{
  ccb->ccb_PIXC &= ~PDV_0_MASK;
  ccb->ccb_PIXC |= (pdv << PDV_0_SHIFT);
}

static
size_t
cel_row_bytes(CCB *cel)
{
  u32 pre1;
  size_t woffset;

  pre1 = CEL_PRE1WORD(cel);
  woffset = (size_t)(pre1 & PRE1_WOFFSET10_MASK) >> PRE1_WOFFSET10_SHIFT;

  return ((woffset + PRE1_WOFFSET_PREFETCH) * sizeof(i32));
}

static
size_t
cel_source_bytes(CCB *cel)
{
  return ((size_t)cel->ccb_Height * cel_row_bytes(cel));
}

static
void
set_cel_plut_color(CCB *ccb,
                   u16  color)
{
  u32 i;
  u16 *plut;

  plut = (u16*)ccb->ccb_PLUTPtr;

  for(i = 0; i < 32; i++)
    plut[i] = color;
}

static
CCB*
create_colored_face_cel(u16  color,
                        CCB *shared_data_cel)
{
  CCB *cel;

  if(shared_data_cel == NULL)
    {
      cel = CreateCel(16, 16, 8, CREATECEL_CODED, NULL);
      if(cel == NULL)
        abort("unable to create face cel");

      /*
        The cel engine reads the CCB to decide how to fetch pixels, how to map
        them onto the destination bitmap, and which optional CCB fields to load.
        These flags make each cube face a small 8bpp cel that can be mapped by
        MapCel().
      */
      cel->ccb_Flags =
        CCB_NPABS  |  /* ccb_NextPtr is an absolute pointer for our linked list. */
        CCB_SPABS  |  /* ccb_SourcePtr is an absolute pointer to cel pixel data. */
        CCB_PPABS  |  /* ccb_PLUTPtr is an absolute pointer to this face's PLUT. */
        CCB_LDSIZE |  /* Load HDX/HDY/VDX/VDY; MapCel() writes these for scaling. */
        CCB_LDPRS  |  /* Load DDX/DDY; MapCel() writes these for perspective. */
        CCB_CCBPRE |  /* Use PRE0/PRE1 from the CCB instead of source data. */
        CCB_YOXY   |  /* Use X/Y position from the CCB; MapCel() writes it. */
        CCB_ACW    |  /* Draw clockwise-mapped pixels. */
        CCB_ACCW   |  /* Draw counter-clockwise-mapped pixels too. */
        CCB_ACE    |  /* Enable both corner engines. */
        CCB_LDPLUT |  /* Load this cel's palette before drawing it. */
        CCB_ALSC   |  /* Clip mapped faces cleanly at the screen edges. */
        CCB_LDPPMP;   /* Load PIXC so PMV/PDV affect the face color. */

      /* Do not treat pixel value 0 as an opaque background pixel. */
      cel->ccb_Flags &= ~CCB_BGND;
      cel->ccb_Width = 16;
      cel->ccb_Height = 16;

      UNLAST_CEL(cel);

      CLEAR_CEL_PREAMBLES(cel);
      SET_CEL_BPP8(cel);
      SET_CEL_WOFFSET10(cel, CALC_WOFFSET_BPP8(cel->ccb_Width));
      SET_CEL_TLHPCNT(cel, cel->ccb_Width);
      SET_CEL_VCNT(cel, cel->ccb_Height);

      memset(cel->ccb_SourcePtr, 0, cel_source_bytes(cel));
    }
  else
    {
      cel = CloneCel(shared_data_cel, CLONECEL_COPY_PLUT);
      if(cel == NULL)
        abort("unable to clone face cel");
    }

  set_cel_plut_color(cel, color);
  set_pixc_pmv(cel, 7);
  set_pixc_pdv(cel, 3);

  return cel;
}

static
CCB*
create_textured_face_cel(CCB *shared_data_cel)
{
  CCB *cel;

  if(shared_data_cel == NULL)
    cel = LoadCel("Art/3do_logo_unpacked.cel", MEMTYPE_CEL);
  else
    cel = CloneCel(shared_data_cel, CLONECEL_CCB_ONLY);

  if(cel == NULL)
    abort("unable to load Art/3do_logo_unpacked.cel");

  cel->ccb_Flags |= CCB_LDPPMP;
  set_pixc_pdv(cel, 3);

  return cel;
}

static
void
zero_mat4x4(mat4x4 mat)
{
  memset(mat, 0, sizeof(frac16) * 16);
}

static
void
identity_mat4x4(mat4x4 mat)
{
  zero_mat4x4(mat);
  mat[0][0] = mat[1][1] = mat[2][2] = mat[3][3] = ONE_F16;
}

static
void
build_rotation_matrix(mat4x4 rot,
                      frac16 xangle,
                      frac16 yangle,
                      frac16 zangle)
{
  frac16 c;
  frac16 s;
  mat4x4 xrot;
  mat4x4 yrot;
  mat4x4 zrot;
  mat4x4 xyrot;

  identity_mat4x4(xrot);
  identity_mat4x4(yrot);
  identity_mat4x4(zrot);

  c = CosF16(xangle);
  s = SinF16(xangle);
  xrot[1][1] = c;
  xrot[1][2] = s;
  xrot[2][1] = -s;
  xrot[2][2] = c;

  c = CosF16(yangle);
  s = SinF16(yangle);
  yrot[0][0] = c;
  yrot[0][2] = -s;
  yrot[2][0] = s;
  yrot[2][2] = c;

  c = CosF16(zangle);
  s = SinF16(zangle);
  zrot[0][0] = c;
  zrot[0][1] = s;
  zrot[1][0] = -s;
  zrot[1][1] = c;

  MulMat44Mat44_F16(xyrot, xrot, yrot);
  MulMat44Mat44_F16(rot, xyrot, zrot);
}

static
void
transform_vertex(Vertex       *out,
                 const Vertex *in,
                 mat4x4        rot,
                 frac16        world_z)
{
  vector4 src, dst;

  src[0] = in->x;
  src[1] = in->y;
  src[2] = in->z;
  src[3] = ONE_F16;

  MulVec4Mat44_F16(dst, src, rot);

  out->x = dst[0];
  out->y = dst[1];
  out->z = dst[2] + world_z;
}

static
void
point_to_vector(vector3       out,
                const Vertex *a,
                const Vertex *b)
{
  out[0] = b->x - a->x;
  out[1] = b->y - a->y;
  out[2] = b->z - a->z;
}

static
boolean
cull_face(Face *face)
{
  vector3 pov, edge1, edge2, normal;

  pov[0] = -face->world[0].x;
  pov[1] = -face->world[0].y;
  pov[2] = -face->world[0].z;

  point_to_vector(edge1, &face->world[0], &face->world[1]);
  point_to_vector(edge2, &face->world[0], &face->world[3]);
  Cross3_F16(normal, edge2, edge1);

  return ((Dot3_F16(normal, pov) > 0) ? TRUE : FALSE);
}

static
void
project_face(Face   *face,
             frac16  half_width,
             frac16  half_height)
{
  u32 i;
  frac16 zsum = 0;

  for(i = 0; i < FACE_VERTS; i++)
    {
      frac16 z;

      z = face->world[i].z == 0 ? ONE_F16 : face->world[i].z;

      face->screen[i].pt_X = (DivSF16(face->world[i].x << VIEW_SHIFT, z) + half_width) >> FRACBITS_16;
      face->screen[i].pt_Y = (DivSF16(-face->world[i].y << VIEW_SHIFT, z) + half_height) >> FRACBITS_16;
      zsum += face->world[i].z;
    }

  face->z = zsum >> 2;
}

static
void
insert_renderable(Face *face)
{
  u32 dst;
  u32 i;

  dst = renderable_count;

  while((dst > 0) && (face->z > renderable_z[dst - 1]))
    dst--;

  for(i = renderable_count; i > dst; i--)
    {
      renderables[i] = renderables[i - 1];
      renderable_z[i] = renderable_z[i - 1];
    }

  renderables[dst] = face;
  renderable_z[dst] = face->z;
  renderable_count++;
}

static
void
update_cube(Cube   *cube,
            frac16  xangle,
            frac16  yangle,
            frac16  zangle)
{
  const frac16 half_width = Convert32_F16(sc->sc_BitmapWidth >> 1);
  const frac16 half_height = Convert32_F16(sc->sc_BitmapHeight >> 1);
  const frac16 world_z = Convert32_F16(6);
  mat4x4 rot;
  u32 face_index;
  u32 vert_index;

  renderable_count = 0;
  build_rotation_matrix(rot, xangle, yangle, zangle);

  /* 1. Rotate the 8 unique cube vertices into world space. */
  for(vert_index = 0; vert_index < VERT_COUNT; vert_index++)
    transform_vertex(&world_vertices[vert_index],
                     &cube_vertices[vert_index],
                     rot,
                     world_z);

  /* 2. For each face, cull, project to screen coordinates, then MapCel(). */
  for(face_index = 0; face_index < FACE_COUNT; face_index++)
    {
      Face *face;

      face = &cube->faces[face_index];

      for(vert_index = 0; vert_index < FACE_VERTS; vert_index++)
        {
          Vertex *src;

          src = &world_vertices[face->indices[vert_index]];
          face->world[vert_index].x = src->x;
          face->world[vert_index].y = src->y;
          face->world[vert_index].z = src->z;
        }

      if(cull_face(face))
        continue;

      project_face(face, half_width, half_height);
      insert_renderable(face);
    }
}

static
void
draw_cube(void)
{
  u32 i;

  for(i = 0; i < renderable_count; i++)
    {
      Face *face;

      face = renderables[i];
      MapCel(face->ccb, face->screen);
      LAST_CEL(face->ccb);
      display_draw_cels(face->ccb);
    }
}

static
void
init_cube(Cube *cube,
          u32   color_scheme)
{
  u32 face_index;
  CCB *shared_data_cel;

  shared_data_cel = NULL;

  for(face_index = 0; face_index < FACE_COUNT; face_index++)
    {
      Face *face;
      u32 vert_index;
      u16 color;

      face = &cube->faces[face_index];

      for(vert_index = 0; vert_index < FACE_VERTS; vert_index++)
        face->indices[vert_index] = cube_faces[face_index][vert_index];

      if(color_scheme == CUBE_COLORS_3DO_LOGO)
        {
          color = face_colors_3do_logo[face_index];
          face->ccb = create_colored_face_cel(color, shared_data_cel);
          face->pmv = 7;
        }
      else if(color_scheme == CUBE_TEXTURED_3DO_LOGO)
        {
          face->ccb = create_textured_face_cel(shared_data_cel);
          face->pmv = solid_face_pmv[face_index];
        }
      else
        {
          color = solid_cube_colors[color_scheme];
          face->ccb = create_colored_face_cel(color, shared_data_cel);
          face->pmv = solid_face_pmv[face_index];
        }

      if(shared_data_cel == NULL)
        shared_data_cel = face->ccb;

      set_pixc_pmv(face->ccb, face->pmv);
      face->z = 0;
    }
}

static
void
init_cubes(void)
{
  u32 color_scheme;

  for(color_scheme = 0; color_scheme < CUBE_COLOR_SCHEME_COUNT; color_scheme++)
    init_cube(&cubes[color_scheme], color_scheme);

  active_cube = CUBE_COLORS_3DO_LOGO;
}

static
void
cleanup_cubes(void)
{
  u32 color_scheme;
  u32 face_index;

  for(color_scheme = 0; color_scheme < CUBE_COLOR_SCHEME_COUNT; color_scheme++)
    for(face_index = 0; face_index < FACE_COUNT; face_index++)
      cubes[color_scheme].faces[face_index].ccb = DeleteCel(cubes[color_scheme].faces[face_index].ccb);
}

static
boolean
check_control_pad(void)
{
  uint32 buttons;
  Err err;

  buttons = 0;
  err = DoControlPad(1, &buttons, 0);
  if(err < 0)
    abort_err(err);

  if(buttons & SWITCH_BUTTONS)
    active_cube = (active_cube + 1) % CUBE_COLOR_SCHEME_COUNT;

  return ((buttons & ControlX) ? TRUE : FALSE);
}

int
main(void)
{
  frac16 xangle = 0;
  frac16 yangle = 0;
  frac16 zangle = 0;

  display_init();
  control_pad_init();
  OpenMathFolio();
  init_cubes();

  display_clear(0);
  display_swap();

  while(TRUE)
    {
      xangle += Convert32_F16(1) >> 1;
      yangle += Convert32_F16(1);
      zangle += Convert32_F16(1) >> 2;

      if(xangle >= ANG_256_F16)
        xangle -= ANG_256_F16;
      if(yangle >= ANG_256_F16)
        yangle -= ANG_256_F16;
      if(zangle >= ANG_256_F16)
        zangle -= ANG_256_F16;

      if(check_control_pad())
        break;

      update_cube(&cubes[active_cube], xangle, yangle, zangle);

      display_clear(0x00000000);
      if(renderable_count > 0)
        draw_cube();

      display_display_and_swap();
      display_waitvbl();
    }

  cleanup_cubes();
  control_pad_cleanup();
  display_cleanup();

  return 0;
}

/*
  Rotating cube demo for the 3DO cel engine.

  The program renders a perspective-projected cube by mapping one cel onto each
  visible face, sorting faces back-to-front, and redrawing the animation each
  VBL. Press A, B, or C to cycle between 3DO logo CELs, red, blue,
  yellow, a 3DO logo CEL, and a Jaguar CEL.
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

#define PMV_0_MASK  0x1C00
#define PDV_0_MASK  0x0300
#define PMV_0_SHIFT 10
#define PDV_0_SHIFT 8

typedef frac16 vector3[3];

typedef struct Vertex
{
  frac16 x;
  frac16 y;
  frac16 z;
} Vertex;

typedef struct Rotation
{
  frac16 m00, m01, m02;
  frac16 m10, m11, m12;
  frac16 m20, m21, m22;
} Rotation;

typedef struct Face
{
  u32    indices[FACE_VERTS];
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

enum CubeCelSet
{
  CUBE_CELS_3DO_LOGO,
  CUBE_CELS_ALL_RED,
  CUBE_CELS_ALL_BLUE,
  CUBE_CELS_ALL_YELLOW,
  CUBE_CELS_TEXTURED_3DO_LOGO,
  CUBE_CELS_TEXTURED_JAG,
  CUBE_CEL_SET_COUNT
};

/* PMV values shade each face so cube sides stay visually distinct. */
static const u32 face_pmv[FACE_COUNT] = { 7, 4, 5, 6, 3, 2 };

static const char *cube_face_cel_paths[CUBE_CEL_SET_COUNT][FACE_COUNT] =
  {
    {
      "cels/red_16x16.cel",
      "cels/red_16x16.cel",
      "cels/blue_16x16.cel",
      "cels/blue_16x16.cel",
      "cels/yellow_16x16.cel",
      "cels/yellow_16x16.cel"
    },
    {
      "cels/red_16x16.cel",
      "cels/red_16x16.cel",
      "cels/red_16x16.cel",
      "cels/red_16x16.cel",
      "cels/red_16x16.cel",
      "cels/red_16x16.cel"
    },
    {
      "cels/blue_16x16.cel",
      "cels/blue_16x16.cel",
      "cels/blue_16x16.cel",
      "cels/blue_16x16.cel",
      "cels/blue_16x16.cel",
      "cels/blue_16x16.cel"
    },
    {
      "cels/yellow_16x16.cel",
      "cels/yellow_16x16.cel",
      "cels/yellow_16x16.cel",
      "cels/yellow_16x16.cel",
      "cels/yellow_16x16.cel",
      "cels/yellow_16x16.cel"
    },
    {
      "Art/3do_logo_unpacked.cel",
      "Art/3do_logo_unpacked.cel",
      "Art/3do_logo_unpacked.cel",
      "Art/3do_logo_unpacked.cel",
      "Art/3do_logo_unpacked.cel",
      "Art/3do_logo_unpacked.cel"
    },
    {
      "cels/jag_128x128.cel",
      "cels/jag_128x128.cel",
      "cels/jag_128x128.cel",
      "cels/jag_128x128.cel",
      "cels/jag_128x128.cel",
      "cels/jag_128x128.cel"
    }
  };

static Cube cubes[CUBE_CEL_SET_COUNT];
static Vertex world_vertices[VERT_COUNT];
static Point screen_vertices[VERT_COUNT];
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
display_clear(int value)
{
  SetVRAMPages(vram_ioreq,
               sc->sc_Bitmaps[current_screen]->bm_Buffer,
               value,
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
CCB*
create_face_cel(const char *path,
                CCB        *shared_data_cel)
{
  CCB *cel;

  if(shared_data_cel == NULL)
    cel = LoadCel((char*)path, MEMTYPE_CEL);
  else
    cel = CloneCel(shared_data_cel, CLONECEL_CCB_ONLY);

  if(cel == NULL)
    abort("unable to load cube cel");

  cel->ccb_Flags |= CCB_ALSC | CCB_LDPPMP | CCB_NPABS;
  cel->ccb_NextPtr = NULL;
  set_pixc_pdv(cel, 3);

  return cel;
}

static
void
build_rotation(Rotation *rot,
               frac16    xangle,
               frac16    yangle,
               frac16    zangle)
{
  frac16 cx, sx;
  frac16 cy, sy;
  frac16 cz, sz;
  frac16 sxsy;
  frac16 cxsy;

  cx = CosF16(xangle);
  sx = SinF16(xangle);
  cy = CosF16(yangle);
  sy = SinF16(yangle);
  cz = CosF16(zangle);
  sz = SinF16(zangle);

  sxsy = MulSF16(sx, sy);
  cxsy = MulSF16(cx, sy);

  rot->m00 = MulSF16(cy, cz);
  rot->m01 = MulSF16(cy, sz);
  rot->m02 = -sy;

  rot->m10 = MulSF16(sxsy, cz) - MulSF16(cx, sz);
  rot->m11 = MulSF16(sxsy, sz) + MulSF16(cx, cz);
  rot->m12 = MulSF16(sx, cy);

  rot->m20 = MulSF16(cxsy, cz) + MulSF16(sx, sz);
  rot->m21 = MulSF16(cxsy, sz) - MulSF16(sx, cz);
  rot->m22 = MulSF16(cx, cy);
}

static
void
transform_vertex(Vertex       *out,
                 const Vertex *in,
                 const Rotation *rot,
                 frac16        world_z)
{
  const frac16 x = in->x;
  const frac16 y = in->y;
  const frac16 z = in->z;

  out->x = MulSF16(x, rot->m00) + MulSF16(y, rot->m10) + MulSF16(z, rot->m20);
  out->y = MulSF16(x, rot->m01) + MulSF16(y, rot->m11) + MulSF16(z, rot->m21);
  out->z = MulSF16(x, rot->m02) + MulSF16(y, rot->m12) + MulSF16(z, rot->m22) + world_z;
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
cull_face_vertices(const Vertex *v0,
                   const Vertex *v1,
                   const Vertex *v3)
{
  vector3 pov, edge1, edge2, normal;

  pov[0] = -v0->x;
  pov[1] = -v0->y;
  pov[2] = -v0->z;

  point_to_vector(edge1, v0, v1);
  point_to_vector(edge2, v0, v3);
  Cross3_F16(normal, edge2, edge1);

  return ((Dot3_F16(normal, pov) > 0) ? TRUE : FALSE);
}

static
void
project_vertex(Point        *screen,
               const Vertex *world,
               frac16        half_width,
               frac16        half_height)
{
  frac16 z;

  z = world->z == 0 ? ONE_F16 : world->z;

  screen->pt_X = (DivSF16(world->x << VIEW_SHIFT, z) + half_width) >> FRACBITS_16;
  screen->pt_Y = (DivSF16(-world->y << VIEW_SHIFT, z) + half_height) >> FRACBITS_16;
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
  Rotation rot;
  u32 face_index;
  u32 vert_index;

  renderable_count = 0;
  build_rotation(&rot, xangle, yangle, zangle);

  /* 1. Rotate and project the 8 unique cube vertices. */
  for(vert_index = 0; vert_index < VERT_COUNT; vert_index++)
    {
      transform_vertex(&world_vertices[vert_index],
                       &cube_vertices[vert_index],
                       &rot,
                       world_z);
      project_vertex(&screen_vertices[vert_index],
                     &world_vertices[vert_index],
                     half_width,
                     half_height);
    }

  /* 2. Cull faces, then copy projected points for only the visible ones. */
  for(face_index = 0; face_index < FACE_COUNT; face_index++)
    {
      Face *face;
      const u32 *indices;
      frac16 zsum;

      face = &cube->faces[face_index];
      indices = face->indices;

      if(cull_face_vertices(&world_vertices[indices[0]],
                            &world_vertices[indices[1]],
                            &world_vertices[indices[3]]))
        continue;

      zsum = 0;
      for(vert_index = 0; vert_index < FACE_VERTS; vert_index++)
        {
          u32 index;

          index = indices[vert_index];
          face->screen[vert_index].pt_X = screen_vertices[index].pt_X;
          face->screen[vert_index].pt_Y = screen_vertices[index].pt_Y;
          zsum += world_vertices[index].z;
        }

      face->z = zsum >> 2;
      insert_renderable(face);
    }
}

static
void
draw_cube(void)
{
  u32 i;

  if(renderable_count == 0)
    return;

  for(i = 0; i < renderable_count; i++)
    {
      Face *face;
      CCB *ccb;

      face = renderables[i];
      ccb = face->ccb;

      MapCel(ccb, face->screen);
      if(i + 1 < renderable_count)
        {
          ccb->ccb_NextPtr = renderables[i + 1]->ccb;
          UNLAST_CEL(ccb);
        }
      else
        {
          ccb->ccb_NextPtr = NULL;
          LAST_CEL(ccb);
        }
    }

  display_draw_cels(renderables[0]->ccb);
}

static
void
init_cube(Cube *cube,
          u32   cel_set)
{
  u32 face_index;

  for(face_index = 0; face_index < FACE_COUNT; face_index++)
    {
      Face *face;
      u32 vert_index;
      CCB *shared_data_cel;
      const char *path;

      face = &cube->faces[face_index];
      path = cube_face_cel_paths[cel_set][face_index];

      for(vert_index = 0; vert_index < FACE_VERTS; vert_index++)
        face->indices[vert_index] = cube_faces[face_index][vert_index];

      shared_data_cel = NULL;
      for(vert_index = 0; vert_index < face_index; vert_index++)
        {
          if(strcmp(path, cube_face_cel_paths[cel_set][vert_index]) == 0)
            {
              shared_data_cel = cube->faces[vert_index].ccb;
              break;
            }
        }

      face->ccb = create_face_cel(path, shared_data_cel);
      if(cel_set == CUBE_CELS_3DO_LOGO)
        face->pmv = 7;
      else
        face->pmv = face_pmv[face_index];

      set_pixc_pmv(face->ccb, face->pmv);
      face->z = 0;
    }
}

static
void
init_cubes(void)
{
  u32 cel_set;

  for(cel_set = 0; cel_set < CUBE_CEL_SET_COUNT; cel_set++)
    init_cube(&cubes[cel_set], cel_set);

  active_cube = CUBE_CELS_3DO_LOGO;
}

static
void
cleanup_cubes(void)
{
  u32 cel_set;
  u32 face_index;

  for(cel_set = 0; cel_set < CUBE_CEL_SET_COUNT; cel_set++)
    for(face_index = 0; face_index < FACE_COUNT; face_index++)
      cubes[cel_set].faces[face_index].ccb = DeleteCel(cubes[cel_set].faces[face_index].ccb);
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
    active_cube = (active_cube + 1) % CUBE_CEL_SET_COUNT;

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

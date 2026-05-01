/*
 * @author Erick Marquez - https://www.youtube.com/@retroloveletter
 * Modifications by trapexit <trapexit@spawn.link>
 *
 * @brief Simple 3D demo. This program will generate three 3D models
 * which make up a 3DO logo.
 *
 * Each logo model can rotate. This program copies prestine vertices
 * into quad local vertices before vertex manipulation based on stored
 * angles. This prevents vertices from becoming distorted due to
 * rounding errors after rotations by applying a reset. Obviously,
 * based on project needs and optimizations, this can be handled in
 * other ways.
 *
 * Backface culling support has been added. This code uses the first
 * vertex on each quad during this calculation to obtain a point on
 * the plane. For improved results when dealing with complex shapes
 * and angles, you can grab the center of each quad instead.
 *
 * There are various optimizations that can be made. The goal was to
 * keep this "simple" for demo purposes.
 *
 * The "camera" in this program is fixed, looking down into the
 * positive Z direction. If a camera is added then updates to the
 * world transform need to be made. You can keep track of a global
 * view matrix in this case.
 *
 * Lighting for non-wireframe rendering is rudimentary and currently
 * only works if backface culling is not disabled as it uses a dot
 * product calculated there.
 *
 * The word 'object' in this code is synonymous with 3D model.
 *
 * This has been tested on hardware and runs a bit slower there
 * compared to RetroArch->Opera.
 *
 * @date 2024-05-30
 */

#include "celutils.h"
#include "abort.h"
#include "controlpad.h"
#include "displayutils.h"
#include "event.h"
#include "mem.h"
#include "operamath.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

/* ********************************************************************************************* */
/* ------------------------------------- CONSTANTS / MACROS ------------------------------------ */
/* ********************************************************************************************* */

#define SIZEOF_ARRAY(X) (sizeof(X)/sizeof((X)[0]))

// settings
#define DEBUG_MODE        0
#define POLY_SIZE         4
#define SOLID_OBJ_COUNT   3
#define WIRE_OBJ_COUNT    3
#define HALF_SCREEN_W_F16 Convert32_F16(320/2)
#define HALF_SCREEN_H_F16 Convert32_F16(240/2)
#define MAX_OBJECT_VERTS  80
#define MAX_OBJECT_QUADS  20
#define VIEW_DIST_SHIFT   9
#define MAX_RENDERABLES   80

// added for context
#define FRACBITS_16 16
#define ONE_F16     Convert32_F16(1)
#define ANG_256_F16 Convert32_F16(256)

// flags
#define CULL_MASK 1

// cel helpers
#define LINK_CEL(a, b) (a->ccb_NextPtr = b)
#define CALC_WOFFSET_BPP8(width) ( (width % 4 == 0) ? (width / 4) : (width / 4 + 1) )
#define CLEAR_CEL_PREAMBLES(ccb) (ccb->ccb_PRE0 = ccb->ccb_PRE1 = 0)
#define SET_CEL_BPP8(ccb) (ccb->ccb_PRE0 |= (PRE0_BPP_8 << PRE0_BPP_SHIFT))
#define SET_CEL_WOFFSET10(ccb, width) (ccb->ccb_PRE1 |= ((width - PRE1_WOFFSET_PREFETCH) << PRE1_WOFFSET10_SHIFT))
#define SET_CEL_TLHPCNT(ccb, width) (ccb->ccb_PRE1 |= ((width - PRE1_TLHPCNT_PREFETCH) << PRE1_TLHPCNT_SHIFT))
#define SET_CEL_VCNT(ccb, height) (ccb->ccb_PRE0 |= ((height - PRE0_VCNT_PREFETCH) << PRE0_VCNT_SHIFT))
#define GET_CEL_BPP_VALUE(ccb) ((ccb->ccb_PRE0 & PRE0_BPP_MASK) >> PRE0_BPP_SHIFT)
#define PMV_0_MASK 0x1C00
#define PDV_0_MASK 0x300
#define PMV_0_SHIFT 10
#define PDV_0_SHIFT 8
#define LIGHT_STATE_UNSET 0xFFFFFFFF

/* ********************************************************************************************* */
/* ------------------------------------------- TYPES ------------------------------------------- */
/* ********************************************************************************************* */

typedef frac16 vector3[3];
typedef vector3 angle3;

typedef struct
{
  frac16 x;
  frac16 y;
  frac16 z;
} vertex, vector, point;

// Polys that make up models
typedef struct
{
  u32     vertIndex[POLY_SIZE];
  Point   screen[POLY_SIZE]; 	// Point is a 3DO lib type, handy for MapCel()
  u32     flags;
  CCB    *ccb;			// Used if not using wireframe mode
  frac16  dp;			// Holds dot product of quad normal to pov
  u32     lightState;
} quad, *quadPtr;

// Models
typedef struct
{
  u16    color;
  u32    numVerts;		// number of distinct vertices
  u32    numQuads;		// number of quads
  point  worldPos;		// object center world position
  vertex verts[MAX_OBJECT_VERTS];
  vertex worldVerts[MAX_OBJECT_VERTS];
  Point  screenVerts[MAX_OBJECT_VERTS];
  quad   quads[MAX_OBJECT_QUADS]; // quad/polygon vertex data
  angle3 angles;		// xyz rotation angles
  angle3 spin_velocity;		// per-axis angular velocity
} object, *objectPtr;

/* ************************************************************************* */
/* ---------------------------------- GLOBALS ------------------------------ */
/* ************************************************************************* */

static Item sport_ioreq;
static Item vbl_ioreq;
static ScreenContext *sc;
static objectPtr solid_objs[SOLID_OBJ_COUNT];
static objectPtr wire_objs_left[WIRE_OBJ_COUNT];
static objectPtr wire_objs_right[WIRE_OBJ_COUNT];
static const ubyte bppTable[] = {0, 1, 2, 4, 6, 8, 16, 0};
static CCB *renderables[MAX_RENDERABLES];
static u32 rend_count;

/* ------------------------------------------------------------------------- */
/* --------------------------- HARDCODED MODEL DATA ------------------------ */
/* ------------------------------------------------------------------------- */

// 16:16 fixed point format
// This data defines the 3DO logo in three parts
// Postfix naming convention is:
// 		_3 = (3)DO
// 		_d = 3(D)O
// 		_o = 3D(O)

// first model
const
static
vertex
verts_3[] =
  {
    {55609,      0,   31457},
    {0,     -55609,   31457},
    {55609,      0,  -31457},
    {0,     -55609,  -31457},
    {0,	     55609,   31457},
    {-55609,     0,   31457},
    {0,	     55609,  -31457},
    {-55609,     0,  -31457}
  };

static
const
u32
quads_3[] =
  {
    0, 4, 6, 2,
    3, 2, 6, 7,
    7, 6, 4, 5,
    5, 1, 3, 7,
    1, 0, 2, 3,
    5, 4, 0, 1
  };

// second model
static
const
vertex
verts_d[] =
  {
    {-52428, -46137, -31457},
    {-52428,  46137, -31457},
    {-52428, -46137,  31457},
    {-52428,  46137,  31457},
    {52428,  -46137, -31457},
    {52428,   46137, -31457},
    {52428,  -46137,  31457},
    {52428,   46137,  31457}
  };

static
const
u32
quads_d[] =
  {
    0, 1, 3, 2,
    2, 3, 7, 6,
    6, 7, 5, 4,
    4, 5, 1, 0,
    2, 6, 4, 0,
    7, 3, 1, 5
  };

// third model
static
const
vertex
verts_o[] =
  {
    {-7,      37538,  37559},
    {-7,          2,  53107},
    {-7,     -37534,  37559},
    {32499,   37538,  18791},
    {45964,       2,  26565},
    {32499,  -37534,  18791},
    {32499,   37538, -18744},
    {45964,       2, -26518},
    {32499,  -37534, -18744},
    {-8,      37538, -37512},
    {-8,          2, -53060},
    {-8,     -37534, -37512},
    {-686,   -52905,   2034},
    {-7,      53086,     23},
    {-32515,  37538, -18744},
    {-45980,      2, -26518},
    {-32515, -37534, -18744},
    {-32515,  37538,  18791},
    {-45980,      2,  26565},
    {-32515, -37534,  18791}
  };

static
const
u32
quads_o[] =
  {
    2, 1, 4, 5,
    1, 0, 3, 4,
    13, 6, 3, 0,
    4, 3, 6, 7,
    5, 4, 7, 8,
    2, 5, 12, 19,
    7, 6, 9, 10,
    8, 7, 10, 11,
    11, 10, 15, 16,
    5, 8, 11, 12,
    10, 9, 14, 15,
    16, 15, 18, 19,
    16, 19, 12, 11,
    13, 14, 9, 6,
    15, 14, 17, 18,
    19, 18, 1, 2,
    18, 17, 0, 1,
    13, 0, 17, 14
  };

/* ************************************************************************* */
/* -------------------------------- PROTOTYPES ----------------------------- */
/* ************************************************************************* */

/**
 * @brief Basic app setup
 */
static void init_demo(void);

/**
 * @brief Create a 3D Object
 * @param verts Pointer to linear vertex list
 * @param numVerts Vertex count (xyz groups) in list, not linear list size
 * @param quads Pointer to linear quad list
 * @param numQuads Quad count (sets of 4) in list, not linear list size
 * @param color Composite 3DO RGB color value
 * @return objectPtr
 */
static objectPtr create_object(const vertex *verts,
                               const u32     numVerts,
                               const u32    *quads,
                               const u32     numQuads,
                               u16           color);

static objectPtr clone_obj(const objectPtr);

/**
 * @brief Helper to set object position
 * @param obj
 * @param x
 * @param y
 * @param z
 */
static void set_object_pos(objectPtr obj, frac16 x, frac16 y, frac16 z);

/**
 * @brief Updates 3D scene
 */
static void update_solid_objs(void);
static void update_wire_objs(void);

static void local_to_world(objectPtr obj);

/**
 * @brief Transform model world space to screen space
 * @param obj
 */
static void world_to_screen(objectPtr obj);

/**
 * @brief Draw model in wireframe
 *
 * @param obj
 */
static void render_wire_obj(objectPtr obj, const u16 color);

static void rotate_object_y_to_world(objectPtr obj, frac16 ycos, frac16 ysin);

static void rotate_object_xyz_to_world(objectPtr obj);

/**
 * @brief Test quad for backface removal. This check will set the quad's cull
 * flag if it should be hidden, along with returning the check result for convenience.
 *
 * @param qptr
 * @return TRUE if cull was set
 */
static boolean cull_backface(objectPtr optr, quadPtr qptr);

static u32 pmv_from_dp(frac16 dp, u32 quad_idx);

/**
 * @brief Prep to render each quad as a linked cel
 */
static void prepare_renderables(void);

/**
 * @brief Cel creation helper
 * @param width
 * @param height
 * @param color Composite 3DO RGB color value
 * @return CCB*
 */
static CCB *create_coded_colored_cel8(u32 width, u32 height, u16 color);

/**
 * @brief Cel data helper
 * @param cel Cel to read source bytes from
 * @return i32
 */
static size_t get_cel_source_bytes(CCB *cel);

/**
 * @brief Cel data helper
 * @param cel Cel to read row bytes from
 * @return i32
 */
static size_t get_cel_row_bytes(CCB *cel);

/**
 * @brief Cel data helper
 * @param cel Cel to read bpp from
 * @return i32
 */
static i32 get_cel_bpp(CCB *cel);

/**
 * @brief Dumps object data for debugging
 *
 * @param obj
 */
static void dump_object(objectPtr obj);


/* ********************************************************************************************* */
/* ------------------------------------------- MAIN -------------------------------------------- */
/* ********************************************************************************************* */

#define RGB15_RED    ((u16)MakeRGB15(31,0,0))
#define RGB15_BLUE   ((u16)MakeRGB15(0,0,31))
#define RGB15_YELLOW ((u16)MakeRGB15(31,31,0))
#define RGB15_BLACK  ((u16)MakeRGB15(0,0,0))

static
void
create_objects(void)
{
  // "3" model
  solid_objs[0] = create_object(verts_3,
                                SIZEOF_ARRAY(verts_3),
                                quads_3,
                                SIZEOF_ARRAY(quads_3)/4,
                                RGB15_RED);
  set_object_pos(solid_objs[0],
                 Convert32_F16(0),
                 Convert32_F16(2),
                 Convert32_F16(18));


  // "D" model
  solid_objs[1] = create_object(verts_d,
                                SIZEOF_ARRAY(verts_d),
                                quads_d,
                                SIZEOF_ARRAY(quads_d)/4,
                                RGB15_BLUE);
  set_object_pos(solid_objs[1],
                 Convert32_F16(0),
                 Convert32_F16(0),
                 Convert32_F16(18));

  // "O" model
  solid_objs[2] = create_object(verts_o,
                                SIZEOF_ARRAY(verts_o),
                                quads_o,
                                SIZEOF_ARRAY(quads_o)/4,
                                RGB15_YELLOW);
  set_object_pos(solid_objs[2],
                 Convert32_F16(0),
                 Convert32_F16(-2),
                 Convert32_F16(18));

  {
    u32 s, a;

    for(s = 0; s < SOLID_OBJ_COUNT; s++)
      for(a = 0; a < 3; a++)
        {
          signed int r = ((signed int)urand() & 7) - 3; /* -3..+4 */
          if(r == 0)
            r = 1;
          solid_objs[s]->spin_velocity[a] = (frac16)((int32)r * 0x4000);
        }
  }

  // "3" model
  wire_objs_left[0] = clone_obj(solid_objs[0]);
  set_object_pos(wire_objs_left[0],
                 Convert32_F16(3),
                 Convert32_F16(2),
                 Convert32_F16(18));

  // "D" model
  wire_objs_left[1] = clone_obj(solid_objs[1]);
  set_object_pos(wire_objs_left[1],
                 Convert32_F16(3),
                 Convert32_F16(0),
                 Convert32_F16(18));

  // "O" model
  wire_objs_left[2] = clone_obj(solid_objs[2]);
  set_object_pos(wire_objs_left[2],
                 Convert32_F16(3),
                 Convert32_F16(-2),
                 Convert32_F16(18));

  // "3" model
  wire_objs_right[0] = clone_obj(solid_objs[0]);
  set_object_pos(wire_objs_right[0],
                 Convert32_F16(-3),
                 Convert32_F16(2),
                 Convert32_F16(18));

  // "D" model
  wire_objs_right[1] = clone_obj(solid_objs[1]);
  set_object_pos(wire_objs_right[1],
                 Convert32_F16(-3),
                 Convert32_F16(0),
                 Convert32_F16(18));

  // "O" model
  wire_objs_right[2] = clone_obj(solid_objs[2]);
  set_object_pos(wire_objs_right[2],
                 Convert32_F16(-3),
                 Convert32_F16(-2),
                 Convert32_F16(18));
}

static
void
delete_object(objectPtr obj_,
              boolean   owns_cels_)
{
  u32 i;

  if(obj_ == NULL)
    return;

  if(owns_cels_)
    for(i = 0; i < obj_->numQuads; i++)
      obj_->quads[i].ccb = DeleteCel(obj_->quads[i].ccb);

  FreeMem(obj_, sizeof(object));
}

static
void
delete_objects(void)
{
  u32 i;

  for(i = 0; i < SIZEOF_ARRAY(wire_objs_left); i++)
    {
      delete_object(wire_objs_left[i], FALSE);
      wire_objs_left[i] = NULL;
    }

  for(i = 0; i < SIZEOF_ARRAY(wire_objs_right); i++)
    {
      delete_object(wire_objs_right[i], FALSE);
      wire_objs_right[i] = NULL;
    }

  for(i = 0; i < SIZEOF_ARRAY(solid_objs); i++)
    {
      delete_object(solid_objs[i], TRUE);
      solid_objs[i] = NULL;
    }
}

static
void
render_wire_objs(objectPtr objs_[],
                 const u32 objs_count_)
{
  u32 i;

  for(i = 0; i < objs_count_; i++)
    render_wire_obj(objs_[i],
                    objs_[i]->color);
}

static
void
clear_screen(void)
{
  SetVRAMPages(sport_ioreq,
               sc->sc_Bitmaps[sc->sc_CurrentScreen]->bm_Buffer,
               0,
                sc->sc_NumBitmapPages,
                0xFFFFFFFF);
}

static
void
display_cleanup(void)
{
  if(vbl_ioreq > 0)
    DeleteVBLIOReq(vbl_ioreq);
  if(sport_ioreq > 0)
    DeleteVRAMIOReq(sport_ioreq);
  if(sc != NULL)
    {
      DeleteBasicDisplay(sc);
      FreeMem(sc, sizeof(ScreenContext));
    }

  vbl_ioreq = 0;
  sport_ioreq = 0;
  sc = NULL;
}

static
boolean
exit_requested(void)
{
  uint32 buttons;
  Err err;

  buttons = 0;
  err = DoControlPad(1, &buttons, 0);
  if(err < 0)
    abort_err(err);

  return ((buttons & ControlX) ? TRUE : FALSE);
}

int
main_3d_3do_logo(void)
{
  Err err;

  init_demo();

  err = InitControlPad(1);
  if(err < 0)
    abort_err(err);

  create_objects();

  while(TRUE)
    {
      if(exit_requested())
        break;

      rend_count = 0;

      clear_screen();

      update_wire_objs();
      render_wire_objs(wire_objs_left,WIRE_OBJ_COUNT);
      render_wire_objs(wire_objs_right,WIRE_OBJ_COUNT);

      // render using cels proper
      update_solid_objs();
      prepare_renderables();

      if(rend_count > 0)
        DrawCels(sc->sc_BitmapItems[sc->sc_CurrentScreen], renderables[0]);

#if DEBUG_MODE
      {
        int i;
        for(i = 0; i < SIZEOF_ARRAY(solid_objs); i++)
          render_wire_obj(solid_objs[i], RGB15_BLACK);
      }
#endif

      DisplayScreen(sc->sc_ScreenItems[sc->sc_CurrentScreen], NULL);
      // flip current screen
      sc->sc_CurrentScreen = 1 - sc->sc_CurrentScreen;

      WaitVBL(vbl_ioreq, 1);
    }

  delete_objects();
  KillControlPad();
  display_cleanup();

  return(0);
}

/* ************************************************************************* */
/* --------------------- FUNCTION IMPLEMENTATION --------------------------- */
/* ************************************************************************* */

static
void
init_demo(void)
{
  u32 display_type;

  // folios
  OpenGraphicsFolio();
  OpenMathFolio();

  // screen
  sc = (ScreenContext*) AllocMem(sizeof(ScreenContext), MEMTYPE_ANY);

  QueryGraphics(QUERYGRAF_TAG_DEFAULTDISPLAYTYPE, &display_type);

  if((display_type == DI_TYPE_PAL1) || (display_type == DI_TYPE_PAL2))
    display_type = DI_TYPE_PAL1;
  else
    display_type = DI_TYPE_NTSC;

  CreateBasicDisplay(sc, display_type, 2);
  sc->sc_CurrentScreen = 0;

  // IO request Items
  sport_ioreq = GetVRAMIOReq();
  vbl_ioreq   = GetVBLIOReq();
}

static
frac16
clamp_angle(frac16 angle_)
{
  if(angle_ >= ANG_256_F16)
    angle_ -= ANG_256_F16;

  return angle_;
}

static
void
update_obj_y_group(objectPtr objs_[],
                   u32       objs_count_,
                   frac16    y_rot_)
{
  u32 i;
  frac16 yangle;
  frac16 ycos;
  frac16 ysin;

  yangle = clamp_angle(objs_[0]->angles[1] + y_rot_);
  ycos = CosF16(yangle);
  ysin = SinF16(yangle);

  for(i = 0; i < objs_count_; i++)
    {
      objs_[i]->angles[1] = yangle;
      rotate_object_y_to_world(objs_[i], ycos, ysin);
      local_to_world(objs_[i]);
      world_to_screen(objs_[i]);
    }
}

static
void
update_wire_objs(void)
{
  update_obj_y_group(wire_objs_left,
                     SIZEOF_ARRAY(wire_objs_left),
                     Convert32_F16(1));

  update_obj_y_group(wire_objs_right,
                     SIZEOF_ARRAY(wire_objs_right),
                     Convert32_F16(-1));
}


static
void
update_solid_objs(void)
{
  u32 i;

  for(i = 0; i < SIZEOF_ARRAY(solid_objs); i++)
    {
      objectPtr obj;

      obj = solid_objs[i];

      obj->angles[0] = clamp_angle(obj->angles[0] + obj->spin_velocity[0]);
      obj->angles[1] = clamp_angle(obj->angles[1] + obj->spin_velocity[1]);
      obj->angles[2] = clamp_angle(obj->angles[2] + obj->spin_velocity[2]);

      rotate_object_xyz_to_world(obj);
      local_to_world(obj);
      world_to_screen(obj);
    }
}

static
void
prepare_renderables(void)
{
  u32 i, j;
  quadPtr qptr;
  objectPtr optr;

  for (i = 0; i < SIZEOF_ARRAY(solid_objs); i++)
    {
      optr = solid_objs[i];

      for (j = 0; j < optr->numQuads; j++)
        {
          qptr = &optr->quads[j];

          if(qptr->flags & CULL_MASK)
            continue;

          if(rend_count >= MAX_RENDERABLES)
            {
              rend_count = MAX_RENDERABLES;
              goto hard_stop;
            }

          // shade based on face angle to pov
          {
            const u32 pmv = pmv_from_dp(qptr->dp, j);

            if(qptr->lightState != pmv)
              {
                qptr->ccb->ccb_PIXC =
                  (qptr->ccb->ccb_PIXC & ~(PMV_0_MASK | PDV_0_MASK)) |
                  (pmv << PMV_0_SHIFT) |
                  (3 << PDV_0_SHIFT);
                qptr->lightState = pmv;
              }
          }

          MapCel(qptr->ccb, qptr->screen);
          renderables[rend_count++] = qptr->ccb;
        }
    }

 hard_stop:

  // link cels
  if(rend_count > 0)
    {
      for (i = 0; i < rend_count - 1; i++)
        {
          LINK_CEL(renderables[i], renderables[i+1]);
          UNLAST_CEL(renderables[i]);
        }

      LAST_CEL(renderables[rend_count-1]);
    }
}

static
void
local_to_world(objectPtr obj)
{
  u32 i;
  quadPtr qptr;

  for (i = 0; i < obj->numQuads; i++)
    {
      qptr = &obj->quads[i];
      qptr->flags = cull_backface(obj, qptr) ? CULL_MASK : 0;
    }
}

static
void
world_to_screen(objectPtr obj)
{
  u32 i, j;
  u32 idx;
  frac16 z;
  quadPtr qptr;
  boolean projected[MAX_OBJECT_VERTS];

  memset(projected, FALSE, sizeof(projected));

  for (i = 0; i < obj->numQuads; i++)
    {
      qptr = &obj->quads[i];

      if(qptr->flags & CULL_MASK)
        continue;

      for (j = 0; j < POLY_SIZE; j++)
        {
          idx = qptr->vertIndex[j];

          if(!projected[idx])
            {
              z = (obj->worldVerts[idx].z == 0) ? 1 : obj->worldVerts[idx].z; // avoid divide by zero
              obj->screenVerts[idx].pt_X = (DivSF16(obj->worldVerts[idx].x << VIEW_DIST_SHIFT, z) + HALF_SCREEN_W_F16) >> FRACBITS_16;
              obj->screenVerts[idx].pt_Y = (DivSF16(-obj->worldVerts[idx].y << VIEW_DIST_SHIFT, z) + HALF_SCREEN_H_F16) >> FRACBITS_16;
              projected[idx] = TRUE;
            }

          qptr->screen[j].pt_X = obj->screenVerts[idx].pt_X;
          qptr->screen[j].pt_Y = obj->screenVerts[idx].pt_Y;
        }
    }
}

static
void
render_wire_obj(objectPtr obj_,
                const u16 color_)
{
  static GrafCon gcon;
  Item bitmapItem;
  u32 i, j;
  quadPtr qptr;

  bitmapItem = sc->sc_BitmapItems[sc->sc_CurrentScreen];

  SetFGPen(&gcon, color_);

  for (i = 0; i < obj_->numQuads; i++)
    {
      qptr = &obj_->quads[i];

      if(qptr->flags & CULL_MASK)
        continue;

      gcon.gc_PenX = qptr->screen[0].pt_X;
      gcon.gc_PenY = qptr->screen[0].pt_Y;

      for (j = 1; j < POLY_SIZE; j++)
        DrawTo(bitmapItem,
               &gcon,
               qptr->screen[j].pt_X,
               qptr->screen[j].pt_Y);

      DrawTo(bitmapItem,
             &gcon,
             qptr->screen[0].pt_X,
             qptr->screen[0].pt_Y);
    }
}


static
objectPtr
clone_obj(const objectPtr obj_)
{
  objectPtr obj;

  obj = (objectPtr)AllocMem(sizeof(*obj),MEMTYPE_DRAM);
  if(obj == NULL)
    return NULL;

  memcpy(obj,obj_,sizeof(object));

  return obj;
}


static
objectPtr
create_object(const vertex *verts,
              const u32     numVerts,
              const u32    *quads,
              const u32     numQuads,
              u16           color)
{
  u32 i;
  objectPtr obj;

  obj = (objectPtr) AllocMem(sizeof(object), MEMTYPE_DRAM);

  obj->numVerts  = numVerts;
  obj->numQuads  = numQuads;
  obj->color     = color;
  obj->angles[0] = obj->angles[1] = obj->angles[2] = 0;
  obj->spin_velocity[0] = obj->spin_velocity[1] = obj->spin_velocity[2] = 0;

  memcpy(obj->verts, verts, numVerts * sizeof(vertex));

  for (i = 0; i < numQuads; i++)
    {
      memcpy(obj->quads[i].vertIndex, &quads[i * 4], 4 * sizeof(u32));
      obj->quads[i].flags = 0;
      obj->quads[i].dp = 0;
      obj->quads[i].lightState = LIGHT_STATE_UNSET;

      obj->quads[i].ccb = create_coded_colored_cel8(16, 16, color);
    }

  return(obj);
}

void
set_object_pos(objectPtr obj,
               frac16    x,
               frac16    y,
               frac16    z)
{
  obj->worldPos.x = x;
  obj->worldPos.y = y;
  obj->worldPos.z = z;
}

/* --------------------------------------------------------------------------------------------- */
/* ----------------------------------------- CORE MATH ----------------------------------------- */
/* --------------------------------------------------------------------------------------------- */

static
void
rotate_object_y_to_world(objectPtr obj,
                         frac16    ycos,
                         frac16    ysin)
{
  u32 i;
  frac16 x, z;

  for (i = 0; i < obj->numVerts; i++)
    {
      x = obj->verts[i].x;
      z = obj->verts[i].z;
      obj->worldVerts[i].x = obj->worldPos.x + MulSF16(x, ycos) + MulSF16(z, ysin);
      obj->worldVerts[i].y = obj->worldPos.y + obj->verts[i].y;
      obj->worldVerts[i].z = obj->worldPos.z + MulSF16(-x, ysin) + MulSF16(z, ycos);
    }
}

static
void
rotate_object_xyz_to_world(objectPtr obj)
{
  u32 i;
  frac16 sx, cx, sy, cy, sz, cz;
  frac16 sxsy, cxsy;
  frac16 r00, r01, r02;
  frac16 r10, r11, r12;
  frac16 r20, r21, r22;

  cx = CosF16(obj->angles[0]);
  sx = SinF16(obj->angles[0]);
  cy = CosF16(obj->angles[1]);
  sy = SinF16(obj->angles[1]);
  cz = CosF16(obj->angles[2]);
  sz = SinF16(obj->angles[2]);

  sxsy = MulSF16(sx, sy);
  cxsy = MulSF16(cx, sy);

  r00 = MulSF16(cy, cz);
  r01 = MulSF16(cy, sz);
  r02 = -sy;
  r10 = MulSF16(sxsy, cz) - MulSF16(cx, sz);
  r11 = MulSF16(sxsy, sz) + MulSF16(cx, cz);
  r12 = MulSF16(sx, cy);
  r20 = MulSF16(cxsy, cz) + MulSF16(sx, sz);
  r21 = MulSF16(cxsy, sz) - MulSF16(sx, cz);
  r22 = MulSF16(cx, cy);

  for(i = 0; i < obj->numVerts; i++)
    {
      frac16 x, y, z;

      x = obj->verts[i].x;
      y = obj->verts[i].y;
      z = obj->verts[i].z;

      obj->worldVerts[i].x = obj->worldPos.x + MulSF16(x, r00) + MulSF16(y, r10) + MulSF16(z, r20);
      obj->worldVerts[i].y = obj->worldPos.y + MulSF16(x, r01) + MulSF16(y, r11) + MulSF16(z, r21);
      obj->worldVerts[i].z = obj->worldPos.z + MulSF16(x, r02) + MulSF16(y, r12) + MulSF16(z, r22);
    }
}

static
u32
pmv_from_dp(frac16 dp, u32 quad_idx)
{
  u32 base;

  if(dp < 2)       base = 1;
  else if(dp < 4)  base = 2;
  else if(dp < 8)  base = 3;
  else if(dp < 14) base = 4;
  else if(dp < 22) base = 5;
  else if(dp < 34) base = 6;
  else             base = 7;

  base += ((quad_idx * 3) & 1);

  if(base > 7) base = 7;
  if(base < 1) base = 1;

  return base;
}

static
boolean
cull_backface(objectPtr optr,
              quadPtr   qptr)
{
  frac16 v1x, v1y, v1z;
  frac16 v2x, v2y, v2z;
  frac16 nx, ny, nz;
  vertex *v0;
  vertex *v1ptr;
  vertex *v3ptr;

  v0    = &optr->worldVerts[qptr->vertIndex[0]];
  v1ptr = &optr->worldVerts[qptr->vertIndex[1]];
  v3ptr = &optr->worldVerts[qptr->vertIndex[3]];

  v1x = v1ptr->x - v0->x;
  v1y = v1ptr->y - v0->y;
  v1z = v1ptr->z - v0->z;
  v2x = v3ptr->x - v0->x;
  v2y = v3ptr->y - v0->y;
  v2z = v3ptr->z - v0->z;

  nx = MulSF16(v2y, v1z) - MulSF16(v2z, v1y);
  ny = MulSF16(v2z, v1x) - MulSF16(v2x, v1z);
  nz = MulSF16(v2x, v1y) - MulSF16(v2y, v1x);

  // no real camera in this demo, so POV vector is -v0.
  qptr->dp = (MulSF16(nx, -v0->x) + MulSF16(ny, -v0->y) + MulSF16(nz, -v0->z)) >> FRACBITS_16;

  if(qptr->dp < 0)
    return(TRUE);

  return(FALSE);
}

/* ------------------------------------------------------------------------- */
/* ----------------------------- CEL HELPERS ------------------------------- */
/* ------------------------------------------------------------------------- */

/*  Passing (void*)1 into CreateCel() will prevent a data buffer from being allocated.
    Passing CREATECEL_UNCODED to CreateCel() will prevent a PLUT from being allocated.
    The CODED bit is then set later when preambles are built. If you will point the
    returned cel's source and PLUT to an existing cel's data, pass FALSE for alloc. If
    you want source and PLUT space allocated for the returned cel, pass TRUE for alloc. */

static
CCB *
create_coded_cel8(u32  width,
                  u32  height,
                  boolean alloc)
{
  CCB *ccb = NULL;

  if(alloc)
    {
      // Source and PLUT are allocated.
      ccb = CreateCel(width, height, 8, CREATECEL_CODED, NULL);
    }
  else
    {
      // Source and PLUT are not allocated.
      ccb = CreateCel(width, height, 8, CREATECEL_UNCODED, (void*)1);
      ccb->ccb_SourcePtr = 0;
      ccb->ccb_PLUTPtr   = 0;
    }

  ccb->ccb_Flags =
    CCB_NPABS   |               // NextPtr is an absolute ptr, not a relative one.
    CCB_SPABS   |               // SourcePtr is absolute ptr, not a relative one.
    CCB_PPABS   |               // PLUTPtr is absolute ptr, not a relative one.
    CCB_LDSIZE  |               // Load hdx-y and vdx-y from CCB.
    CCB_LDPRS   |               // Load ddx-y from CCB.
    CCB_CCBPRE  |               // Load the preamble words from CCB, opposed to from source data.
    CCB_YOXY    |               // Load X/Y pos from the CCB.
    CCB_ACW     |               // Pixels facing forward will be seen.
    CCB_ACCW    |               // Pixels facing backward will be seen.
    CCB_ACE     |               // Enable both corner engines.
    CCB_LDPLUT  |               // Load PLUT from this cel.
    CCB_ALSC    |               // Line superclipping.
    CCB_LDPPMP;                 // Load the PIXC word from CCB.

  ccb->ccb_Flags &= ~CCB_BGND; // Support 000 as transparent pixel.

  ccb->ccb_Width  = width;
  ccb->ccb_Height = height;

  UNLAST_CEL(ccb);

  CLEAR_CEL_PREAMBLES(ccb); // This also marks the cel coded since it zeros the uncoded bit.

  SET_CEL_BPP8(ccb);
  SET_CEL_WOFFSET10(ccb, CALC_WOFFSET_BPP8(width));
  SET_CEL_TLHPCNT(ccb, width);
  SET_CEL_VCNT(ccb, height);

  return(ccb);
}

static
CCB *
create_coded_colored_cel8(u32 width,
                          u32 height,
                          u16 color)
{
  u32 i;
  size_t src_bytes;
  u16 *pptr;
  CCB *cel;

  cel = create_coded_cel8(width, height, TRUE);

  pptr = (u16*)cel->ccb_PLUTPtr;

  for (i = 0; i < 32; i++)
    {
      *pptr = color;
      pptr++;
    }

  src_bytes = get_cel_source_bytes(cel);
  memset(cel->ccb_SourcePtr, 0, src_bytes);

  return(cel);
}

/*
  The 3DO library contains GetCelDataBufferSize(), which this function
  was derived from. GetCelDataBufferSize() does not compile correctly
  due to precedence differences. This version uses parenthesis to
  force the correct calculation order.
*/
static
size_t
get_cel_source_bytes(CCB *cel)
{
  return (((size_t)cel->ccb_Height * get_cel_row_bytes(cel)) +
          ((cel->ccb_Flags & CCB_CCBPRE) ? 0 : 8));
}

static
size_t
get_cel_row_bytes(CCB *cel)
{
  size_t woffset;
  u32 pre1;

  pre1 = CEL_PRE1WORD(cel);

  if(get_cel_bpp(cel) < 8)
    woffset = (size_t)(pre1 & PRE1_WOFFSET8_MASK)  >> PRE1_WOFFSET8_SHIFT;
  else
    woffset = (size_t)(pre1 & PRE1_WOFFSET10_MASK) >> PRE1_WOFFSET10_SHIFT;

  return ((woffset + PRE1_WOFFSET_PREFETCH) * sizeof(i32));
}

static
i32
get_cel_bpp(CCB *cel)
{
  return(bppTable[GET_CEL_BPP_VALUE(cel)]);
}

/* ------------------------------------------------------------------------- */
/* ----------------------------------- DEBUG ------------------------------- */
/* ------------------------------------------------------------------------- */

static
void
dump_object(objectPtr obj)
{
  u32 i;

  printf("***************************************************\n");
  printf("**** Dumping Object *******************************\n");
  printf("***************************************************\n\n");

  printf("Vertex count: %d\n", obj->numVerts);
  printf("Quad count: %d\n\n", obj->numQuads);

  printf("Vertices:\n");
  for (i = 0; i < obj->numVerts; i++)
    printf("\t%d,\t%d,\t%d\n", obj->verts[i].x, obj->verts[i].y, obj->verts[i].z);

  printf("\nQuads:\n");
  for (i = 0; i < obj->numQuads; i++)
    printf("\t%d\t%d\t%d\t%d\n", obj->quads[i].vertIndex[0],
           obj->quads[i].vertIndex[1],
           obj->quads[i].vertIndex[2],
           obj->quads[i].vertIndex[3]);

  printf("\n");
}

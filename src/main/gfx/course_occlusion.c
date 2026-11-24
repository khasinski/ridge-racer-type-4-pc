/* Extended draw distance: opponent faces hidden behind the course
 * (rr/course_occlusion.h). */
#include <stdlib.h>
#include <string.h>
#include "rr/course_occlusion.h"
#include "rr/draw_distance.h"
#include "rr/depth_buffer.h"
#include "rr/widescreen.h"
#include "psyq/libgpu/gpuenv.h"
#include "rr/gpu_scratch_types.h"

unsigned int Psyz_GteDataRead(unsigned reg);
unsigned int Psyz_GteCtrlRead(unsigned reg);

/* Cells of one screen pixel (1 << CELL_SHIFT square); the view's viewport
 * (with the wide margin) must fit. A face is hidden when the cells it
 * covers and CELL_MARGIN cells around them are. */
enum { CELL_SHIFT = 0, CELL_MARGIN = 1, CELLS_X = 1024, CELLS_Y = 320, MAX_FACES = 8192 };
/* Corners nearer than this (or off the GTE's screen range) leave a tile out:
 * its projection is not the drawn one (near-plane subdivision, saturation). */
enum { NEAR_SZ = 0x200, SCREEN_LIMIT = 1000 };
/* A car with a face nearer than this (about 25 m) is left as it is: its
 * faces cover many pixels, and none of them was found wholly behind the
 * course (only partly, as in the original). */
enum { NEAR_FACE_SZ = 0x1400 };

/* A view: its scene viewport, mirroring, ordering table and screen offset
 * (the split screen's halves share a viewport rectangle). */
typedef struct {
    s32 x0, y0, x1, y1; /* screen bounds, inclusive */
    s32 renderFlags;
    const GpuDmaTag *order;
    s32 offsetX, offsetY;
} OcclusionView;

/* 1 / depth of the course where it shows (0: none) and its tile's ordering
 * entry (an index into the view's table) */
static float CourseInvDepth[CELLS_Y][CELLS_X];
static u16 CourseEntry[CELLS_Y][CELLS_X];
static GpuDmaTag *OrderBase; /* the view's ordering table */
enum { ORDER_ENTRIES = 704 };
static OcclusionView BufferView;   /* the view the buffer holds */
static int BufferValid, BufferFilling;
static s32 OriginX, OriginY, CellsW, CellsH;
/* The cells the faces can test: all of the view, or (when its cars came
 * first) those around the faces waiting for it. */
static s32 RegionX0, RegionY0, RegionX1, RegionY1;

/* The tile waiting for its packets (they follow its projection). */
static struct {
    int valid;
    float x[4], y[4], z[4];
    const GpuDmaTag *ot;
    uintptr_t start;
} Tile;

/* The view's opaque tiles, drawn into the buffer in the GPU's order once
 * the course is submitted: by ordering entry from the back, and in one
 * entry the last linked first. */
enum { MAX_TILES = 8192 };
typedef struct {
    float x[4], y[4], z[4];
    uintptr_t entry;
    s32 order;
} OpaqueTile;
static u16 RasterEntry; /* the entry of the tile being rasterized */
static OpaqueTile Tiles[MAX_TILES];
static int TileCount;

typedef struct {
    GpuDmaTag *entry;          /* the ordering entry it is linked in */
    GpuDmaTag *first, *last;   /* its packets: a run of the entry's list */
    GpuDmaTag *second;         /* a second drawing packet of the run */
    s16 x0, y0, x1, y1;
    s16 x[4], y[4];            /* its corners (the PS1 quad 0-1-2, 1-2-3) */
    u16 depth;
    u8 shadow;
} PendingFace;
static PendingFace Pending[MAX_FACES];
static int PendingCount;

/* A car's faces: Pending[first .. first + count - 1]. */
enum { MAX_CARS = 64 };
typedef struct {
    int first, count;
    int near;                  /* a face nearer than NEAR_FACE_SZ */
    OcclusionView view;
} PendingCar;
static PendingCar Cars[MAX_CARS];
static int CarCount;
int CourseOcclusionCarActive;
static int CarShadow;

/* ---- course textures: which tiles show through ---- */

/* A copy of the VRAM taken at the first course view after a course load:
 * a tile whose texels include the transparent colour (fences, trees, the
 * cut-out shapes) or that is semi-transparent hides nothing. Without a copy
 * (no course loaded yet) no textured tile hides anything. */
static u16 Vram[512][1024];
static int VramWanted, VramCopied;
enum { OPACITY_SLOTS = 4096 };
static u64 OpacityKey[OPACITY_SLOTS];
static u8 OpacityValue[OPACITY_SLOTS];

void CourseOcclusionCourseLoaded(void) {
    VramWanted = 1;
    VramCopied = 0;
}

static void CopyVram(void) {
    GpuRectangle rect;
    rect.rect.x = 0;
    rect.rect.y = 0;
    rect.rect.w = 1024;
    rect.rect.h = 512;
    DrawSync(0);
    StoreImage(&rect, Vram);
    DrawSync(0);
    memset(OpacityKey, 0, sizeof(OpacityKey));
    VramCopied = 1;
}

/* Nonzero when every texel of the rectangle u0..u1 x v0..v1 of the page and
 * palette is opaque. */
static int TexelsOpaque(u32 tpage, u32 clut, s32 u0, s32 v0, s32 u1, s32 v1) {
    s32 pageX = (s32)(tpage & 0xF) << 6, pageY = (s32)(tpage & 0x10) << 4;
    s32 mode = (s32)(tpage >> 7) & 3;
    s32 clutX = (s32)(clut & 0x3F) << 4, clutY = (s32)(clut >> 6) & 0x1FF;
    s32 u, v;
    for (v = v0; v <= v1; v++) {
        const u16 *row = Vram[(pageY + v) & 511];
        for (u = u0; u <= u1; u++) {
            u16 color;
            if (mode == 0) {
                u32 index = (row[(pageX + (u >> 2)) & 1023] >> ((u & 3) * 4)) & 0xF;
                color = Vram[clutY][(clutX + index) & 1023];
            } else if (mode == 1) {
                u32 index = (row[(pageX + (u >> 1)) & 1023] >> ((u & 1) * 8)) & 0xFF;
                color = Vram[clutY][(clutX + index) & 1023];
            } else {
                color = row[(pageX + u) & 1023];
            }
            if (color == 0) return 0;
        }
    }
    return 1;
}

static int TextureOpaque(u32 tpage, u32 clut, s32 u0, s32 v0, s32 u1, s32 v1) {
    u64 key = ((u64)(tpage & 0x1FF) << 48) | ((u64)(clut & 0xFFFF) << 32) |
              ((u64)u0 << 24) | ((u64)v0 << 16) | ((u64)u1 << 8) | (u64)v1;
    u32 slot = (u32)((key * 0x9E3779B97F4A7C15ull) >> 52) & (OPACITY_SLOTS - 1);
    int probes;
    if (!VramCopied) return 0; /* not known: see-through */
    key |= 1ull << 63; /* never zero */
    for (probes = 0; probes < 16; probes++, slot = (slot + 1) & (OPACITY_SLOTS - 1)) {
        if (OpacityKey[slot] == key) return OpacityValue[slot];
        if (OpacityKey[slot] == 0) {
            OpacityKey[slot] = key;
            OpacityValue[slot] = (u8)TexelsOpaque(tpage, clut, u0, v0, u1, v1);
            return OpacityValue[slot];
        }
    }
    return TexelsOpaque(tpage, clut, u0, v0, u1, v1);
}

/* The tile's packets: linked at the head of its ordering entry, after the
 * cursor its projection saw. Nonzero when they are opaque polygons. */
static int TilePacketsOpaque(const GpuDmaTag *ot, uintptr_t start, uintptr_t end) {
    const GpuDmaTag *p = (const GpuDmaTag *)ot->address;
    int polygons = 0;
    while ((uintptr_t)p != RR_GPU_END_LINK && (uintptr_t)p >= start && (uintptr_t)p < end) {
        const u32 *w = (const u32 *)(p + 1);
        u32 n = (u32)p->length, k = 0, code;
        while (k < n && (w[k] == RR_GPU_MERGED_PRIM_NOP || (w[k] >> 24) == 0 ||
                         ((w[k] >> 24) >= 0xE1 && (w[k] >> 24) <= 0xE6)))
            k++;
        if (k < n) {
            code = w[k] >> 24;
            if (code >= 0x20 && code <= 0x3F) {
                int quad = (code & 8) != 0, gouraud = (code & 0x10) != 0;
                if (code & 2) return 0; /* semi-transparent */
                if (code & 4) {
                    /* words: colour, then per vertex [colour] xy uv */
                    u32 step = gouraud ? 3 : 2, vertices = quad ? 4 : 3, i;
                    u32 uvAt = k + 2, clut = 0, tpage = 0;
                    s32 u0 = 255, v0 = 255, u1 = 0, v1 = 0;
                    if (uvAt + step * (vertices - 1) >= n) return 0;
                    for (i = 0; i < vertices; i++) {
                        u32 uv = w[uvAt + step * i];
                        s32 u = (s32)(uv & 0xFF), v = (s32)((uv >> 8) & 0xFF);
                        if (i == 0) clut = uv >> 16;
                        if (i == 1) tpage = uv >> 16;
                        if (u < u0) u0 = u;
                        if (u > u1) u1 = u;
                        if (v < v0) v0 = v;
                        if (v > v1) v1 = v;
                    }
                    if (!TextureOpaque(tpage, clut, u0, v0, u1, v1)) return 0;
                }
                polygons++;
            }
        }
        p = (const GpuDmaTag *)p->address;
    }
    return polygons != 0;
}

/* ---- the buffer ---- */

static OcclusionView ViewOf(const RroSceneScratchPrefix *scene) {
    const GpuRectangleFields *vp = &scene->viewport6C.rect;
    OcclusionView v;
    v.x0 = vp->x - WideMarginX;
    v.x1 = vp->w + WideMarginX;
    v.y0 = vp->y;
    v.y1 = vp->h;
    v.renderFlags = scene->renderFlags64 != 0;
    v.order = RR_GPU_SCRATCH->order;
    v.offsetX = (s32)Psyz_GteCtrlRead(24); /* OFX, OFY */
    v.offsetY = (s32)Psyz_GteCtrlRead(25);
    return v;
}

static int SameView(const OcclusionView *a, const OcclusionView *b) {
    return a->x0 == b->x0 && a->x1 == b->x1 && a->y0 == b->y0 && a->y1 == b->y1 &&
           a->renderFlags == b->renderFlags && a->order == b->order &&
           a->offsetX == b->offsetX && a->offsetY == b->offsetY;
}

void CourseOcclusionFrame(void) {
    BufferValid = BufferFilling = 0;
    Tile.valid = 0;
    PendingCount = 0;
    CarCount = 0;
    CourseOcclusionCarActive = 0;
}

void CourseOcclusionCourseBegin(const RroSceneScratchPrefix *scene) {
    s32 y;
    BufferFilling = 0;
    BufferValid = 0;
    Tile.valid = 0;
    TileCount = 0;
    /* The depth buffer (rr/depth_buffer.h) hides them per pixel itself. */
    if (!ExtendedDrawDistance || DepthBuffer) return;
    BufferView = ViewOf(scene);
    OriginX = BufferView.x0 - 2;
    OriginY = BufferView.y0 - 2;
    CellsW = ((BufferView.x1 + 2 - OriginX) >> CELL_SHIFT) + 1;
    CellsH = ((BufferView.y1 + 2 - OriginY) >> CELL_SHIFT) + 1;
    if (CellsW > CELLS_X || CellsH > CELLS_Y || CellsW <= 0 || CellsH <= 0) return;
    OrderBase = RR_GPU_SCRATCH->order;
    RegionX0 = 0;
    RegionY0 = 0;
    RegionX1 = CellsW - 1;
    RegionY1 = CellsH - 1;
    if (CarCount) {
        int c, i, any = 0;
        s32 x0 = 0x7FFF, y0 = 0x7FFF, x1 = -0x8000, y1 = -0x8000;
        for (c = 0; c < CarCount; c++) for (i = Cars[c].first; i < Cars[c].first + Cars[c].count; i++) {
            const PendingFace *f = &Pending[i];
            if (Cars[c].near || !SameView(&Cars[c].view, &BufferView)) continue;
            any = 1;
            if (f->x0 < x0) x0 = f->x0;
            if (f->y0 < y0) y0 = f->y0;
            if (f->x1 > x1) x1 = f->x1;
            if (f->y1 > y1) y1 = f->y1;
        }
        if (!any) return; /* no car of this view: nothing to test */
        x0 = ((x0 - OriginX) >> CELL_SHIFT) - CELL_MARGIN;
        y0 = ((y0 - OriginY) >> CELL_SHIFT) - CELL_MARGIN;
        x1 = ((x1 - OriginX) >> CELL_SHIFT) + CELL_MARGIN;
        y1 = ((y1 - OriginY) >> CELL_SHIFT) + CELL_MARGIN;
        RegionX0 = x0 > 0 ? x0 : 0;
        RegionY0 = y0 > 0 ? y0 : 0;
        RegionX1 = x1 < RegionX1 ? x1 : RegionX1;
        RegionY1 = y1 < RegionY1 ? y1 : RegionY1;
        if (RegionX0 > RegionX1 || RegionY0 > RegionY1) return;
    } else if (!BufferView.renderFlags) {
        /* the main view's cars come first: none, nothing to test */
        return;
    }
    if (VramWanted && !VramCopied) CopyVram();
    for (y = RegionY0; y <= RegionY1; y++)
        memset(&CourseInvDepth[y][RegionX0], 0,
               (size_t)(RegionX1 - RegionX0 + 1) * sizeof(CourseInvDepth[0][0]));
    BufferFilling = 1;
}

/* The course one triangle leaves where it is drawn: the reciprocal of its
 * depth (linear on the screen, perspective correct), sampled at cell
 * centres, stepped along each row. */
static void RasterTriangle(const float *x, const float *y, const float *z) {
    const float cell = (float)(1 << CELL_SHIFT);
    float area = (x[1] - x[0]) * (y[2] - y[0]) - (x[2] - x[0]) * (y[1] - y[0]);
    float minx, maxx, miny, maxy, inv, iz[3];
    float a0, c0, d0, a1, c1, d1, ai, ci, di;
    s32 cx0, cx1, cy0, cy1, cx, cy;
    if (area == 0.0f) return;
    inv = 1.0f / area;
    minx = x[0] < x[1] ? x[0] : x[1]; if (x[2] < minx) minx = x[2];
    maxx = x[0] > x[1] ? x[0] : x[1]; if (x[2] > maxx) maxx = x[2];
    miny = y[0] < y[1] ? y[0] : y[1]; if (y[2] < miny) miny = y[2];
    maxy = y[0] > y[1] ? y[0] : y[1]; if (y[2] > maxy) maxy = y[2];
    /* cell c covers pixels [c, c + 1) << CELL_SHIFT; its centre is sampled */
    cx0 = (s32)((minx - OriginX) / cell - 0.5f);
    cx1 = (s32)((maxx - OriginX) / cell + 0.5f);
    cy0 = (s32)((miny - OriginY) / cell - 0.5f);
    cy1 = (s32)((maxy - OriginY) / cell + 0.5f);
    if (cx0 < RegionX0) cx0 = RegionX0;
    if (cy0 < RegionY0) cy0 = RegionY0;
    if (cx1 > RegionX1) cx1 = RegionX1;
    if (cy1 > RegionY1) cy1 = RegionY1;
    if (cx0 > cx1 || cy0 > cy1) return;
    iz[0] = 1.0f / z[0];
    iz[1] = 1.0f / z[1];
    iz[2] = 1.0f / z[2];
    /* barycentric b0, b1 and 1/z as a * px + c * py + d */
    a0 = (y[1] - y[2]) * inv; c0 = (x[2] - x[1]) * inv; d0 = (x[1] * y[2] - x[2] * y[1]) * inv;
    a1 = (y[2] - y[0]) * inv; c1 = (x[0] - x[2]) * inv; d1 = (x[2] * y[0] - x[0] * y[2]) * inv;
    ai = a0 * iz[0] + a1 * iz[1] - (a0 + a1) * iz[2];
    ci = c0 * iz[0] + c1 * iz[1] - (c0 + c1) * iz[2];
    di = d0 * iz[0] + d1 * iz[1] + (1.0f - d0 - d1) * iz[2];
    for (cy = cy0; cy <= cy1; cy++) {
        float py = OriginY + ((float)cy + 0.5f) * cell;
        float px = OriginX + ((float)cx0 + 0.5f) * cell;
        float b0 = a0 * px + c0 * py + d0, b1 = a1 * px + c1 * py + d1;
        float i = ai * px + ci * py + di;
        float *row = CourseInvDepth[cy];
        for (cx = cx0; cx <= cx1; cx++) {
            if (b0 >= 0.0f && b1 >= 0.0f && b0 + b1 <= 1.0f && i > 0.0f) {
                row[cx] = i;
                CourseEntry[cy][cx] = RasterEntry;
            }
            b0 += a0 * cell;
            b1 += a1 * cell;
            i += ai * cell;
        }
    }
}

/* The waiting tile, once its packets are linked: drawn opaque, it is kept
 * for the buffer. */
static void FinishTile(uintptr_t end) {
    OpaqueTile *t;
    if (!Tile.valid) return;
    Tile.valid = 0;
    if (TileCount >= MAX_TILES || Tile.ot < OrderBase || Tile.ot >= OrderBase + ORDER_ENTRIES ||
        !TilePacketsOpaque(Tile.ot, Tile.start, end))
        return;
    t = &Tiles[TileCount];
    memcpy(t->x, Tile.x, sizeof(t->x));
    memcpy(t->y, Tile.y, sizeof(t->y));
    memcpy(t->z, Tile.z, sizeof(t->z));
    t->entry = (uintptr_t)Tile.ot;
    t->order = TileCount++;
}

/* Back to front: the higher ordering entry first (the tables are linked
 * from their last entry), then the later linked of one entry. */
static int DrawOrder(const void *a, const void *b) {
    const OpaqueTile *p = a, *q = b;
    if (p->entry != q->entry) return p->entry < q->entry ? 1 : -1;
    return q->order - p->order;
}

/* The buffer holds the depth of the course where it shows: tiles a later
 * one covers (the course pushes some behind others by their ordering bias,
 * nearer though they are) leave nothing behind it. */
static void DrawTiles(void) {
    int n, i;
    qsort(Tiles, (size_t)TileCount, sizeof(Tiles[0]), DrawOrder);
    for (n = 0; n < TileCount; n++) {
        const OpaqueTile *t = &Tiles[n];
        float tx[3], ty[3], tz[3];
        RasterEntry = (u16)((const GpuDmaTag *)t->entry - OrderBase);
        /* the PS1 quad: triangles 0-1-2 and 1-2-3 */
        RasterTriangle(t->x, t->y, t->z);
        for (i = 0; i < 3; i++) {
            tx[i] = t->x[i + 1];
            ty[i] = t->y[i + 1];
            tz[i] = t->z[i + 1];
        }
        RasterTriangle(tx, ty, tz);
    }
    TileCount = 0;
}

void CourseOcclusionTile(const s32 sxy[4], const GpuDmaTag *ot, const u8 *cursor) {
    int i;
    if (!BufferFilling) return;
    FinishTile((uintptr_t)cursor);
    for (i = 0; i < 4; i++) {
        s32 sz = (s32)(Psyz_GteDataRead(16 + i) & 0xFFFF);
        s32 sx = (s16)(sxy[i] & 0xFFFF), sy = sxy[i] >> 16;
        if (sz < NEAR_SZ || sx < -SCREEN_LIMIT || sx > SCREEN_LIMIT || sy < -SCREEN_LIMIT ||
            sy > SCREEN_LIMIT)
            return;
        Tile.x[i] = (float)sx;
        Tile.y[i] = (float)sy;
        Tile.z[i] = (float)sz;
    }
    {
        /* a tile wholly outside the cells the faces test changes nothing */
        float lo = Tile.x[0], hi = Tile.x[0], top = Tile.y[0], bottom = Tile.y[0];
        for (i = 1; i < 4; i++) {
            if (Tile.x[i] < lo) lo = Tile.x[i];
            if (Tile.x[i] > hi) hi = Tile.x[i];
            if (Tile.y[i] < top) top = Tile.y[i];
            if (Tile.y[i] > bottom) bottom = Tile.y[i];
        }
        if ((s32)hi < OriginX + (RegionX0 << CELL_SHIFT) || (s32)lo > OriginX + ((RegionX1 + 1) << CELL_SHIFT) ||
            (s32)bottom < OriginY + (RegionY0 << CELL_SHIFT) || (s32)top > OriginY + ((RegionY1 + 1) << CELL_SHIFT))
            return;
    }
    Tile.ot = ot;
    Tile.start = (uintptr_t)cursor;
    Tile.valid = 1;
}

/* Nonzero when the point lies in the triangle a-b-c (either winding). */
static int InTriangle(float px, float py, float ax, float ay, float bx, float by, float cx, float cy) {
    float d0 = (bx - ax) * (py - ay) - (by - ay) * (px - ax);
    float d1 = (cx - bx) * (py - by) - (cy - by) * (px - bx);
    float d2 = (ax - cx) * (py - cy) - (ay - cy) * (px - cx);
    return (d0 >= 0.0f && d1 >= 0.0f && d2 >= 0.0f) || (d0 <= 0.0f && d1 <= 0.0f && d2 <= 0.0f);
}

static int FaceCovers(const PendingFace *f, float px, float py) {
    return InTriangle(px, py, f->x[0], f->y[0], f->x[1], f->y[1], f->x[2], f->y[2]) ||
           InTriangle(px, py, f->x[1], f->y[1], f->x[2], f->y[2], f->x[3], f->y[3]);
}

/* A face against the course where it shows. */
typedef struct {
    int hidden;    /* course nearer over its cells and the cells around */
    int shows;     /* course nearer somewhere on its cells, drawn over it */
    s32 own;       /* its ordering entry */
    s32 need;      /* entries back to be drawn before that course */
    s32 room;      /* entries back it can go and stay after the course
                    * behind it */
} FaceState;

static void FaceClassify(const PendingFace *f, FaceState *st) {
    /* the buffer's tiles are flat quads at their corners' depths, the drawn
     * ones subdivided: a margin of 1/64 of the depth */
    s32 limit = (s32)f->depth - (f->depth >> 6) - 16;
    float nearer;
    s32 cx0 = ((f->x0 - OriginX) >> CELL_SHIFT) - CELL_MARGIN;
    s32 cx1 = ((f->x1 - OriginX) >> CELL_SHIFT) + CELL_MARGIN;
    s32 cy0 = ((f->y0 - OriginY) >> CELL_SHIFT) - CELL_MARGIN;
    s32 cy1 = ((f->y1 - OriginY) >> CELL_SHIFT) + CELL_MARGIN;
    s32 cx, cy, front = -1, behind = ORDER_ENTRIES;
    int visible = 0;
    st->hidden = st->shows = 0;
    st->own = (s32)(f->entry - OrderBase);
    st->need = 0;
    st->room = 0;
    if (limit <= 0 || st->own < 0 || st->own >= ORDER_ENTRIES) return;
    nearer = 1.0f / (float)limit;
    /* a face reaching out of the cells drawn is not known to be hidden */
    if (cx0 < RegionX0 || cy0 < RegionY0 || cx1 > RegionX1 || cy1 > RegionY1) return;
    for (cy = cy0; cy <= cy1 && !visible; cy++)
        for (cx = cx0; cx <= cx1; cx++)
            if (!(CourseInvDepth[cy][cx] >= nearer)) {
                visible = 1;
                break;
            }
    if (!visible) {
        st->hidden = 1;
        return;
    }
    /* the course over the face's own cells: in front of it, or behind */
    for (cy = cy0 + CELL_MARGIN; cy <= cy1 - CELL_MARGIN; cy++) {
        for (cx = cx0 + CELL_MARGIN; cx <= cx1 - CELL_MARGIN; cx++) {
            float course = CourseInvDepth[cy][cx];
            s32 entry = CourseEntry[cy][cx];
            if (!FaceCovers(f, OriginX + ((cx << CELL_SHIFT) + (1 << CELL_SHIFT) / 2.0f),
                            OriginY + ((cy << CELL_SHIFT) + (1 << CELL_SHIFT) / 2.0f)))
                continue;
            if (course >= nearer) {
                if (entry > front) front = entry;
            } else if (course > 0.0f && entry < behind) {
                behind = entry;
            }
        }
    }
    /* the entries drawn first are the higher ones: the face shows over
     * course in front of it linked at its entry or after */
    st->shows = front >= st->own;
    st->need = front + 1 - st->own;
    st->room = behind - 1 - st->own;
}

/* Linked as before, no payload: the GPU skips it. */
static void HideFace(const PendingFace *f) {
    f->first->length = 0;
    if (f->second) f->second->length = 0;
}

/* Move a face's packets to the head of the entry delta behind its own. */
static void MoveFace(const PendingFace *f, s32 delta) {
    GpuDmaTag *p = f->entry, *target = f->entry + delta;
    int guard;
    for (guard = 0; guard < 4096 && p->address != (uintptr_t)f->first; guard++) {
        if (p->address == RR_GPU_END_LINK) return;
        p = (GpuDmaTag *)p->address;
    }
    if (p->address != (uintptr_t)f->first) return;
    p->address = f->last->address;
    f->last->address = target->address;
    target->address = (uintptr_t)f->first;
}

/* One car against its view's course. Faces wholly behind the course are
 * not drawn. When others show over course in front of them (the tiles are
 * ordered as a whole, pushed back by their bias), the whole car moves back
 * in the table behind that course, keeping its own order, if every face
 * stays drawn after the course behind it; otherwise only its shadow faces
 * move, and the hidden faces of a car whose body or wheels still show over
 * the course stay too (a car drawn whole rather than in pieces). */
static void ProcessCar(const PendingCar *car) {
    enum { MAX_CAR_FACES = 1024 };
    static FaceState st[MAX_CAR_FACES];
    const PendingFace *faces = &Pending[car->first];
    int n = car->count < MAX_CAR_FACES ? car->count : MAX_CAR_FACES, i;
    int shows = 0, bodyShows = 0;
    s32 delta = 0, room = ORDER_ENTRIES;
    if (car->near) return;
    for (i = 0; i < n; i++) {
        FaceClassify(&faces[i], &st[i]);
        if (st[i].hidden) continue;
        if (st[i].room < room) room = st[i].room;
        if (!st[i].shows) continue;
        shows = 1;
        if (!faces[i].shadow) bodyShows = 1;
        if (st[i].need > delta) delta = st[i].need;
    }
    if (shows && delta <= room) {
        for (i = 0; i < n; i++) {
            if (st[i].hidden) HideFace(&faces[i]);
            else MoveFace(&faces[i], delta);
        }
        return;
    }
    for (i = 0; i < n; i++) {
        if (st[i].hidden) {
            if (faces[i].shadow || !bodyShows) HideFace(&faces[i]);
        } else if (faces[i].shadow && st[i].shows && st[i].need <= st[i].room) {
            MoveFace(&faces[i], st[i].need);
        }
    }
}

void CourseOcclusionCourseEnd(const u8 *cursor) {
    int i;
    if (!BufferFilling) {
        PendingCount = 0;
        CarCount = 0;
        return;
    }
    FinishTile((uintptr_t)cursor);
    DrawTiles();
    BufferFilling = 0;
    BufferValid = 1;
    /* the cars of this view, drawn before its course; any other view's are
     * dropped (drawn as they are) */
    for (i = 0; i < CarCount; i++)
        if (SameView(&Cars[i].view, &BufferView)) ProcessCar(&Cars[i]);
    PendingCount = 0;
    CarCount = 0;
}

void CourseOcclusionCarBegin(const RroSceneScratchPrefix *scene) {
    PendingCar *car;
    CarShadow = 0;
    if (!ExtendedDrawDistance || DepthBuffer || CarCount >= MAX_CARS) return;
    car = &Cars[CarCount++];
    car->view = ViewOf(scene);
    car->first = PendingCount;
    car->count = 0;
    car->near = 0;
    CourseOcclusionCarActive = 1;
}

void CourseOcclusionCarEnd(void) {
    PendingCar *car;
    if (!CourseOcclusionCarActive) return;
    car = &Cars[CarCount - 1];
    CourseOcclusionCarActive = 0;
    CarShadow = 0;
    car->count = PendingCount - car->first;
    /* the mirror's cars come after its course: done at once */
    if (BufferValid && SameView(&car->view, &BufferView)) {
        ProcessCar(car);
        PendingCount = car->first;
        CarCount--;
    }
}

void CourseOcclusionCarShadow(int shadow) {
    CarShadow = shadow;
}

void CourseOcclusionFace(GpuDmaTag *entry, GpuDmaTag *first, GpuDmaTag *last, GpuDmaTag *second,
                         const s32 *sxy, int count) {
    PendingFace f;
    s32 depth = 0xFFFF;
    int i;
    f.x0 = f.y0 = 0x7FFF;
    f.x1 = f.y1 = -0x8000;
    for (i = 0; i < count; i++) {
        s32 sx = (s16)(sxy[i] & 0xFFFF), sy = sxy[i] >> 16;
        s32 sz = (s32)(Psyz_GteDataRead(16 + 4 - count + i) & 0xFFFF);
        if (sx < f.x0) f.x0 = (s16)sx;
        if (sx > f.x1) f.x1 = (s16)sx;
        if (sy < f.y0) f.y0 = (s16)sy;
        if (sy > f.y1) f.y1 = (s16)sy;
        f.x[i] = (s16)sx;
        f.y[i] = (s16)sy;
        if (sz < depth) depth = sz;
    }
    for (; i < 4; i++) {
        f.x[i] = f.x[i - 1];
        f.y[i] = f.y[i - 1];
    }
    if (depth < NEAR_FACE_SZ) Cars[CarCount - 1].near = 1;
    f.depth = (u16)depth;
    f.entry = entry;
    f.first = first;
    f.last = last;
    f.second = second;
    f.shadow = (u8)CarShadow;
    if (PendingCount < MAX_FACES) Pending[PendingCount++] = f;
}

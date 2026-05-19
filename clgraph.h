#ifndef CLGRAPH_H
#define CLGRAPH_H

/*
 * clgraph.h  —  Epic CLI Graph Library
 * Backend: ncurses + braille sub-pixel rendering
 * Supports: 2D fn / parametric / polar / scatter / bar /
 *           vector field / heatmap / contour / 3D surface /
 *           parametric 3D / animation / color palettes
 * Deps: ncurses, math.h  (optionally gsl)
 * Works on Linux, macOS, Termux (Android)
 */

/* ensure M_PI and friends are available on all platforms (C11 strict mode) */
#ifndef _USE_MATH_DEFINES
#  define _USE_MATH_DEFINES
#endif
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#endif

#include <ncurses.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

/* ─────────────────────────────────────────
   BRAILLE SUB-PIXEL LAYOUT  (2 cols × 4 rows)
   bit positions in braille unicode dot mask:
      col0  col1
   r0:  0    3
   r1:  1    4
   r2:  2    5
   r3:  6    7
   ───────────────────────────────────────── */
static const uint8_t CLG_DOT[4][2] = {
    {0, 3}, {1, 4}, {2, 5}, {6, 7}
};
#define CLG_BRAILLE_BASE 0x2800u

/* ─────────────────────────────────────────
   COLOR PAIR IDS  (we allocate 64 pairs)
   ───────────────────────────────────────── */
#define CLG_MAX_COLORS  64

/* ─────────────────────────────────────────
   CANVAS
   ───────────────────────────────────────── */
typedef struct {
    int      cols, rows;   /* ncurses cell dimensions        */
    int      px_w, px_h;  /* sub-pixel dimensions (×2, ×4)  */
    uint8_t *bits;         /* 1 bit per sub-pixel            */
    short   *cpair;        /* ncurses color pair per cell    */
    uint32_t fg;           /* current draw color (packed RGB)*/
    short    cur_pair;     /* current ncurses color pair id  */
    WINDOW  *win;          /* ncurses window                 */
    bool     has_colors;
} ClgCanvas;

/* ─────────────────────────────────────────
   VIEWPORT  (math space → pixel space)
   ───────────────────────────────────────── */
typedef struct {
    double xmin, xmax;
    double ymin, ymax;
    double zmin, zmax;
} ClgViewport;

/* ─────────────────────────────────────────
   CAMERA  (3-D orthographic projection)
   ───────────────────────────────────────── */
typedef struct {
    double rx, ry, rz;  /* Euler rotation angles (radians) */
    double scale;       /* zoom factor, typical 0.4–0.8    */
    double ox, oy;      /* viewport offset (fraction)      */
} ClgCamera;

/* ─────────────────────────────────────────
   FUNCTION TYPEDEFS
   ───────────────────────────────────────── */
typedef double (*ClgFn1D)    (double x);
typedef double (*ClgFnPolar) (double theta);
typedef double (*ClgFn2D)    (double x, double y);
typedef double (*ClgFn3D)    (double x, double y);  /* z = f(x,y) */
typedef void   (*ClgFnParam2)(double t, double *x, double *y);
typedef void   (*ClgFnParam3)(double u, double v,
                               double *x, double *y, double *z);
typedef void   (*ClgFnVec2)  (double x, double y,
                               double *u, double *vv);
/* animation frame callback */
typedef void   (*ClgFrameFn) (ClgCanvas *c, ClgViewport *v,
                               double t, void *userdata);

/* ─────────────────────────────────────────
   ANIMATION STATE
   ───────────────────────────────────────── */
typedef struct {
    double t, dt, t_end;
    int    fps;
    bool   loop;
} ClgAnim;

/* ═══════════════════════════════════════════
   API  —  canvas lifecycle
   ═══════════════════════════════════════════ */
ClgCanvas *clg_init(int cols, int rows);
ClgCanvas *clg_init_fullscreen(void);
void       clg_free(ClgCanvas *c);
void       clg_clear(ClgCanvas *c);
void       clg_render(ClgCanvas *c);
void       clg_set_color(ClgCanvas *c, uint8_t r, uint8_t g, uint8_t b);

/* ═══════════════════════════════════════════
   API  —  primitives
   ═══════════════════════════════════════════ */
void clg_pixel  (ClgCanvas *c, int px, int py);
void clg_line   (ClgCanvas *c, int x0, int y0, int x1, int y1);
void clg_rect   (ClgCanvas *c, int x,  int y,  int w,  int h);
void clg_circle (ClgCanvas *c, int cx, int cy, int r);
void clg_fill   (ClgCanvas *c, int x,  int y,  int w,  int h);
void clg_text   (ClgCanvas *c, int row, int col,
                 uint8_t r, uint8_t g, uint8_t b,
                 const char *fmt, ...);

/* ═══════════════════════════════════════════
   API  —  2-D plots
   ═══════════════════════════════════════════ */
void clg_plot_fn      (ClgCanvas *c, ClgViewport *v,
                       ClgFn1D f, double step,
                       uint8_t r, uint8_t g, uint8_t b);

void clg_plot_param2  (ClgCanvas *c, ClgViewport *v,
                       ClgFnParam2 f,
                       double t0, double t1, double step,
                       uint8_t r, uint8_t g, uint8_t b);

void clg_plot_polar   (ClgCanvas *c, ClgViewport *v,
                       ClgFnPolar f,
                       double t0, double t1, double step,
                       uint8_t r, uint8_t g, uint8_t b);

void clg_plot_scatter (ClgCanvas *c, ClgViewport *v,
                       double *xs, double *ys, int n,
                       uint8_t r, uint8_t g, uint8_t b);

void clg_plot_bar     (ClgCanvas *c, ClgViewport *v,
                       double *xs, double *ys, int n,
                       uint8_t r, uint8_t g, uint8_t b);

void clg_plot_vecfield(ClgCanvas *c, ClgViewport *v,
                       ClgFnVec2 f, int gcols, int grows,
                       uint8_t r, uint8_t g, uint8_t b);

/* ═══════════════════════════════════════════
   API  —  2-D field plots
   ═══════════════════════════════════════════ */
void clg_plot_heatmap (ClgCanvas *c, ClgViewport *v,
                       ClgFn2D f, int samples);

void clg_plot_contour (ClgCanvas *c, ClgViewport *v,
                       ClgFn2D f,
                       double *levels, int n_levels,
                       int samples);

/* ═══════════════════════════════════════════
   API  —  3-D plots
   ═══════════════════════════════════════════ */
void clg_project3d    (ClgCamera *cam, ClgCanvas *c,
                       double x, double y, double z,
                       int *px, int *py);

void clg_plot_surface (ClgCanvas *c, ClgViewport *v,
                       ClgCamera *cam,
                       ClgFn3D f, int usteps, int vsteps,
                       uint8_t r, uint8_t g, uint8_t b);

void clg_plot_param3  (ClgCanvas *c, ClgViewport *v,
                       ClgCamera *cam,
                       ClgFnParam3 f,
                       double u0, double u1,
                       double v0, double v1,
                       int usteps, int vsteps,
                       uint8_t r, uint8_t g, uint8_t b);

/* ═══════════════════════════════════════════
   API  —  decoration
   ═══════════════════════════════════════════ */
void clg_axes   (ClgCanvas *c, ClgViewport *v,
                 uint8_t r, uint8_t g, uint8_t b);
void clg_grid   (ClgCanvas *c, ClgViewport *v,
                 double xstep, double ystep,
                 uint8_t r, uint8_t g, uint8_t b);
void clg_border (ClgCanvas *c,
                 uint8_t r, uint8_t g, uint8_t b);
void clg_labels (ClgCanvas *c, ClgViewport *v,
                 const char *xlabel, const char *ylabel);
void clg_title  (ClgCanvas *c, const char *title,
                 uint8_t r, uint8_t g, uint8_t b);

/* ═══════════════════════════════════════════
   API  —  animation
   ═══════════════════════════════════════════ */
ClgAnim *clg_anim_new (double dt, double t_end, int fps, bool loop);
void     clg_anim_run (ClgAnim *a, ClgCanvas *c, ClgViewport *v,
                       ClgFrameFn fn, void *userdata);
void     clg_anim_free(ClgAnim *a);

/* ═══════════════════════════════════════════
   API  —  color palettes  (t in [0,1])
   ═══════════════════════════════════════════ */
void clg_pal_rainbow(double t, uint8_t *r, uint8_t *g, uint8_t *b);
void clg_pal_plasma (double t, uint8_t *r, uint8_t *g, uint8_t *b);
void clg_pal_fire   (double t, uint8_t *r, uint8_t *g, uint8_t *b);
void clg_pal_cool   (double t, uint8_t *r, uint8_t *g, uint8_t *b);
void clg_pal_viridis(double t, uint8_t *r, uint8_t *g, uint8_t *b);

/* ═══════════════════════════════════════════
   API  —  utility
   ═══════════════════════════════════════════ */
void   clg_term_size(int *cols, int *rows);
void   clg_sleep_ms (int ms);
double clg_now_ms   (void);

#endif /* CLGRAPH_H */

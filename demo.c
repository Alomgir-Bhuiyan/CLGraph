/*
 * demo.c  —  clgraph showcase
 * Demonstrates every major plot type.
 * Press ENTER to cycle through demos.
 * Press 'q' during animation to stop.
 *
 * Build:
 *   gcc demo.c clgraph.c -o demo -lncurses -lm
 */

#include "clgraph.h"
#include <stdio.h>
#include <string.h>

/* ── helpers ─────────────────────────────── */
static void wait_key(ClgCanvas *c) {
    clg_text(c, c->rows-1, 0, 180,180,180,
             " [ENTER] next  [q] quit ");
    wrefresh(c->win);
    nodelay(c->win, FALSE);
    int ch;
    while ((ch = wgetch(c->win)) != '\n' && ch != 'q' && ch != 'Q');
    nodelay(c->win, TRUE);
}

/* ═══════════════════════════════════════════
   DEMO 1  —  multi-function 2D plot
   ═══════════════════════════════════════════ */
static double fn_sin (double x){ return sin(x);               }
static double fn_cos (double x){ return cos(x);               }
static double fn_sinc(double x){ return x==0?1:sin(x)/x;     }
static double fn_tanh(double x){ return tanh(x);              }

void demo_2d(ClgCanvas *c) {
    ClgViewport v = { -6.5, 6.5, -1.5, 1.5, 0, 0 };
    clg_clear(c);
    clg_grid (c, &v, 1.0, 0.5, 30, 30, 30);
    clg_axes (c, &v, 100, 100, 100);
    clg_plot_fn(c, &v, fn_sin,  0, 255,  80,  80);
    clg_plot_fn(c, &v, fn_cos,  0,  80, 150, 255);
    clg_plot_fn(c, &v, fn_sinc, 0, 255, 220,  50);
    clg_plot_fn(c, &v, fn_tanh, 0,  80, 255, 150);
    clg_border (c, 60, 60, 80);
    clg_title  (c, "  sin / cos / sinc / tanh  ", 220, 220, 255);
    clg_labels (c, &v, "x", "y");
    clg_render (c);
    wait_key   (c);
}

/* ═══════════════════════════════════════════
   DEMO 2  —  parametric (Lissajous + spirograph)
   ═══════════════════════════════════════════ */
static void lissajous(double t, double *x, double *y) {
    *x = sin(3*t + M_PI/4);
    *y = sin(2*t);
}
static void spirograph(double t, double *x, double *y) {
    double R=1.0, r=0.31, d=0.6;
    *x = (R-r)*cos(t) + d*cos((R-r)/r*t);
    *y = (R-r)*sin(t) - d*sin((R-r)/r*t);
}

void demo_param(ClgCanvas *c) {
    ClgViewport v = { -1.5, 1.5, -1.5, 1.5, 0, 0 };
    clg_clear(c);
    clg_axes(c, &v, 60, 60, 60);
    clg_plot_param2(c, &v, lissajous,  0, 2*M_PI, 0.002, 255, 120,  60);
    clg_plot_param2(c, &v, spirograph, 0, 40*M_PI, 0.005, 60, 220, 255);
    clg_border(c, 60, 60, 80);
    clg_title (c, "  Lissajous + Spirograph  ", 255, 180, 80);
    clg_labels(c, &v, "x(t)", "y(t)");
    clg_render(c);
    wait_key  (c);
}

/* ═══════════════════════════════════════════
   DEMO 3  —  polar plots
   ═══════════════════════════════════════════ */
static double rose4   (double t){ return cos(4*t);              }
static double archi   (double t){ return 0.1*t;                 }
static double cardioid(double t){ return 1 + cos(t);            }
static double limacon (double t){ return 0.5 + cos(t);          }

void demo_polar(ClgCanvas *c) {
    ClgViewport v = { -2.2, 2.2, -2.2, 2.2, 0, 0 };
    clg_clear(c);
    clg_axes  (c, &v, 50, 50, 50);
    clg_plot_polar(c, &v, rose4,    0, 2*M_PI,  0.003, 255,  80,  80);
    clg_plot_polar(c, &v, archi,    0, 6*M_PI,  0.005,  80, 200, 255);
    clg_plot_polar(c, &v, cardioid, 0, 2*M_PI,  0.003, 255, 200,  60);
    clg_plot_polar(c, &v, limacon,  0, 2*M_PI,  0.003,  80, 255, 150);
    clg_border(c, 60, 60, 80);
    clg_title (c, "  Rose / Archimedean / Cardioid / Limacon  ", 180, 255, 200);
    clg_labels(c, &v, "polar", "");
    clg_render(c);
    wait_key  (c);
}

/* ═══════════════════════════════════════════
   DEMO 4  —  scatter + bar
   ═══════════════════════════════════════════ */
void demo_scatter_bar(ClgCanvas *c) {
    ClgViewport sv = { -3, 3, -3, 3, 0, 0 };
    /* random scatter */
    srand(42);
    double xs[80], ys[80];
    for (int i = 0; i < 80; i++) {
        xs[i] = ((double)rand()/RAND_MAX)*6.0 - 3.0;
        ys[i] = sin(xs[i]) + ((double)rand()/RAND_MAX)*0.8 - 0.4;
    }
    clg_clear(c);
    clg_axes    (c, &sv, 80, 80, 80);
    clg_plot_scatter(c, &sv, xs, ys, 80, 255, 160, 60);
    /* overlay the true curve */
    clg_plot_fn (c, &sv, fn_sin, 0, 80, 200, 255);
    clg_border  (c, 60, 60, 80);
    clg_title   (c, "  Scatter: sin(x) + noise  ", 255, 200, 100);
    clg_labels  (c, &sv, "x", "y");
    clg_render  (c);
    wait_key    (c);

    /* bar chart */
    ClgViewport bv = { -0.5, 5.5, 0, 30, 0, 0 };
    double bx[] = {0,1,2,3,4,5};
    double by[] = {5, 22, 14, 28, 9, 18};
    clg_clear(c);
    clg_axes   (c, &bv, 80, 80, 80);
    clg_plot_bar(c, &bv, bx, by, 6, 80, 180, 255);
    clg_border (c, 60, 60, 80);
    clg_title  (c, "  Bar Chart  ", 180, 220, 255);
    clg_labels (c, &bv, "category", "value");
    clg_render (c);
    wait_key   (c);
}

/* ═══════════════════════════════════════════
   DEMO 5  —  vector field
   ═══════════════════════════════════════════ */
static void vf_pendulum(double x, double y, double *u, double *vv) {
    /* phase portrait of simple pendulum: x=theta, y=theta_dot */
    *u  = y;
    *vv = -sin(x);
}
static void vf_spiral(double x, double y, double *u, double *vv) {
    *u  = -y;
    *vv =  x;
}

void demo_vecfield(ClgCanvas *c) {
    ClgViewport v = { -M_PI, M_PI, -2.5, 2.5, 0, 0 };
    clg_clear(c);
    clg_grid    (c, &v, 1.0, 1.0, 25, 25, 25);
    clg_axes    (c, &v, 80, 80, 80);
    clg_plot_vecfield(c, &v, vf_pendulum, 16, 10, 255, 160, 60);
    clg_border  (c, 60, 60, 80);
    clg_title   (c, "  Vector Field: Pendulum Phase Portrait  ", 255, 200, 100);
    clg_labels  (c, &v, "theta", "d_theta/dt");
    clg_render  (c);
    wait_key    (c);

    ClgViewport v2 = { -2, 2, -2, 2, 0, 0 };
    clg_clear(c);
    clg_plot_vecfield(c, &v2, vf_spiral, 14, 10, 80, 200, 255);
    clg_axes (c, &v2, 80, 80, 80);
    clg_border(c, 60, 60, 80);
    clg_title (c, "  Vector Field: Rotation  ", 100, 200, 255);
    clg_render(c);
    wait_key  (c);
}

/* ═══════════════════════════════════════════
   DEMO 6  —  heatmap + contour
   ═══════════════════════════════════════════ */
static double peaks(double x, double y) {
    return 3*(1-x)*(1-x)*exp(-(x*x)-(y+1)*(y+1))
          -10*(x/5 - x*x*x - y*y*y*y*y)*exp(-x*x-y*y)
          -1.0/3*exp(-(x+1)*(x+1)-y*y);
}
static double ripple(double x, double y) {
    return sin(sqrt(x*x + y*y)*4) / (sqrt(x*x+y*y)*0.5 + 1);
}

void demo_field(ClgCanvas *c) {
    ClgViewport v = { -3, 3, -3, 3, -8, 8 };
    clg_clear(c);
    clg_plot_heatmap(c, &v, peaks, 120);
    clg_border(c, 60, 60, 80);
    clg_title (c, "  Heatmap: peaks(x,y)  ", 255, 255, 200);
    clg_labels(c, &v, "x", "y");
    clg_render(c);
    wait_key  (c);

    /* contour */
    double levels[] = {-6,-4,-2,-1,0,1,2,4,6};
    clg_clear(c);
    clg_plot_contour(c, &v, peaks, levels, 9, 150);
    clg_axes (c, &v, 80, 80, 80);
    clg_border(c, 60, 60, 80);
    clg_title (c, "  Contour: peaks(x,y)  ", 200, 255, 200);
    clg_labels(c, &v, "x", "y");
    clg_render(c);
    wait_key  (c);

    /* ripple heatmap */
    ClgViewport v2 = { -5, 5, -5, 5, -1, 1 };
    clg_clear(c);
    clg_plot_heatmap(c, &v2, ripple, 130);
    clg_border(c, 60, 60, 80);
    clg_title (c, "  Heatmap: Ripple  ", 200, 200, 255);
    clg_render(c);
    wait_key  (c);
}

/* ═══════════════════════════════════════════
   DEMO 7  —  3D surfaces
   ═══════════════════════════════════════════ */
static double surf_saddle(double x, double y){ return x*x - y*y;       }
static double surf_wave  (double x, double y){ return sin(x)*cos(y);   }
static double surf_gauss (double x, double y){
    return exp(-(x*x+y*y)/2.0);
}

void demo_3d(ClgCanvas *c) {
    ClgViewport v  = { -2.5, 2.5, -2.5, 2.5, -8, 8 };
    ClgCamera   cam = { .rx=0.5, .ry=0.0, .rz=0.6,
                        .scale=0.55, .ox=0, .oy=0 };
    clg_clear(c);
    clg_plot_surface(c, &v, &cam, surf_wave, 40, 40, 0,0,0);
    clg_border(c, 60, 60, 80);
    clg_title (c, "  3D Surface: sin(x)*cos(y)  ", 200, 200, 255);
    clg_render(c);
    wait_key  (c);

    clg_clear(c);
    clg_plot_surface(c, &v, &cam, surf_saddle, 35, 35, 0,0,0);
    clg_border(c, 60, 60, 80);
    clg_title (c, "  3D Surface: x^2 - y^2  (saddle)  ", 255, 200, 150);
    clg_render(c);
    wait_key  (c);

    clg_clear(c);
    clg_plot_surface(c, &v, &cam, surf_gauss, 35, 35, 0,0,0);
    clg_border(c, 60, 60, 80);
    clg_title (c, "  3D Surface: Gaussian  ", 150, 255, 200);
    clg_render(c);
    wait_key  (c);
}

/* ═══════════════════════════════════════════
   DEMO 8  —  parametric 3D (torus + helix)
   ═══════════════════════════════════════════ */
static void torus(double u, double v2, double *x, double *y, double *z) {
    double R=1.0, r=0.35;
    *x = (R + r*cos(v2))*cos(u);
    *y = (R + r*cos(v2))*sin(u);
    *z = r*sin(v2);
}
static void helix(double u, double v2, double *x, double *y, double *z) {
    double t = u * 4*M_PI;
    double s = v2;  /* cross-section angle */
    double cx=cos(t), sy=sin(t);
    *x = cx*(1 + 0.1*cos(s));
    *y = sy*(1 + 0.1*cos(s));
    *z = t/(4*M_PI) - 0.5 + 0.1*sin(s);
}

void demo_param3d(ClgCanvas *c) {
    ClgViewport v   = { -2, 2, -2, 2, -2, 2 };
    ClgCamera   cam = { .rx=0.4, .ry=0.0, .rz=0.7,
                        .scale=0.7, .ox=0, .oy=0 };
    clg_clear(c);
    clg_plot_param3(c, &v, &cam, torus,
                    0, 2*M_PI, 0, 2*M_PI, 40, 20,
                    80, 180, 255);
    clg_border(c, 60, 60, 80);
    clg_title (c, "  3D Torus  ", 120, 200, 255);
    clg_render(c);
    wait_key  (c);

    clg_clear(c);
    clg_plot_param3(c, &v, &cam, helix,
                    0, 1, 0, 2*M_PI, 60, 12,
                    255, 120, 60);
    clg_border(c, 60, 60, 80);
    clg_title (c, "  3D Helix  ", 255, 160, 80);
    clg_render(c);
    wait_key  (c);
}

/* ═══════════════════════════════════════════
   DEMO 9  —  animation: rotating 3D wave
   ═══════════════════════════════════════════ */
typedef struct { ClgCamera cam; } AnimData;

static void frame_wave(ClgCanvas *c, ClgViewport *v, double t, void *ud) {
    AnimData *d = ud;
    d->cam.rz = t * 0.8;
    d->cam.rx = 0.5 + 0.2*sin(t*0.4);
    clg_plot_surface(c, v, &d->cam, surf_wave, 30, 30, 0,0,0);
    clg_border (c, 50, 50, 80);
    clg_text   (c, 0, 2, 200, 220, 255,
                " Animated 3D: sin(x)*cos(y)  t=%.2f  [q] quit ", t);
}

void demo_anim3d(ClgCanvas *c) {
    ClgViewport v   = { -3, 3, -3, 3, -2, 2 };
    AnimData    ad  = { .cam = {.rx=0.5,.ry=0,.rz=0,.scale=0.55,.ox=0,.oy=0} };
    ClgAnim    *a   = clg_anim_new(0.05, 0, 30, true);
    clg_anim_run(a, c, &v, frame_wave, &ad);
    clg_anim_free(a);
}

/* ═══════════════════════════════════════════
   DEMO 10 —  animation: 2D wave equation
   ═══════════════════════════════════════════ */
static void frame_wave2d(ClgCanvas *c, ClgViewport *v, double t, void *ud) {
    (void)ud;
    /* draw multiple traveling sinusoids */
    for (int k = 1; k <= 5; k++) {
        double phase = t * (0.5 + k*0.3);
        uint8_t r, g, b;
        clg_pal_rainbow((double)(k-1)/4.0, &r, &g, &b);
        /* lambda to capture k and phase — use static helper via macro */
        /* C doesn't have closures, so we plot via direct pixel loop */
        clg_set_color(c, r, g, b);
        int ppx=-1, ppy=-1;
        double s = (v->xmax - v->xmin) / (c->px_w * 2.0);
        for (double x = v->xmin; x <= v->xmax; x += s) {
            double y = sin(k*x - phase) / k;
            if (!isfinite(y)){ ppx=-1; continue; }
            int px = (int)((x-v->xmin)/(v->xmax-v->xmin)*(c->px_w-1));
            int py2 = (int)((1-(y-v->ymin)/(v->ymax-v->ymin))*(c->px_h-1));
            if (ppx>=0) clg_line(c, ppx, ppy, px, py2);
            ppx=px; ppy=py2;
        }
    }
    clg_axes  (c, v, 60, 60, 60);
    clg_border(c, 50, 50, 80);
    clg_text  (c, 0, 2, 200, 255, 200,
               " Fourier harmonics  t=%.2f  [q] quit ", t);
}

void demo_anim2d(ClgCanvas *c) {
    ClgViewport v = { -M_PI, M_PI, -1.5, 1.5, 0, 0 };
    ClgAnim *a = clg_anim_new(0.04, 0, 30, true);
    clg_anim_run(a, c, &v, frame_wave2d, NULL);
    clg_anim_free(a);
}

/* ═══════════════════════════════════════════
   MAIN
   ═══════════════════════════════════════════ */
int main(void) {
    ClgCanvas *c = clg_init_fullscreen();

    /* welcome screen */
    clg_clear(c);
    clg_text(c, c->rows/2-3, c->cols/2-18, 100,220,255,
             "  ██████╗██╗      ██████╗ ██████╗  █████╗ ██████╗ ██╗  ██╗");
    clg_text(c, c->rows/2-2, c->cols/2-18, 80, 180,255,
             "  CLGRAPH  —  Epic CLI Graph Library");
    clg_text(c, c->rows/2,   c->cols/2-18, 180,255,180,
             "  10 demos: 2D / parametric / polar / scatter /");
    clg_text(c, c->rows/2+1, c->cols/2-18, 180,255,180,
             "  vector field / heatmap / contour / 3D / animation");
    clg_text(c, c->rows/2+3, c->cols/2-18, 220,220,100,
             "  Press ENTER to begin ...");
    clg_render(c);
    nodelay(c->win, FALSE);
    wgetch(c->win);
    nodelay(c->win, TRUE);

    demo_2d         (c);
    demo_param      (c);
    demo_polar      (c);
    demo_scatter_bar(c);
    demo_vecfield   (c);
    demo_field      (c);
    demo_3d         (c);
    demo_param3d    (c);
    demo_anim3d     (c);   /* press q to continue */
    demo_anim2d     (c);   /* press q to continue */

    clg_free(c);
    printf("Done. All demos finished.\n");
    return 0;
}

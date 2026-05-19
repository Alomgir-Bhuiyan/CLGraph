# clgraph — Epic CLI Graph Library

A high-performance terminal graph library in pure C.  
Renders via **ncurses** + **Unicode braille** sub-pixels (each terminal cell = 2×4 dots).  
Works on Linux, macOS, and **Termux (Android)**.

```
  ██████╗██╗      ██████╗ ██████╗  █████╗ ██████╗ ██╗  ██╗
  ██╔════╝██║     ██╔════╝ ██╔══██╗██╔══██╗██╔══██╗██║  ██║
  ██║     ██║     ██║  ███╗██████╔╝███████║██████╔╝███████║
  ██║     ██║     ██║   ██║██╔══██╗██╔══██║██╔═══╝ ██╔══██║
  ╚██████╗███████╗╚██████╔╝██║  ██║██║  ██║██║     ██║  ██║
   ╚═════╝╚══════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝  ╚═╝
```

---

## Features

| Category | What you get |
|---|---|
| **2-D** | y=f(x), parametric, polar, scatter, bar chart |
| **Fields** | vector field with arrows, heatmap, contour (marching squares) |
| **3-D** | wireframe surface z=f(x,y), parametric 3-D (torus, helix, …) |
| **Animation** | ncurses frame loop, target FPS, q-to-quit, loop/oneshot |
| **Color** | true-color ncurses pairs, 5 palettes: rainbow/plasma/fire/cool/viridis |
| **Axes** | auto-axes, dashed grid, border, title, axis labels |
| **Interop** | works with `math.h`, GSL, or any numeric library |
| **Portability** | Linux / macOS / **Termux** — no X11, no GPU |

---

## Files

```
clgraph/
├── clgraph.h   — full API header (include this)
├── clgraph.c   — implementation
├── demo.c      — 10-demo showcase (great starting point)
└── Makefile
```

---

## Install & Build

### Linux / macOS

```bash
# install ncurses dev headers (one-time)
sudo apt install libncurses-dev      # Debian/Ubuntu
sudo dnf install ncurses-devel       # Fedora
brew install ncurses                 # macOS Homebrew

# build the demo
make

# run
./demo
```

### Termux (Android)

```bash
pkg install clang make ncurses-dev
make termux
./demo
```

### Build as a static library

```bash
make lib
# → libclgraph.a

# use in your project
gcc myprogram.c -I/path/to/clgraph -L/path/to/clgraph \
    -lclgraph -lncurses -lm -o myprogram
```

### With GSL

Uncomment the two GSL lines in the Makefile, then `make`.

---

## Quick Start

```c
#include "clgraph.h"
#include <math.h>

static double my_fn(double x) { return sin(x) * exp(-x*x*0.1); }

int main(void) {
    /* auto-size to terminal */
    ClgCanvas  *c = clg_init_fullscreen();
    ClgViewport v = { -5, 5, -1.2, 1.2, 0, 0 };

    clg_grid  (c, &v, 1.0, 0.5,  30, 30, 30);   /* dashed grid */
    clg_axes  (c, &v, 120, 120, 120);             /* x/y axes   */
    clg_plot_fn(c, &v, my_fn, 0, 100, 220, 80);   /* plot curve */
    clg_border(c, 60, 60, 90);
    clg_title (c, " sin(x)*e^{-0.1x²} ", 200, 200, 255);
    clg_labels(c, &v, "x", "y");
    clg_render(c);

    /* wait for any key */
    nodelay(c->win, FALSE);
    wgetch(c->win);

    clg_free(c);
    return 0;
}
```

Compile:
```bash
gcc myprogram.c clgraph.c -lncurses -lm -o myprogram
./myprogram
```

---

## Full API Reference

### Canvas Lifecycle

```c
// create canvas sized to terminal (recommended)
ClgCanvas *clg_init_fullscreen(void);

// create canvas with explicit cell size
ClgCanvas *clg_init(int cols, int rows);

// destroy canvas + end ncurses
void clg_free(ClgCanvas *c);

// clear all pixels (keep ncurses alive)
void clg_clear(ClgCanvas *c);

// flush pixel buffer → ncurses window → terminal
void clg_render(ClgCanvas *c);

// set current draw color (RGB 0-255 each)
void clg_set_color(ClgCanvas *c, uint8_t r, uint8_t g, uint8_t b);
```

### Viewport

```c
ClgViewport v = {
    .xmin = -5, .xmax = 5,   // x math range
    .ymin = -2, .ymax = 2,   // y math range
    .zmin = -1, .zmax = 1    // z range (for 3D color mapping)
};
```

### Primitives

```c
void clg_pixel (c, px, py);                    // set one sub-pixel
void clg_line  (c, x0, y0, x1, y1);           // Bresenham line
void clg_rect  (c, x, y, w, h);               // hollow rectangle
void clg_circle(c, cx, cy, r);                // midpoint circle
void clg_fill  (c, x, y, w, h);               // filled rectangle
void clg_text  (c, row, col, r, g, b, fmt, ...); // colored text overlay
```

Note: pixel coords here are **sub-pixel** coords (px_w = cols×2, px_h = rows×4).

### 2-D Plots

```c
// y = f(x) — step=0 means auto (pixel-resolution)
void clg_plot_fn(c, &v, fn, step, r, g, b);

// parametric: (x(t), y(t))
void clg_plot_param2(c, &v, fn, t0, t1, step, r, g, b);

// polar: r = f(theta)
void clg_plot_polar(c, &v, fn, theta0, theta1, step, r, g, b);

// scatter plot (xs[], ys[], n points)
void clg_plot_scatter(c, &v, xs, ys, n, r, g, b);

// bar chart
void clg_plot_bar(c, &v, xs, ys, n, r, g, b);

// vector field: (u(x,y), v(x,y)), gcols×grows grid
void clg_plot_vecfield(c, &v, fn, gcols, grows, r, g, b);
```

Function pointer signatures:
```c
typedef double (*ClgFn1D)    (double x);
typedef double (*ClgFnPolar) (double theta);
typedef void   (*ClgFnParam2)(double t, double *x, double *y);
typedef void   (*ClgFnVec2)  (double x, double y, double *u, double *v);
```

### Field Plots

```c
// heatmap: f(x,y) → plasma color, samples×samples grid
void clg_plot_heatmap(c, &v, fn2d, samples);

// contour lines (marching squares)
double levels[] = {-2, -1, 0, 1, 2};
void clg_plot_contour(c, &v, fn2d, levels, 5, samples);
```

```c
typedef double (*ClgFn2D)(double x, double y);
```

### 3-D Plots

```c
// camera setup
ClgCamera cam = {
    .rx = 0.5,    // pitch (radians)
    .ry = 0.0,    // yaw
    .rz = 0.6,    // roll
    .scale = 0.55, // zoom (0.3 far … 1.0 close)
    .ox = 0,      // x offset (fraction of canvas)
    .oy = 0       // y offset
};

// wireframe z = f(x,y)
void clg_plot_surface(c, &v, &cam, fn3d, usteps, vsteps, r, g, b);

// parametric 3-D surface: (x,y,z) = f(u,v)
void clg_plot_param3(c, &v, &cam, fn,
                     u0, u1, v0, v1, usteps, vsteps, r, g, b);

// manual projection (for custom geometry)
void clg_project3d(&cam, c, x, y, z, &px, &py);
```

```c
typedef double (*ClgFn3D)    (double x, double y);
typedef void   (*ClgFnParam3)(double u, double v,
                               double *x, double *y, double *z);
```

### Animation

```c
// dt = time step per frame, t_end <= 0 means run forever, loop = restart
ClgAnim *a = clg_anim_new(dt, t_end, fps, loop);

// frame callback signature
typedef void (*ClgFrameFn)(ClgCanvas *c, ClgViewport *v,
                            double t, void *userdata);

// run loop (blocks until done or user presses 'q')
clg_anim_run(a, c, &v, my_frame_fn, userdata);
clg_anim_free(a);
```

Example animated frame:
```c
static void my_frame(ClgCanvas *c, ClgViewport *v, double t, void *ud) {
    // draw stuff for time t
    clg_set_color(c, 100, 200, 255);
    int ppx=-1, ppy=-1;
    double s = (v->xmax-v->xmin)/(c->px_w*2.0);
    for (double x = v->xmin; x <= v->xmax; x += s) {
        double y = sin(x - t);
        int px = (int)((x-v->xmin)/(v->xmax-v->xmin)*(c->px_w-1));
        int py = (int)((1-(y-v->ymin)/(v->ymax-v->ymin))*(c->px_h-1));
        if (ppx>=0) clg_line(c, ppx, ppy, px, py);
        ppx=px; ppy=py;
    }
    clg_axes(c, v, 80,80,80);
}

// in main:
ClgAnim *a = clg_anim_new(0.05, 0, 30, true);  // 30 fps, loop forever
clg_anim_run(a, c, &v, my_frame, NULL);
clg_anim_free(a);
```

### Decoration

```c
void clg_axes  (c, &v, r, g, b);              // draw x/y axes through origin
void clg_grid  (c, &v, xstep, ystep, r,g,b); // dashed grid
void clg_border(c, r, g, b);                  // canvas border
void clg_title (c, "My Plot", r, g, b);       // title in first row
void clg_labels(c, &v, "x axis", "y axis");   // axis labels + range info
```

### Color Palettes

All take `t ∈ [0, 1]` and fill R, G, B (uint8_t).

```c
clg_pal_rainbow(t, &r, &g, &b);   // full hue sweep
clg_pal_plasma (t, &r, &g, &b);   // purple → yellow (like matplotlib)
clg_pal_fire   (t, &r, &g, &b);   // black → red → yellow
clg_pal_cool   (t, &r, &g, &b);   // teal → ice blue
clg_pal_viridis(t, &r, &g, &b);   // dark purple → green → yellow
```

### Utility

```c
void   clg_term_size(int *cols, int *rows); // current terminal dimensions
void   clg_sleep_ms (int ms);               // portable nanosleep wrapper
double clg_now_ms   (void);                 // monotonic clock in ms
```

---

## Plotting Mathematical Equations

### Arbitrary y = f(x)

```c
#include <gsl/gsl_sf_bessel.h>

double bessel_j0(double x) { return gsl_sf_bessel_J0(x); }
double bessel_j1(double x) { return gsl_sf_bessel_J1(x); }

ClgViewport v = { 0, 20, -0.5, 1.1, 0,0 };
clg_plot_fn(c, &v, bessel_j0, 0, 255,100,100);
clg_plot_fn(c, &v, bessel_j1, 0, 100,100,255);
```

### Complex function magnitude |f(z)|

```c
// plot |z^3 - 1| as a heatmap on the complex plane
double cpx_mag(double x, double y) {
    // z = x + iy,  f(z) = z^3 - 1
    double re = x*x*x - 3*x*y*y - 1;
    double im = 3*x*x*y - y*y*y;
    return sqrt(re*re + im*im);
}
ClgViewport v = { -2, 2, -2, 2, 0, 4 };
clg_plot_heatmap(c, &v, cpx_mag, 150);
```

### Phase portrait (ODE)

```c
// Lorenz system — project x vs z
void lorenz_xz(double t, double *ox, double *oy) {
    static double sx=1, sy=1, sz=1;
    double dt=0.01;
    double dx = 10*(sy-sx), dy = sx*(28-sz)-sy, dz = sx*sy - (8.0/3)*sz;
    sx+=dx*dt; sy+=dy*dt; sz+=dz*dt;
    *ox = sx; *oy = sz;
    (void)t;
}
ClgViewport v = { -25, 25, 0, 50, 0,0 };
clg_plot_param2(c, &v, lorenz_xz, 0, 100, 0.01, 80, 200, 255);
```

### Animated 3-D rotation

```c
typedef struct { double angle; } MyData;

void frame(ClgCanvas *c, ClgViewport *v, double t, void *ud) {
    MyData *d = ud;
    ClgCamera cam = { .rx=0.5, .ry=0, .rz=t*0.7, .scale=0.55 };
    clg_plot_surface(c, v, &cam, my_surface_fn, 35, 35, 0,0,0);
}

MyData d = {0};
ClgAnim *a = clg_anim_new(0.04, 0, 30, true);
clg_anim_run(a, c, &v, frame, &d);
```

---

## Demo Gallery

Run `./demo` to see all 10 demos:

| # | Demo | Keys |
|---|------|------|
| 1 | sin / cos / sinc / tanh | ENTER |
| 2 | Lissajous + Spirograph | ENTER |
| 3 | Rose / Archimedean / Cardioid / Limaçon | ENTER |
| 4 | Scatter plot + Bar chart | ENTER |
| 5 | Vector field: pendulum + rotation | ENTER |
| 6 | Heatmap + Contour (peaks, ripple) | ENTER |
| 7 | 3-D wireframe (wave, saddle, Gaussian) | ENTER |
| 8 | Parametric 3-D (Torus + Helix) | ENTER |
| 9 | **Animated** rotating 3-D surface | q |
| 10 | **Animated** Fourier harmonics | q |

---

## Notes on Termux

Termux may not support `TIOCGWINSZ` over SSH — in that case set terminal size manually:

```c
ClgCanvas *c = clg_init(100, 40);  // explicit size instead of fullscreen
```

Also ensure your terminal emulator app (e.g. Termux itself, or ConnectBot) uses a font that supports Unicode braille (U+2800–U+28FF). **Most modern fonts do.**

---

## License

MIT — do whatever you want with it.

/*
 * clgraph.c  —  Epic CLI Graph Library (ncurses backend)
 * See clgraph.h for API docs.
 */

#include "clgraph.h"
#include <signal.h>

/* ═══════════════════════════════════════════════════════════
   INTERNAL HELPERS
   ═══════════════════════════════════════════════════════════ */

static inline void _set_px(ClgCanvas *c, int px, int py) {
    if (px < 0 || py < 0 || px >= c->px_w || py >= c->px_h) return;
    int idx = py * c->px_w + px;
    c->bits[idx / 8] |= (uint8_t)(1u << (idx % 8));
    /* tag the parent cell with the current color pair */
    int cx = px / 2, cy = py / 4;
    c->cpair[cy * c->cols + cx] = c->cur_pair;
}

static inline bool _get_px(ClgCanvas *c, int px, int py) {
    if (px < 0 || py < 0 || px >= c->px_w || py >= c->px_h) return false;
    int idx = py * c->px_w + px;
    return (c->bits[idx / 8] >> (idx % 8)) & 1u;
}

static inline int _math2px(ClgViewport *v, ClgCanvas *c, double x) {
    return (int)((x - v->xmin) / (v->xmax - v->xmin) * (c->px_w - 1));
}
static inline int _math2py(ClgViewport *v, ClgCanvas *c, double y) {
    return (int)((1.0 - (y - v->ymin) / (v->ymax - v->ymin)) * (c->px_h - 1));
}

/* Allocate or reuse an ncurses color pair for a 24-bit RGB color.
   ncurses only has 64-1000 pairs; we cycle through CLG_MAX_COLORS. */
static short g_pair_counter = 1;  /* 0 is reserved by ncurses */

static short _alloc_pair(uint8_t r, uint8_t g, uint8_t b) {
    if (!has_colors()) return 0;
    /* scale 0-255 → 0-1000 (ncurses color range) */
    short cr = (short)(r * 1000 / 255);
    short cg = (short)(g * 1000 / 255);
    short cb = (short)(b * 1000 / 255);
    short color_id = g_pair_counter; /* use pair id as color id too */
    init_color(color_id, cr, cg, cb);
    init_pair (color_id, color_id, -1);  /* -1 = transparent bg */
    short pair = color_id;
    g_pair_counter++;
    if (g_pair_counter >= CLG_MAX_COLORS) g_pair_counter = 1;
    return pair;
}

/* ═══════════════════════════════════════════════════════════
   CANVAS LIFECYCLE
   ═══════════════════════════════════════════════════════════ */

ClgCanvas *clg_init(int cols, int rows) {
    ClgCanvas *c = calloc(1, sizeof(ClgCanvas));
    c->cols  = cols;
    c->rows  = rows;
    c->px_w  = cols * 2;
    c->px_h  = rows * 4;

    int total = c->px_w * c->px_h;
    c->bits  = calloc((total + 7) / 8, 1);
    c->cpair = calloc(cols * rows, sizeof(short));

    /* ncurses init */
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(0);

    c->has_colors = has_colors();
    if (c->has_colors) {
        start_color();
        use_default_colors();
    }

    c->win = newwin(rows, cols, 0, 0);
    scrollok(c->win, FALSE);
    c->cur_pair = 0;
    c->fg = 0xFFFFFF;
    return c;
}

ClgCanvas *clg_init_fullscreen(void) {
    int cols, rows;
    /* probe real terminal size before initscr */
    initscr();
    getmaxyx(stdscr, rows, cols);
    endwin();
    rows -= 2; /* leave 2 rows for labels */
    if (rows < 4)  rows = 4;
    if (cols < 10) cols = 10;
    return clg_init(cols, rows);
}

void clg_free(ClgCanvas *c) {
    if (!c) return;
    delwin(c->win);
    endwin();
    free(c->bits);
    free(c->cpair);
    free(c);
}

void clg_clear(ClgCanvas *c) {
    int total = c->px_w * c->px_h;
    memset(c->bits,  0, (total + 7) / 8);
    memset(c->cpair, 0, c->cols * c->rows * sizeof(short));
    werase(c->win);
}

/* flush braille canvas → ncurses window → screen */
void clg_render(ClgCanvas *c) {
    for (int row = 0; row < c->rows; row++) {
        for (int col = 0; col < c->cols; col++) {
            /* build braille byte from 2×4 sub-pixels */
            uint8_t b8 = 0;
            for (int r4 = 0; r4 < 4; r4++)
                for (int c2 = 0; c2 < 2; c2++)
                    if (_get_px(c, col*2 + c2, row*4 + r4))
                        b8 |= (uint8_t)(1u << CLG_DOT[r4][c2]);

            if (b8 == 0) {
                mvwaddch(c->win, row, col, ' ');
                continue;
            }

            /* build UTF-8 for braille codepoint U+2800+b8 */
            uint32_t cp = CLG_BRAILLE_BASE + b8;
            char utf8[4];
            utf8[0] = (char)(0xE0 | (cp >> 12));
            utf8[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
            utf8[2] = (char)(0x80 | (cp & 0x3F));
            utf8[3] = '\0';

            short pair = c->cpair[row * c->cols + col];
            if (c->has_colors && pair > 0) {
                wattron(c->win, COLOR_PAIR(pair));
                mvwaddstr(c->win, row, col, utf8);
                wattroff(c->win, COLOR_PAIR(pair));
            } else {
                mvwaddstr(c->win, row, col, utf8);
            }
        }
    }
    wrefresh(c->win);
}

void clg_set_color(ClgCanvas *c, uint8_t r, uint8_t g, uint8_t b) {
    c->fg = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    if (c->has_colors)
        c->cur_pair = _alloc_pair(r, g, b);
}

/* ═══════════════════════════════════════════════════════════
   PRIMITIVES
   ═══════════════════════════════════════════════════════════ */

void clg_pixel(ClgCanvas *c, int px, int py) {
    _set_px(c, px, py);
}

void clg_line(ClgCanvas *c, int x0, int y0, int x1, int y1) {
    int dx = abs(x1-x0), dy = -abs(y1-y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        _set_px(c, x0, y0);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void clg_rect(ClgCanvas *c, int x, int y, int w, int h) {
    clg_line(c, x,     y,     x+w-1, y);
    clg_line(c, x,     y+h-1, x+w-1, y+h-1);
    clg_line(c, x,     y,     x,     y+h-1);
    clg_line(c, x+w-1, y,     x+w-1, y+h-1);
}

void clg_circle(ClgCanvas *c, int cx, int cy, int r) {
    int x = 0, y = r, d = 3 - 2*r;
    while (x <= y) {
        _set_px(c, cx+x, cy+y); _set_px(c, cx-x, cy+y);
        _set_px(c, cx+x, cy-y); _set_px(c, cx-x, cy-y);
        _set_px(c, cx+y, cy+x); _set_px(c, cx-y, cy+x);
        _set_px(c, cx+y, cy-x); _set_px(c, cx-y, cy-x);
        if (d < 0) d += 4*x + 6;
        else       { d += 4*(x-y) + 10; y--; }
        x++;
    }
}

void clg_fill(ClgCanvas *c, int x, int y, int w, int h) {
    for (int row = y; row < y+h; row++)
        for (int col = x; col < x+w; col++)
            _set_px(c, col, row);
}

void clg_text(ClgCanvas *c, int row, int col,
              uint8_t r, uint8_t g, uint8_t b,
              const char *fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    short pair = 0;
    if (c->has_colors) pair = _alloc_pair(r, g, b);
    if (pair > 0) wattron(c->win, COLOR_PAIR(pair) | A_BOLD);
    mvwaddstr(c->win, row, col, buf);
    if (pair > 0) wattroff(c->win, COLOR_PAIR(pair) | A_BOLD);
}

/* ═══════════════════════════════════════════════════════════
   2-D PLOTS
   ═══════════════════════════════════════════════════════════ */

void clg_plot_fn(ClgCanvas *c, ClgViewport *v,
                 ClgFn1D f, double step,
                 uint8_t r, uint8_t g, uint8_t b) {
    clg_set_color(c, r, g, b);
    double s = step > 0 ? step : (v->xmax - v->xmin) / (c->px_w * 2.0);
    int ppx = -1, ppy = -1;
    for (double x = v->xmin; x <= v->xmax + s*0.5; x += s) {
        double y = f(x);
        if (!isfinite(y)) { ppx = -1; continue; }
        int px = _math2px(v, c, x);
        int py = _math2py(v, c, y);
        if (ppx >= 0) clg_line(c, ppx, ppy, px, py);
        else          _set_px(c, px, py);
        ppx = px; ppy = py;
    }
}

void clg_plot_param2(ClgCanvas *c, ClgViewport *v,
                     ClgFnParam2 f,
                     double t0, double t1, double step,
                     uint8_t r, uint8_t g, uint8_t b) {
    clg_set_color(c, r, g, b);
    int ppx = -1, ppy = -1;
    for (double t = t0; t <= t1 + step*0.5; t += step) {
        double x, y;
        f(t, &x, &y);
        if (!isfinite(x) || !isfinite(y)) { ppx = -1; continue; }
        int px = _math2px(v, c, x);
        int py = _math2py(v, c, y);
        if (ppx >= 0) clg_line(c, ppx, ppy, px, py);
        else          _set_px(c, px, py);
        ppx = px; ppy = py;
    }
}

void clg_plot_polar(ClgCanvas *c, ClgViewport *v,
                    ClgFnPolar f,
                    double t0, double t1, double step,
                    uint8_t r, uint8_t g, uint8_t b) {
    clg_set_color(c, r, g, b);
    int ppx = -1, ppy = -1;
    for (double th = t0; th <= t1 + step*0.5; th += step) {
        double rr = f(th);
        double x  = rr * cos(th);
        double y  = rr * sin(th);
        if (!isfinite(x) || !isfinite(y)) { ppx = -1; continue; }
        int px = _math2px(v, c, x);
        int py = _math2py(v, c, y);
        if (ppx >= 0) clg_line(c, ppx, ppy, px, py);
        else          _set_px(c, px, py);
        ppx = px; ppy = py;
    }
}

void clg_plot_scatter(ClgCanvas *c, ClgViewport *v,
                      double *xs, double *ys, int n,
                      uint8_t r, uint8_t g, uint8_t b) {
    clg_set_color(c, r, g, b);
    for (int i = 0; i < n; i++) {
        int px = _math2px(v, c, xs[i]);
        int py = _math2py(v, c, ys[i]);
        /* draw small diamond */
        clg_line(c, px-2, py,   px,   py-2);
        clg_line(c, px,   py-2, px+2, py);
        clg_line(c, px+2, py,   px,   py+2);
        clg_line(c, px,   py+2, px-2, py);
    }
}

void clg_plot_bar(ClgCanvas *c, ClgViewport *v,
                  double *xs, double *ys, int n,
                  uint8_t r, uint8_t g, uint8_t b) {
    clg_set_color(c, r, g, b);
    int zero = _math2py(v, c, 0.0);
    int bw   = (c->px_w / n) - 2;
    if (bw < 1) bw = 1;
    for (int i = 0; i < n; i++) {
        int px = _math2px(v, c, xs[i]);
        int py = _math2py(v, c, ys[i]);
        int by = py < zero ? py : zero;
        int bh = abs(zero - py);
        if (bh < 1) bh = 1;
        clg_fill(c, px - bw/2, by, bw, bh);
    }
}

void clg_plot_vecfield(ClgCanvas *c, ClgViewport *v,
                       ClgFnVec2 f, int gcols, int grows,
                       uint8_t r, uint8_t g, uint8_t b) {
    clg_set_color(c, r, g, b);
    double dx = (v->xmax - v->xmin) / gcols;
    double dy = (v->ymax - v->ymin) / grows;
    double scale = 0.35 * (dx < dy ? dx : dy);

    for (int gi = 0; gi <= gcols; gi++) {
        for (int gj = 0; gj <= grows; gj++) {
            double x = v->xmin + gi * dx;
            double y = v->ymin + gj * dy;
            double u, vv;
            f(x, y, &u, &vv);
            double mag = sqrt(u*u + vv*vv);
            if (mag < 1e-12) continue;
            u /= mag; vv /= mag;
            u *= scale; vv *= scale;

            int x0 = _math2px(v, c, x);
            int y0 = _math2py(v, c, y);
            int x1 = _math2px(v, c, x + u);
            int y1 = _math2py(v, c, y + vv);
            clg_line(c, x0, y0, x1, y1);

            /* arrowhead */
            double ang = atan2(vv, u);
            double ah  = scale * 0.4;
            clg_line(c, x1, y1,
                     _math2px(v, c, x+u - ah*cos(ang+0.5)),
                     _math2py(v, c, y+vv - ah*sin(ang+0.5)));
            clg_line(c, x1, y1,
                     _math2px(v, c, x+u - ah*cos(ang-0.5)),
                     _math2py(v, c, y+vv - ah*sin(ang-0.5)));
        }
    }
}

/* ═══════════════════════════════════════════════════════════
   HEATMAP & CONTOUR  (marching squares)
   ═══════════════════════════════════════════════════════════ */

void clg_plot_heatmap(ClgCanvas *c, ClgViewport *v,
                      ClgFn2D f, int samples) {
    int sx = samples, sy = samples;
    double *z   = malloc(sizeof(double) * sx * sy);
    double zmin =  1e18, zmax = -1e18;

    for (int j = 0; j < sy; j++)
        for (int i = 0; i < sx; i++) {
            double x = v->xmin + (v->xmax-v->xmin)*i/(sx-1);
            double y = v->ymin + (v->ymax-v->ymin)*j/(sy-1);
            double val = f(x, y);
            z[j*sx+i] = val;
            if (isfinite(val)) {
                if (val < zmin) zmin = val;
                if (val > zmax) zmax = val;
            }
        }

    double zrng = zmax - zmin;
    if (zrng < 1e-12) zrng = 1.0;

    for (int j = 0; j < sy; j++) {
        for (int i = 0; i < sx; i++) {
            double val = z[j*sx+i];
            if (!isfinite(val)) continue;
            double t = (val - zmin) / zrng;
            uint8_t r, g, b;
            clg_pal_plasma(t, &r, &g, &b);
            clg_set_color(c, r, g, b);
            int px = _math2px(v, c, v->xmin+(v->xmax-v->xmin)*i/(sx-1));
            int py = _math2py(v, c, v->ymin+(v->ymax-v->ymin)*j/(sy-1));
            _set_px(c, px, py);
        }
    }
    free(z);
}

void clg_plot_contour(ClgCanvas *c, ClgViewport *v,
                      ClgFn2D f,
                      double *levels, int n_levels,
                      int samples) {
    int sx = samples, sy = samples;
    double *z = malloc(sizeof(double) * sx * sy);
    for (int j = 0; j < sy; j++)
        for (int i = 0; i < sx; i++) {
            double x = v->xmin + (v->xmax-v->xmin)*i/(sx-1);
            double y = v->ymin + (v->ymax-v->ymin)*j/(sy-1);
            z[j*sx+i] = f(x, y);
        }

    for (int li = 0; li < n_levels; li++) {
        double lv = levels[li];
        double t  = (double)li / (n_levels > 1 ? n_levels-1 : 1);
        uint8_t r, g, b;
        clg_pal_rainbow(t, &r, &g, &b);
        clg_set_color(c, r, g, b);

        #define CROSS(a,b) (((a)<lv) != ((b)<lv))
        for (int j = 0; j < sy-1; j++) {
            for (int i = 0; i < sx-1; i++) {
                double z00=z[j*sx+i], z10=z[j*sx+(i+1)];
                double z01=z[(j+1)*sx+i], z11=z[(j+1)*sx+(i+1)];
                if (!CROSS(z00,z10) && !CROSS(z00,z01) &&
                    !CROSS(z10,z11) && !CROSS(z01,z11)) continue;

                double x0=v->xmin+(v->xmax-v->xmin)*i    /(sx-1);
                double x1=v->xmin+(v->xmax-v->xmin)*(i+1)/(sx-1);
                double y0=v->ymin+(v->ymax-v->ymin)*j    /(sy-1);
                double y1=v->ymin+(v->ymax-v->ymin)*(j+1)/(sy-1);

                double pts[8]; int np=0;
                #define LERP(a,b,za,zb) ((a) + ((lv-(za))/((zb)-(za)))*((b)-(a)))
                if(CROSS(z00,z10)){pts[np++]=LERP(x0,x1,z00,z10);pts[np++]=y0;}
                if(CROSS(z10,z11)){pts[np++]=x1;pts[np++]=LERP(y0,y1,z10,z11);}
                if(CROSS(z01,z11)){pts[np++]=LERP(x0,x1,z01,z11);pts[np++]=y1;}
                if(CROSS(z00,z01)){pts[np++]=x0;pts[np++]=LERP(y0,y1,z00,z01);}
                #undef LERP

                if(np>=4)
                    clg_line(c,
                        _math2px(v,c,pts[0]),_math2py(v,c,pts[1]),
                        _math2px(v,c,pts[2]),_math2py(v,c,pts[3]));
                if(np>=8)
                    clg_line(c,
                        _math2px(v,c,pts[4]),_math2py(v,c,pts[5]),
                        _math2px(v,c,pts[6]),_math2py(v,c,pts[7]));
            }
        }
        #undef CROSS
    }
    free(z);
}

/* ═══════════════════════════════════════════════════════════
   3-D PROJECTION & SURFACE
   ═══════════════════════════════════════════════════════════ */

void clg_project3d(ClgCamera *cam, ClgCanvas *c,
                   double x, double y, double z,
                   int *out_px, int *out_py) {
    /* combined Rz * Ry * Rx rotation */
    double crx=cos(cam->rx), srx=sin(cam->rx);
    double cry=cos(cam->ry), sry=sin(cam->ry);
    double crz=cos(cam->rz), srz=sin(cam->rz);

    double x1 = crz*x - srz*y;
    double y1 = srz*x + crz*y;
    double z1 = z;

    double x2 =  cry*x1 + sry*z1;
    double y2 =  y1;
    double z2 = -sry*x1 + cry*z1;

    double x3 = x2;
    double y3 = crx*y2 - srx*z2;

    double hw = c->px_w * 0.5;
    double hh = c->px_h * 0.5;
    *out_px = (int)(hw + (cam->ox + x3) * cam->scale * hw);
    *out_py = (int)(hh - (cam->oy + y3) * cam->scale * hh);
}

void clg_plot_surface(ClgCanvas *c, ClgViewport *v,
                      ClgCamera *cam,
                      ClgFn3D f, int usteps, int vsteps,
                      uint8_t r, uint8_t g, uint8_t b) {
    double zrng = v->zmax - v->zmin;
    if (zrng < 1e-12) zrng = 1.0;
    (void)r; (void)g; (void)b;   /* z-mapped color overrides */

    for (int j = 0; j < vsteps; j++) {
        for (int i = 0; i < usteps; i++) {
            double x0=v->xmin+(v->xmax-v->xmin)*i    /(usteps-1);
            double x1=v->xmin+(v->xmax-v->xmin)*(i+1)/(usteps-1);
            double y0=v->ymin+(v->ymax-v->ymin)*j    /(vsteps-1);
            double y1=v->ymin+(v->ymax-v->ymin)*(j+1)/(vsteps-1);

            double z00=f(x0,y0), z10=f(x1,y0), z01=f(x0,y1);
            if (!isfinite(z00)||!isfinite(z10)||!isfinite(z01)) continue;

            uint8_t cr, cg, cb;
            clg_pal_plasma((z00-v->zmin)/zrng, &cr, &cg, &cb);
            clg_set_color(c, cr, cg, cb);

            int px00,py00,px10,py10,px01,py01;
            clg_project3d(cam,c,x0,y0,z00,&px00,&py00);
            clg_project3d(cam,c,x1,y0,z10,&px10,&py10);
            clg_project3d(cam,c,x0,y1,z01,&px01,&py01);
            clg_line(c,px00,py00,px10,py10);
            clg_line(c,px00,py00,px01,py01);
        }
    }
}

void clg_plot_param3(ClgCanvas *c, ClgViewport *v,
                     ClgCamera *cam,
                     ClgFnParam3 f,
                     double u0, double u1,
                     double v0, double v1,
                     int usteps, int vsteps,
                     uint8_t r, uint8_t g, uint8_t b) {
    (void)v;
    clg_set_color(c, r, g, b);
    /* draw iso-u lines */
    for (int i = 0; i <= usteps; i++) {
        double uu = u0 + (u1-u0)*i/usteps;
        int ppx=-1, ppy=-1;
        for (int j = 0; j <= vsteps; j++) {
            double vv = v0 + (v1-v0)*j/vsteps;
            double x,y,z;
            f(uu, vv, &x, &y, &z);
            if (!isfinite(x)||!isfinite(y)||!isfinite(z)){ppx=-1;continue;}
            int px, py;
            clg_project3d(cam,c,x,y,z,&px,&py);
            if (ppx>=0) clg_line(c,ppx,ppy,px,py);
            ppx=px; ppy=py;
        }
    }
    /* draw iso-v lines */
    for (int j = 0; j <= vsteps; j++) {
        double vv = v0 + (v1-v0)*j/vsteps;
        int ppx=-1, ppy=-1;
        for (int i = 0; i <= usteps; i++) {
            double uu = u0 + (u1-u0)*i/usteps;
            double x,y,z;
            f(uu, vv, &x, &y, &z);
            if (!isfinite(x)||!isfinite(y)||!isfinite(z)){ppx=-1;continue;}
            int px, py;
            clg_project3d(cam,c,x,y,z,&px,&py);
            if (ppx>=0) clg_line(c,ppx,ppy,px,py);
            ppx=px; ppy=py;
        }
    }
}

/* ═══════════════════════════════════════════════════════════
   DECORATION
   ═══════════════════════════════════════════════════════════ */

void clg_axes(ClgCanvas *c, ClgViewport *v,
              uint8_t r, uint8_t g, uint8_t b) {
    clg_set_color(c, r, g, b);
    if (v->ymin <= 0.0 && v->ymax >= 0.0)
        clg_line(c, 0, _math2py(v,c,0.0), c->px_w-1, _math2py(v,c,0.0));
    if (v->xmin <= 0.0 && v->xmax >= 0.0)
        clg_line(c, _math2px(v,c,0.0), 0, _math2px(v,c,0.0), c->px_h-1);
}

void clg_grid(ClgCanvas *c, ClgViewport *v,
              double xstep, double ystep,
              uint8_t r, uint8_t g, uint8_t b) {
    clg_set_color(c, r, g, b);
    /* vertical dashed */
    double x = ceil(v->xmin/xstep)*xstep;
    for (; x <= v->xmax; x += xstep) {
        int px = _math2px(v,c,x);
        for (int py = 0; py < c->px_h; py += 10)
            clg_line(c, px, py, px, py+4);
    }
    /* horizontal dashed */
    double y = ceil(v->ymin/ystep)*ystep;
    for (; y <= v->ymax; y += ystep) {
        int py = _math2py(v,c,y);
        for (int px = 0; px < c->px_w; px += 10)
            clg_line(c, px, py, px+4, py);
    }
}

void clg_border(ClgCanvas *c, uint8_t r, uint8_t g, uint8_t b) {
    clg_set_color(c, r, g, b);
    clg_rect(c, 0, 0, c->px_w, c->px_h);
}

void clg_labels(ClgCanvas *c, ClgViewport *v,
                const char *xlabel, const char *ylabel) {
    int lrow = c->rows;   /* row below canvas */
    /* x label centered */
    int xpos = c->cols/2 - (int)strlen(xlabel)/2;
    if (xpos < 0) xpos = 0;
    clg_text(c, lrow, xpos, 255, 220, 80, "%s", xlabel);
    /* y label left side */
    clg_text(c, c->rows/2, 0, 255, 220, 80, "%s", ylabel);
    /* axis ranges */
    clg_text(c, lrow+1, 0, 100, 200, 255,
             "x[%.3g,%.3g]  y[%.3g,%.3g]",
             v->xmin, v->xmax, v->ymin, v->ymax);
    wrefresh(c->win);
}

void clg_title(ClgCanvas *c, const char *title,
               uint8_t r, uint8_t g, uint8_t b) {
    int col = c->cols/2 - (int)strlen(title)/2;
    if (col < 0) col = 0;
    /* title goes in first row of canvas */
    clg_text(c, 0, col, r, g, b, "%s", title);
    wrefresh(c->win);
}

/* ═══════════════════════════════════════════════════════════
   ANIMATION
   ═══════════════════════════════════════════════════════════ */

static volatile bool g_stop = false;
static void _sig(int s) { (void)s; g_stop = true; }

ClgAnim *clg_anim_new(double dt, double t_end, int fps, bool loop) {
    ClgAnim *a = calloc(1, sizeof(ClgAnim));
    a->dt    = dt;
    a->t_end = t_end;
    a->fps   = fps > 0 ? fps : 30;
    a->loop  = loop;
    return a;
}

void clg_anim_run(ClgAnim *a, ClgCanvas *c, ClgViewport *v,
                  ClgFrameFn fn, void *userdata) {
    g_stop = false;
    signal(SIGINT, _sig);

    double frame_ms = 1000.0 / a->fps;

    while (!g_stop) {
        if (a->t_end > 0 && a->t >= a->t_end) {
            if (a->loop) a->t = 0.0;
            else break;
        }

        double t0 = clg_now_ms();
        clg_clear(c);
        fn(c, v, a->t, userdata);
        clg_render(c);
        a->t += a->dt;

        /* handle 'q' key to quit */
        int ch = wgetch(c->win);
        if (ch == 'q' || ch == 'Q') break;

        double elapsed = clg_now_ms() - t0;
        int wait = (int)(frame_ms - elapsed);
        if (wait > 0) clg_sleep_ms(wait);
    }
}

void clg_anim_free(ClgAnim *a) { free(a); }

/* ═══════════════════════════════════════════════════════════
   COLOR PALETTES
   ═══════════════════════════════════════════════════════════ */

void clg_pal_rainbow(double t, uint8_t *r, uint8_t *g, uint8_t *b) {
    t = fmod(t < 0 ? t+1 : t, 1.0);
    double h = t * 6.0;
    int    i = (int)h % 6;
    double f = h - (int)h, q = 1.0-f;
    double tbl[6][3] = {
        {1,f,0},{q,1,0},{0,1,f},{0,q,1},{f,0,1},{1,0,q}
    };
    *r=(uint8_t)(tbl[i][0]*255);
    *g=(uint8_t)(tbl[i][1]*255);
    *b=(uint8_t)(tbl[i][2]*255);
}

void clg_pal_plasma(double t, uint8_t *r, uint8_t *g, uint8_t *b) {
    t = t<0?0:t>1?1:t;
    /* piecewise approximation of matplotlib plasma */
    double rv = 0.050 + 2.747*t - 2.040*t*t;
    double gv = 0.034 - 0.175*t + 2.180*t*t - 1.700*t*t*t;
    double bv = 0.527 + 0.748*t - 4.200*t*t + 3.700*t*t*t;
    #define CLAMP01(x) ((x)<0?0:(x)>1?1:(x))
    *r=(uint8_t)(CLAMP01(rv)*255);
    *g=(uint8_t)(CLAMP01(gv)*255);
    *b=(uint8_t)(CLAMP01(bv)*255);
    #undef CLAMP01
}

void clg_pal_fire(double t, uint8_t *r, uint8_t *g, uint8_t *b) {
    t = t<0?0:t>1?1:t;
    *r = (uint8_t)(t < 0.5 ? t*2*255 : 255);
    *g = (uint8_t)(t < 0.5 ? 0       : (t-0.5)*2*255);
    *b = 0;
}

void clg_pal_cool(double t, uint8_t *r, uint8_t *g, uint8_t *b) {
    t = t<0?0:t>1?1:t;
    *r = (uint8_t)(t * 120);
    *g = (uint8_t)(100 + t*155);
    *b = (uint8_t)(210 + t*45);
}

void clg_pal_viridis(double t, uint8_t *r, uint8_t *g, uint8_t *b) {
    t = t<0?0:t>1?1:t;
    /* rough viridis fit */
    double rv = 0.267 + 0.004*t + 1.106*t*t - 0.400*t*t*t;
    double gv = 0.005 + 1.698*t - 1.680*t*t + 0.860*t*t*t;
    double bv = 0.329 + 1.748*t - 4.440*t*t + 3.440*t*t*t;
    #define CLAMP01(x) ((x)<0?0:(x)>1?1:(x))
    *r=(uint8_t)(CLAMP01(rv)*255);
    *g=(uint8_t)(CLAMP01(gv)*255);
    *b=(uint8_t)(CLAMP01(bv)*255);
    #undef CLAMP01
}

/* ═══════════════════════════════════════════════════════════
   UTILITY
   ═══════════════════════════════════════════════════════════ */

void clg_term_size(int *cols, int *rows) {
    /* ncurses getmaxyx works on Linux, macOS, and Termux — no ioctl needed */
    *cols = 80; *rows = 24;
    if (stdscr) {
        getmaxyx(stdscr, *rows, *cols);
    } else {
        WINDOW *w = initscr();
        if (w) { getmaxyx(stdscr, *rows, *cols); endwin(); }
    }
    if (*cols < 10) *cols = 80;
    if (*rows < 4)  *rows = 24;
}

void clg_sleep_ms(int ms) {
    struct timespec ts = { ms/1000, (long)(ms%1000)*1000000L };
    nanosleep(&ts, NULL);
}

double clg_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec*1000.0 + ts.tv_nsec/1e6;
}

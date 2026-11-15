// acceltest.c -- c4's port: Duke's accelerators (c4/accel.mjs) against the
// loops they stand for, each as draw.c and fixedPoint_math.c have it: the
// same pixels, the same values left behind, on made-up textures, palettes,
// steps and shifts. test-duke3d runs it on c4's libjs.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "c4accel.h"

int32_t accel_draw = -1;
int32_t accel_base(void) { return accel_draw = accel_find("duke3d"); }

typedef union { unsigned int i; float f; } f2i;

unsigned seed = 12345;
int rnd(int n) { seed = seed * 1103515245 + 12345; return (int)((seed >> 8) % (unsigned)n); }

unsigned char *tex, *pal, *bufc, *bufa;
int fails, cases;
enum { TEXN = 65536, PALN = 65536 + 512, BUFN = 320 * 240 };

void fill(unsigned char *p, int n, int holes) {
    int i;
    for (i = 0; i < n; i++) { p[i] = rnd(256); if (holes && rnd(8) == 0) p[i] = 255; }
}
void check(char *what, int k, int same) {
    cases++;
    if (memcmp(bufc, bufa, BUFN) != 0 || !same) {
        fails++;
        if (fails < 10) printf("accel: %s, case %d, differs\n", what, k);
    }
}
void clear() { memset(bufc, 7, BUFN); memset(bufa, 7, BUFN); }

// Duke's vlineasm1 and mvlineasm1, a column
unsigned colc(int n, unsigned v, unsigned inc, int sh, unsigned char *t, unsigned char *p, unsigned char *d, int bpl, int masked) {
    int c;
    for (; n > 0; n--) {
        c = t[v >> sh];
        if (!masked || c != 255) *d = p[c];
        v += inc; d += bpl;
    }
    return v;
}
void test_tcol() {
    int k, n, sh, masked, x;
    unsigned v, inc, va, vc;
    for (k = 0; k < 200; k++) {
        masked = k & 1;
        clear();
        n = rnd(240); sh = 16 + rnd(16); v = (unsigned)rnd(1 << 30) * 4 + rnd(4); inc = (unsigned)rnd(1 << 30) * 2;
        x = rnd(320);
        vc = colc(n, v, inc, sh, tex, pal, bufc + x, 320, masked);
        if (masked) va = duke_tcol_masked(n, v, inc, sh, tex, pal, bufa + x, 320);
        else va = duke_tcol(n, v, inc, sh, tex, pal, bufa + x, 320);
        check(masked ? "tcol_masked" : "tcol", k, va == vc);
    }
}

// Duke's hlineasm4: a row right to left
void rowc(int n, unsigned a, unsigned b, unsigned da, unsigned db, int sh1, int bits, int sh2, unsigned char *t, unsigned char *p, unsigned char *d) {
    for (; n > 0; n--) { *d = p[t[((a >> sh1) << bits) | (b >> sh2)]]; d--; a -= da; b -= db; }
}
void test_hline() {
    int k, n, bits, sh1, sh2, x;
    unsigned a, b, da, db;
    for (k = 0; k < 200; k++) {
        clear();
        bits = 4 + rnd(5); sh1 = 32 - (16 - bits) + rnd(2); sh2 = (32 - bits) & 31;
        if (sh1 > 31) sh1 = 31;
        a = (unsigned)rnd(1 << 30) * 4 + rnd(4); b = (unsigned)rnd(1 << 30) * 4 + rnd(4);
        da = (unsigned)rnd(1 << 28); db = (unsigned)rnd(1 << 28);
        n = rnd(320); x = 320 * 100 + 319;
        rowc(n, a, b, da, db, sh1, bits, sh2, tex, pal, bufc + x);
        duke_hline(n, a, b, da, db, sh1, bits, sh2, tex, pal, bufa + x);
        check("hline", k, 1);
    }
}

// Duke's rhlineasm4, its x and y with their sign bits flipped
void rrowc(int n, unsigned char *t, unsigned char *p, unsigned char *d, int c, int s, int x, int y, int da, int db, int ax, int bx) {
    for (; n > 0; n--) {
        *d = p[*t]; t -= c;
        if (y < bx) t--; if (x < ax) t -= s; x -= da; y -= db; d--;
    }
}
void test_rhline() {
    int k, n, c, s, x, y, da, db, ax, bx, at;
    for (k = 0; k < 200; k++) {
        clear();
        n = rnd(320); c = rnd(64); s = rnd(64);
        x = rnd(1 << 30) * 4; y = rnd(1 << 30) * 4; da = rnd(1 << 30); db = rnd(1 << 30);
        ax = rnd(1 << 30) * 4; bx = rnd(1 << 30) * 4;
        at = 320 * 50 + 319;
        rrowc(n, tex + TEXN - 1, pal, bufc + at, c, s, x, y, da, db, ax, bx);
        duke_rhline(n, tex + TEXN - 1, pal, bufa + at, c, s, x, y, da, db, ax, bx);
        check("rhline", k, 1);
    }
}

// Duke's slopevlin from its first eight on, as draw.c has it
int recip[2048];
int *pals;
int fpu_c, asm4_c, fpu_a, asm4_a;
void slopec(unsigned char *dest, unsigned z, int *ps, int left, unsigned esi, unsigned edi, int sh2, int sh1, int mask, int bpl,
            unsigned char *t, float a, float step, int gx3, int gy3) {
    f2i c;
    int u, sgn, k, n, bx, vy, pix;
    unsigned r, xs, yhi, ystep;
    unsigned char *p;
    do {
        c.f = a;
        u = (int)c.i;
        fpu_c = u;
        sgn = u >> 31;
        k = (((u >> 23) & 255) - 2) & 31;
        r = ((unsigned)recip[(u >> 12) & 0x7ff] >> k) ^ (unsigned)sgn;
        xs = (unsigned)gx3 * (r - z);
        ystep = (unsigned)gy3 * (r - z);
        z = r;
        a += step;
        asm4_c = left;
        n = left >= 8 ? 8 : (left & 0xff);
        xs &= ~0xffu;
        yhi = ystep & ~0xffu;
        while (n) {
            bx = ((int)esi >> sh2) & mask;
            esi += xs | (unsigned)n;
            vy = (int)(edi >> sh1);
            edi += ystep;
            p = (unsigned char *)*ps;
            ps--;
            pix = p[(vy & ~0xff) | t[bx + vy]];
            ystep = yhi | (unsigned)pix;
            dest += bpl;
            *dest = (unsigned char)pix;
            n--;
        }
        left = asm4_c - 8;
    } while (left > 0);
}
void test_slope() {
    int k, i, left, gx3, gy3, sh1, sh2, mask;
    unsigned z, esi, edi;
    float a, step;
    f2i fa, fs;
    int d;
    for (i = 0; i < 2048; i++) recip[i] = rnd(1 << 30) * 2 + rnd(2);
    for (i = 0; i < 256; i++) pals[i] = (int)(pal + rnd(256) * 256);
    for (k = 0; k < 200; k++) {
        clear();
        left = 1 + rnd(200); sh1 = 24 + rnd(8); sh2 = 20 + rnd(12); mask = (1 << (4 + rnd(8))) - 1;
        z = (unsigned)rnd(1 << 30) * 4; esi = (unsigned)rnd(1 << 30) * 4; edi = (unsigned)rnd(1 << 30) * 4;
        gx3 = rnd(1 << 30) - (1 << 29); gy3 = rnd(1 << 30) - (1 << 29);
        a = (float)(rnd(100000) + 1) * 0.37f; step = (float)(rnd(20000) - 10000) * 0.011f;
        if (k & 1) a = -a;
        d = 320 * 230 + rnd(320);
        slopec(bufc + d, z, pals + 255, left, esi, edi, sh2, sh1, mask, -320, tex, a, step, gx3, gy3);
        fa.f = a; fs.f = step;
        duke_slope(bufa + d, z, pals + 255, left, esi, edi, sh2, sh1, mask, -320, tex, fa.i, fs.i, gx3, gy3, recip, &fpu_a, &asm4_a);
        check("slope", k, fpu_a == fpu_c && asm4_a == asm4_c);
    }
}

// fixedPoint_math.c's helpers and buffer loops
void test_math() {
    int k, a, b, s, r, x;
    unsigned hi, lo, q;
    int64_t p;
    uint64_t n;
    for (k = 0; k < 400; k++) {
        a = rnd(1 << 30) * 4 + rnd(4); b = rnd(1 << 30) * 4 + rnd(4); s = 1 + rnd(31);
        if (k & 1) { a >>= rnd(31); b >>= rnd(31); }
        p = (int64_t)a * b;
        r = duke_mulscale(a, b, s);
        cases++;
        if (r != (int)(p >> s)) { fails++; if (fails < 10) printf("accel: mulscale %d %d %d: %d, not %d
", a, b, s, r, (int)(p >> s)); }
        hi = (unsigned)rnd(1 << 30) * 4 + rnd(4); lo = (unsigned)rnd(1 << 30) * 4 + rnd(4); x = 1 + rnd((1 << (30 - (k & 15))) - 1);
        if (k & 2) hi = hi >> rnd(32);
        n = ((uint64_t)hi << 32) | lo;
        q = duke_udiv64(hi, lo, x);
        cases++;
        if (q != (unsigned)(n / (uint64_t)x)) { fails++; if (fails < 10) printf("accel: udiv64 %u %u %d: %u
", hi, lo, x, q); }
    }
}
void test_buffers() {
    int k, n, val, add, x, i, c, a, y;
    for (k = 0; k < 200; k++) {
        clear();
        n = rnd(320); val = rnd(1 << 30) * 4; add = rnd(1 << 28) - (1 << 27); x = 2 * rnd(320 * 100);
        for (i = 0; i < n; i++) ((short *)(bufc + x))[i] = (short)((val + i * add) >> 16);
        duke_qinterp16(bufa + x, n, val, add);
        check("qinterp16", k, 1);
        clear();
        c = 1 + rnd(4000); a = rnd(1 << 30) * 4 + rnd(4); x = rnd(320 * 200);
        for (i = 0; i < c; i++) bufc[x + i] = (a >> (8 * (i & 3))) & 0xff;
        duke_clearb(bufa + x, c, a);
        check("clearb", k, 1);
        clear();
        c = 1 + rnd(4000); x = rnd(32000); y = 40000 + rnd(32000);
        memcpy(bufc + y, tex + x, c);
        duke_copyb(tex + x, bufa + y, c);
        check("copyb", k, 1);
    }
}

// draw.c's translucent loops, as Chocolate Duke3D has them: pal stands in
// for transluc, tex for the textures and palettes
unsigned tvc(int inc, unsigned char *p, int n, unsigned v, unsigned char *t, unsigned char *d, int sh, int bpl, int rev) {
    unsigned short ci;
    while (n) {
        unsigned tt = t[v >> sh];
        if (tt != 255) {
            ci = p[tt]; ci |= (*d) << 8;
            if (rev) ci = (ci >> 8) | (ci << 8);
            *d = pal[ci];
        }
        v += inc; d += bpl; n--;
    }
    return v;
}
unsigned asm1c, asm2c, asm1a, asm2a;
int bothc;
void tv2c(unsigned ebp, unsigned inca, unsigned char *bufa, unsigned char *bufb, unsigned i5, unsigned i6, unsigned incb,
          unsigned edi, int sh, unsigned char *pa, unsigned char *pb, int bpl, int rev) {
    unsigned a, b;
    unsigned short l, r;
    i6 -= edi;
    do {
        a = bufa[i5 >> sh]; b = bufb[ebp >> sh];
        i5 += inca; ebp += incb;
        if (a == 255) {
            if (b != 255) { r = pb[b] | (((unsigned char *)i6)[edi + 1] << 8); if (rev) r = (r >> 8) | (r << 8); ((unsigned char *)i6)[edi + 1] = pal[r]; }
        } else if (b == 255) {
            l = pa[a] | (((unsigned char *)i6)[edi] << 8); if (rev) l = (l >> 8) | (l << 8); ((unsigned char *)i6)[edi] = pal[l];
        } else {
            l = ((unsigned char *)i6)[edi] << 8; r = ((unsigned char *)i6)[edi + 1] << 8;
            l |= pa[a]; r |= pb[b];
            if (rev) { l = (l >> 8) | (l << 8); r = (r >> 8) | (r << 8); }
            ((unsigned char *)i6)[edi] = pal[l]; ((unsigned char *)i6)[edi + 1] = pal[r];
            bothc++;
        }
        i6 += bpl;
    } while (i6 > i6 - bpl);
    asm1c = i5; asm2c = ebp;
}
unsigned adderc, addera;
void tsc(int i2, int n, unsigned i4, unsigned char *t, unsigned char *d, unsigned char *p, unsigned eax1, unsigned eax3, unsigned ecx, int bpl, int rev) {
    unsigned char ci;
    unsigned short val;
    while (n) {
        n--;
        if (n != 0) {
            i4 += ecx;
            if (i4 < (i4 - ecx)) adderc = eax3;
            ci = *t;
            i2 += eax1;
            if (i2 < (i2 - eax1)) t++;
            t += adderc;
            if (ci != 255) { val = p[ci]; val |= (*d) << 8; if (rev) val = (val >> 8) | (val << 8); *d = pal[val]; }
            d += bpl;
        }
    }
}
void thc(unsigned i2, unsigned i3, int i5, unsigned char *d, unsigned char *t, unsigned char *p, int inc2, int inc5, int al, int bl, int rev) {
    unsigned ebx;
    int counter = (i3 >> 16), i1;
    unsigned short val;
    while (counter >= 0) {
        ebx = i2 >> al;
        ebx = (((unsigned)i5) >> (32 - bl)) | (ebx << bl);
        i1 = t[ebx];
        if ((i1 & 0xff) != 0xff) { val = p[i1]; val |= (*d) << 8; if (rev) val = (val >> 8) | (val << 8); *d = pal[val]; }
        i2 += inc2; i5 += inc5; d++; counter--;
    }
}
void test_trans() {
    int k, n, sh, x, rows, bpl, both, rev, al, bl, i2, eax1, ecx, eax3, inc2, inc5;
    unsigned v, inc, va, vc, ebp, i5, inca, incb, edi, i4;
    for (k = 0; k < 200; k++) {
        rev = k & 1;
        clear();
        for (x = 0; x < BUFN; x++) bufa[x] = bufc[x] = rnd(256);
        n = rnd(240); sh = 16 + rnd(16); v = (unsigned)rnd(1 << 30) * 4 + rnd(4); inc = (unsigned)rnd(1 << 30) * 2;
        x = rnd(320);
        vc = tvc(inc, tex + 40000, n, v, tex, bufc + x, sh, 320, rev);
        va = duke_tvline1(inc, tex + 40000, n, v, tex, bufa + x, sh, 320, rev, pal);
        check("tvline1", k, va == vc);

        rows = 1 + rnd(230); bpl = 320; x = rnd(318); sh = 17 + rnd(15);
        ebp = (unsigned)rnd(1 << 30) * 4; i5 = (unsigned)rnd(1 << 30) * 4; inca = rnd(1 << 30); incb = rnd(1 << 30);
        edi = (unsigned)(bufc + x + rows * bpl); bothc = 0;
        tv2c(ebp, inca, tex, tex + 1000, i5, edi - rows * bpl, incb, edi, sh, tex + 40000, tex + 50000, bpl, rev);
        edi = (unsigned)(bufa + x + rows * bpl);
        both = duke_tvline2(ebp, inca, tex, tex + 1000, i5, edi - rows * bpl, incb, edi, sh, tex + 40000, tex + 50000, bpl, rev, pal, &asm1a, &asm2a);
        check("tvline2", k, both == bothc && asm1a == asm1c && asm2a == asm2c);

        n = rnd(230); i2 = rnd(1 << 30) * 4; i4 = (unsigned)rnd(1 << 30) * 4; eax1 = rnd(1 << 30) * 4; ecx = rnd(1 << 30) * 4;
        eax3 = rnd(64); adderc = rnd(64); addera = adderc; x = rnd(320);
        tsc(i2, n, i4, tex, bufc + x, tex + 40000, eax1, eax3, ecx, 320, rev);
        duke_tsprite(i2, n, i4, tex, bufa + x, tex + 40000, eax1, eax3, ecx, &addera, 320, rev, pal);
        check("tsprite", k, addera == adderc);

        al = 24 + rnd(8); bl = 1 + rnd(7); x = 320 * rnd(200);
        i2 = rnd(1 << 30) * 4; i5 = rnd(1 << 30) * 4; inc2 = rnd(1 << 24); inc5 = rnd(1 << 24);
        n = rnd(318);
        thc(i2, (unsigned)n << 16, i5, bufc + x, tex, tex + 40000, inc2, inc5, al, bl, rev);
        duke_thline(i2, (unsigned)n << 16, i5, bufa + x, tex, tex + 40000, inc2, inc5, al, bl, rev, pal);
        check("thline", k, 1);
    }
}

// fixedPoint_math.h's scale and dmulscale, in int64_t as its fallbacks have them
int pick(int k) {
    switch (rnd(6)) {
    case 0: return rnd(46341) - rnd(46341);
    case 1: return rnd(1 << 30) * 4 + rnd(4);
    case 2: return (rnd(1 << 30) * 4 + rnd(4)) >> rnd(31);
    case 3: return k & 1 ? (int)0x80000000 : 0x7fffffff;
    case 4: return rnd(3) - 1;
    default: return rnd(1 << 16) - (1 << 15);
    }
}
void test_scale() {
    int k, a, b, c, d, s, r, w;
    for (k = 0; k < 600; k++) {
        a = pick(k); b = pick(k + 1); c = pick(k + 2);
        if (c == 0) c = 1 + rnd(1000);
        r = duke_scale(a, b, c);
        w = (int)((int64_t)a * b / c);
        cases++;
        if (r != w) { fails++; if (fails < 10) printf("accel: scale %d %d %d: %d, not %d\n", a, b, c, r, w); }
        d = pick(k + 3); s = 1 + rnd(31);
        r = duke_dmulscale(a, b, c, d, s);
        w = (int)(((int64_t)a * b + (int64_t)c * d) >> s);
        cases++;
        if (r != w) { fails++; if (fails < 10) printf("accel: dmulscale %d %d %d %d %d: %d, not %d\n", a, b, c, d, s, r, w); }
    }
}

int main() {
    if (DUKE_ACCEL() <= 0) { printf("accel: Duke's are not here
"); return 0; }
    tex = malloc(TEXN); pal = malloc(PALN); bufc = malloc(BUFN); bufa = malloc(BUFN);
    pals = malloc(256 * sizeof(int));
    fill(tex, TEXN, 1); fill(pal, PALN, 0);
    test_tcol(); test_hline(); test_rhline(); test_slope(); test_math(); test_buffers(); test_trans(); test_scale();
    if (fails) printf("accel: %d of %d cases differ
", fails, cases);
    else printf("accel: Duke's, %d cases, each as its loop has it
", cases);
    return fails != 0;
}

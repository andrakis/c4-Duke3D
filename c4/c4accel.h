/* c4accel.h -- c4's port: Duke's accelerators, c4/accel.mjs, from the
   game's side (c4's libjs/guest/accel.h, docs/accel.md). On a machine that
   has them, with Video Setup's ACCELERATED on, the host runs these in the
   game's own JavaScript, one instruction each; elsewhere, and in gcc's
   build, the C runs as before. accel_draw is the first of Duke's numbers
   (0 for none, -1 until asked: draw.c); DUKE_ACCEL() asks once. The
   numbers are the module's operations in its order. */
#ifndef C4ACCEL_H
#define C4ACCEL_H

#include "accel.h"

enum {
	DA_TCOL, DA_TCOLM, DA_HLINE, DA_RHLINE, DA_SLOPE,
	DA_MULSCALE, DA_UDIV64, DA_QINTERP16, DA_CLEARB, DA_COPYB,
	DA_TVLINE1, DA_TVLINE2, DA_TSPRITE, DA_THLINE,
	DA_SCALE, DA_DMULSCALE, DA_COPYR, DA_MHLINE
};

extern int32_t accel_draw;
int32_t accel_base(void);
#define DUKE_ACCEL() (accel_draw >= 0 ? accel_draw : accel_base())

#ifdef __c4_inttypes__
#define DA_OP(op) (accel_draw + (op))
#define duke_tcol(n, v, inc, sh, tex, pal, dest, stride) \
	__c4_opcode((n), (v), (inc), (sh), (tex), (pal), (dest), (stride), DA_OP(DA_TCOL))
#define duke_tcol_masked(n, v, inc, sh, tex, pal, dest, stride) \
	__c4_opcode((n), (v), (inc), (sh), (tex), (pal), (dest), (stride), DA_OP(DA_TCOLM))
#define duke_hline(n, a, b, da, db, sh1, bits, sh2, tex, pal, dest) \
	__c4_opcode((n), (a), (b), (da), (db), (sh1), (bits), (sh2), (tex), (pal), (dest), DA_OP(DA_HLINE))
#define duke_rhline(n, tex, pal, p, c, s, x, y, da, db, ax, bx) \
	__c4_opcode((n), (tex), (pal), (p), (c), (s), (x), (y), (da), (db), (ax), (bx), DA_OP(DA_RHLINE))
#define duke_slope(dest, z, pals, left, esi, edi, sh2, sh1, mask, bpl, tex, abits, stepbits, gx3, gy3, recip, pfpuasm, pasm4) \
	__c4_opcode((dest), (z), (pals), (left), (esi), (edi), (sh2), (sh1), (mask), (bpl), (tex), (abits), \
		(stepbits), (gx3), (gy3), (recip), (pfpuasm), (pasm4), DA_OP(DA_SLOPE))
#define duke_mulscale(a, b, s) __c4_opcode((a), (b), (s), DA_OP(DA_MULSCALE))
#define duke_udiv64(hi, lo, b) __c4_opcode((hi), (lo), (b), DA_OP(DA_UDIV64))
#define duke_qinterp16(buf, num, val, add) __c4_opcode((buf), (num), (val), (add), DA_OP(DA_QINTERP16))
#define duke_clearb(d, c, a) __c4_opcode((d), (c), (a), DA_OP(DA_CLEARB))
#define duke_copyb(s, d, c) __c4_opcode((s), (d), (c), DA_OP(DA_COPYB))
/* copybufreverse: c bytes from s down, to d up (a mirror's rows) */
#define duke_copyr(s, d, c) __c4_opcode((s), (d), (c), DA_OP(DA_COPYR))
/* mhlineskipmodify: a masked floor's or ceiling's row, left to right */
#define duke_mhline(n, a, b, dest, sh1, bits, tex, pal, da, db) \
	__c4_opcode((n), (a), (b), (dest), (sh1), (bits), (tex), (pal), (da), (db), DA_OP(DA_MHLINE))
/* the translucent loops: tvlineasm1, tvlineasm2, DrawSpriteVerticalLine,
   thlineskipmodify (draw.c), with the globals they read as arguments */
#define duke_tvline1(inc, pal, n, v, tex, dest, sh, bpl, rev, tl) \
	__c4_opcode((inc), (pal), (n), (v), (tex), (dest), (sh), (bpl), (rev), (tl), DA_OP(DA_TVLINE1))
#define duke_tvline2(ebp, inca, bufa, bufb, i5, i6, incb, edi, sh, pala, palb, bpl, rev, tl, pasm1, pasm2) \
	__c4_opcode((ebp), (inca), (bufa), (bufb), (i5), (i6), (incb), (edi), (sh), (pala), (palb), (bpl), (rev), (tl), \
		(pasm1), (pasm2), DA_OP(DA_TVLINE2))
#define duke_tsprite(i2, n, i4, tex, dest, pal, eax1, eax3, ecx, padder, bpl, rev, tl) \
	__c4_opcode((i2), (n), (i4), (tex), (dest), (pal), (eax1), (eax3), (ecx), (padder), (bpl), (rev), (tl), DA_OP(DA_TSPRITE))
#define duke_thline(i2, i3, i5, i6, tex, pal, inc2, inc5, al, bl, rev, tl) \
	__c4_opcode((i2), (i3), (i5), (i6), (tex), (pal), (inc2), (inc5), (al), (bl), (rev), (tl), DA_OP(DA_THLINE))
/* fixedPoint_math.h's: (int32_t)((int64_t)a*b/c) for c not 0, and
   (int32_t)(((int64_t)a*b + (int64_t)c*d) >> s) for 0 < s < 32 */
#define duke_scale(a, b, c) __c4_opcode((a), (b), (c), DA_OP(DA_SCALE))
#define duke_dmulscale(a, b, c, d, s) __c4_opcode((a), (b), (c), (d), (s), DA_OP(DA_DMULSCALE))
#else
#define duke_tcol(n, v, inc, sh, tex, pal, dest, stride) 0
#define duke_tcol_masked(n, v, inc, sh, tex, pal, dest, stride) 0
#define duke_hline(n, a, b, da, db, sh1, bits, sh2, tex, pal, dest) 0
#define duke_rhline(n, tex, pal, p, c, s, x, y, da, db, ax, bx) 0
#define duke_slope(dest, z, pals, left, esi, edi, sh2, sh1, mask, bpl, tex, abits, stepbits, gx3, gy3, recip, pfpuasm, pasm4) 0
#define duke_mulscale(a, b, s) 0
#define duke_udiv64(hi, lo, b) 0
#define duke_qinterp16(buf, num, val, add) 0
#define duke_clearb(d, c, a) 0
#define duke_copyb(s, d, c) 0
#define duke_copyr(s, d, c) 0
#define duke_mhline(n, a, b, dest, sh1, bits, tex, pal, da, db) 0
#define duke_tvline1(inc, pal, n, v, tex, dest, sh, bpl, rev, tl) 0
#define duke_tvline2(ebp, inca, bufa, bufb, i5, i6, incb, edi, sh, pala, palb, bpl, rev, tl, pasm1, pasm2) 0
#define duke_tsprite(i2, n, i4, tex, dest, pal, eax1, eax3, ecx, padder, bpl, rev, tl) 0
#define duke_thline(i2, i3, i5, i6, tex, pal, inc2, inc5, al, bl, rev, tl) 0
#define duke_scale(a, b, c) 0
#define duke_dmulscale(a, b, c, d, s) 0
#endif

#endif

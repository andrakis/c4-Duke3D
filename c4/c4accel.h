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
	DA_MULSCALE, DA_UDIV64, DA_QINTERP16, DA_CLEARB, DA_COPYB
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
#endif

#endif

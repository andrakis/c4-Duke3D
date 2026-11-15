// accel.mjs -- Duke Nukem 3D's accelerators for c4's libjs (its libjs/accel.js,
// docs/accel.md): the engine's innermost loops and its 64-bit helpers, run by
// the host when the machine has them and Video Setup's ACCELERATED is on.
// c4/c4accel.h is the game's side, with each operation's number (DA_*);
// Engine/src/draw.c and fixedPoint_math.c call them, and keep their own C for
// every other machine. Each does exactly what that C does: the same pixels in
// the same order, the same values left behind, 32-bit arithmetic wrapping,
// shifts modulo 32 as c4's SHL and LSR take them, a single's sum as
// Math.fround of the double's.
export default function duke3d({ reader, writer, reader32, writer32, args, fval, fbits }) {
  // DA_TCOL, DA_TCOLM (count, v, inc, sh, tex, pal, dest, stride): a wall's
  // column, vlineasm1/4, *dest = pal[tex[v >>> sh]]; v += inc; dest +=
  // stride; gives v. The masked one (mvlineasm1/4) leaves texel 255 out.
  function tcol(m, masked) {
    let [n, v, inc, sh, tex, pal, dest, stride] = args(m, 8);
    sh &= 31;
    const rt = reader(m), rp = reader(m), wr = writer(m);
    for (; n > 0; n--, v = (v + inc) | 0, dest = (dest + stride) | 0) {
      const c = rt((tex + (v >>> sh)) | 0);
      if (!masked || c !== 255) wr(dest, rp((pal + c) | 0));
    }
    return v;
  }

  // DA_HLINE (count, a, b, da, db, sh1, bits, sh2, tex, pal, dest): a floor's
  // row right to left, hlineasm4: *dest-- = pal[tex[((a >>> sh1) << bits) |
  // (b >>> sh2)]]; a -= da; b -= db
  function hline(m) {
    let [n, a, b, da, db, sh1, bits, sh2, tex, pal, dest] = args(m, 11);
    const rt = reader(m), rp = reader(m), wr = writer(m);
    for (; n > 0; n--, dest = (dest - 1) | 0, a = (a - da) | 0, b = (b - db) | 0)
      wr(dest, rp((pal + rt((tex + (((a >>> sh1) << bits) | (b >>> sh2))) | 0)) | 0));
    return 0;
  }

  // DA_RHLINE (n, tex, pal, p, c, s, x, y, da, db, ax, bx): the tilted
  // screen's row, rhlineasm4, x and y with their sign bits flipped as draw.c
  // keeps them: *p-- = pal[*tex]; tex -= c; if (y < bx) tex--; if (x < ax)
  // tex -= s; x -= da; y -= db
  function rhline(m) {
    let [n, tex, pal, p, c, s, x, y, da, db, ax, bx] = args(m, 12);
    const rt = reader(m), rp = reader(m), wr = writer(m);
    for (; n > 0; n--) {
      wr(p, rp((pal + rt(tex)) | 0));
      tex = (tex - c) | 0;
      if (y < bx) tex = (tex - 1) | 0;
      if (x < ax) tex = (tex - s) | 0;
      x = (x - da) | 0;
      y = (y - db) | 0;
      p = (p - 1) | 0;
    }
    return 0;
  }

  // DA_SLOPE (dest, z, pals, left, esi, edi, sh2, sh1, mask, bpl, tex, abits,
  // stepbits, gx3, gy3, recip, pfpuasm, pasm4): slopevlin from its first
  // eight on, 1/z from a float's bits every eight, the floats as their bits;
  // leaves fpuasm and asm4 as the loop does
  function slope(m) {
    let [dest, z, pals, left, esi, edi, sh2, sh1, mask, bpl, tex, abits, stepbits, gx3, gy3, recip, pfpu, pasm4] = args(m, 18);
    sh1 &= 31; sh2 &= 31;
    const step = fval(stepbits);
    let a = fval(abits);
    const rt = reader(m), rp = reader(m), wr = writer(m), r32 = reader32(m), w32 = writer32(m);
    let u = 0, asm4 = 0;
    do {
      u = fbits(a);
      const sgn = u >> 31;
      const k = (((u >> 23) & 255) - 2) & 31;
      const r = (r32((recip + 4 * ((u >> 12) & 0x7ff)) | 0) >>> k) ^ sgn;
      const d = (r - z) | 0;
      const xs = Math.imul(gx3, d) & ~0xff;
      let ystep = Math.imul(gy3, d);
      const yhi = ystep & ~0xff;
      z = r;
      a = Math.fround(a + step);
      asm4 = left;
      for (let n = left >= 8 ? 8 : (left & 0xff); n !== 0; n--) {
        const bx = (esi >> sh2) & mask;
        esi = (esi + (xs | n)) | 0;
        const vy = (edi >>> sh1) | 0;
        edi = (edi + ystep) | 0;
        const pal = r32(pals);
        pals = (pals - 4) | 0;
        const pix = rp((pal + ((vy & ~0xff) | rt((tex + bx + vy) | 0))) | 0);
        ystep = yhi | pix;
        dest = (dest + bpl) | 0;
        wr(dest, pix);
      }
      left = (asm4 - 8) | 0;
    } while (left > 0);
    w32(pfpu, u);
    w32(pasm4, asm4);
    return 0;
  }

  // DA_MULSCALE (a, b, s), 0 < s < 32: (int32_t)(((int64_t)a * b) >> s), the
  // product in 16-bit digits as fixedPoint_math.c's c4_mulscale has it
  function mulscale(m) {
    const [a, b, s] = args(m, 3);
    const ah = a >> 16, al = a & 0xffff, bh = b >> 16, bl = b & 0xffff;
    const m1 = ah * bl, m2 = al * bh;
    let lo = (al * bl) >>> 0;
    let hi = (ah * bh + (m1 >> 16) + (m2 >> 16)) | 0;
    const t = (lo + ((m1 << 16) >>> 0)) >>> 0;
    if (t < lo) hi = (hi + 1) | 0;
    lo = (t + ((m2 << 16) >>> 0)) >>> 0;
    if (lo < t) hi = (hi + 1) | 0;
    return (lo >>> s) | (hi << (32 - s));
  }

  // DA_UDIV64 (hi, lo, b), 0 < b < 2^30: hi:lo / b, the quotient's low 32
  // bits, as c4_udiv64: the remainder of hi, then lo's two halves, each a
  // quotient below 2^16 of a number below 2^46, exact in a double
  const qdiv = (x, b) => { let q = Math.floor(x / b); if (q * b > x) q--; else if ((q + 1) * b <= x) q++; return q; };
  function udiv64(m) {
    const [hi, lo, b] = args(m, 3);
    let r = (hi >>> 0) % b;
    let x = r * 65536 + (lo >>> 16);
    const q1 = qdiv(x, b);
    r = x - q1 * b;
    x = r * 65536 + (lo & 0xffff);
    const q0 = qdiv(x, b);
    return ((q1 << 16) | q0) | 0;
  }

  // DA_QINTERP16 (buf, num, val, add): qinterpolatedown16short, num shorts
  // of val >> 16, val stepping by add
  function qinterp16(m) {
    let [p, n, val, add] = args(m, 4);
    const wr = writer(m);
    for (; n > 0; n--, p = (p + 2) | 0, val = (val + add) | 0) {
      const h = val >> 16;
      wr(p, h & 255);
      wr((p + 1) | 0, (h >> 8) & 255);
    }
    return 0;
  }

  // DA_CLEARB (dest, c, a): clearbufbyte, c bytes of a's four in turn from
  // its lowest
  function clearb(m) {
    let [p, c, a] = args(m, 3);
    const wr = writer(m);
    for (let z = 0; c > 0; c--, p = (p + 1) | 0, z = (z + 1) & 3) wr(p, (a >>> (8 * z)) & 255);
    return 0;
  }

  // DA_COPYB (src, dest, c): copybufbyte where the two do not overlap (the
  // game asks only then: run again after a fault, it reads what it read)
  function copyb(m) {
    let [s, d, c] = args(m, 3);
    const rs = reader(m), wr = writer(m);
    for (; c > 0; c--, s = (s + 1) | 0, d = (d + 1) | 0) wr(d, rs(s));
    return 0;
  }

  return [m => tcol(m, false), m => tcol(m, true), hline, rhline, slope, mulscale, udiv64, qinterp16, clearb, copyb];
}

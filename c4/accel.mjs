// accel.mjs -- Duke Nukem 3D's accelerators for c4's libjs (its libjs/accel.js,
// docs/accel.md): the engine's innermost loops and its 64-bit helpers, run by
// the host when the machine has them and Video Setup's ACCELERATED is on.
// c4/c4accel.h is the game's side, with each operation's number (DA_*);
// Engine/src/draw.c and fixedPoint_math.c call them, and keep their own C for
// every other machine. Each does exactly what that C does: the same pixels in
// the same order, the same values left behind, 32-bit arithmetic wrapping,
// shifts modulo 32 as c4's SHL and LSR take them, a single's sum as
// Math.fround of the double's.
export default function duke3d({ reader, writer, reader32, writer32, probe, span, args, fval, fbits }) {
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
  // (int64_t)a * b as its two words, hi signed and lo unsigned: the 16-bit
  // digits, the high ones signed, so that any a and b take, INT32_MIN too
  const mul64 = (a, b) => {
    const ah = a >> 16, al = a & 0xffff, bh = b >> 16, bl = b & 0xffff;
    const m1 = ah * bl, m2 = al * bh;
    let lo = (al * bl) >>> 0;
    let hi = (ah * bh + (m1 >> 16) + (m2 >> 16)) | 0;
    const t = (lo + ((m1 << 16) >>> 0)) >>> 0;
    if (t < lo) hi = (hi + 1) | 0;
    lo = (t + ((m2 << 16) >>> 0)) >>> 0;
    if (lo < t) hi = (hi + 1) | 0;
    return [hi, lo];
  };
  function mulscale(m) {
    const [a, b, s] = args(m, 3);
    const [hi, lo] = mul64(a, b);
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
  // a page at a time (span), as one move where both sides are plain bytes
  function copyb(m) {
    let [s, d, c] = args(m, 3);
    while (c > 0) {
      const n = Math.min(c, 4096 - (s & 4095), 4096 - (d & 4095));
      const a = span(m, s, n, false), b = span(m, d, n, true);
      if (a && b) b.set(a);
      else {
        const rs = reader(m), wr = writer(m);
        for (let k = 0; k < n; k++) wr((d + k) | 0, rs((s + k) | 0));
      }
      s = (s + n) | 0; d = (d + n) | 0; c -= n;
    }
    return 0;
  }

  // DA_COPYR (src, dest, c): copybufreverse, c bytes read down from src and
  // written up from dest -- a mirror's rows, turned round. The game asks
  // only where the two do not overlap, so it too may run again after a fault
  function copyr(m) {
    let [s, d, c] = args(m, 3);
    while (c > 0) {
      const n = Math.min(c, (s & 4095) + 1, 4096 - (d & 4095));
      const a = span(m, (s - n + 1) | 0, n, false), b = span(m, d, n, true);
      if (a && b) for (let k = 0, j = n - 1; k < n; k++, j--) b[k] = a[j];
      else {
        const rs = reader(m), wr = writer(m);
        for (let k = 0; k < n; k++) wr((d + k) | 0, rs((s - k) | 0));
      }
      s = (s - n) | 0; d = (d + n) | 0; c -= n;
    }
    return 0;
  }


  // The translucent loops blend what is on the screen: drawn twice, a pixel
  // would be blended twice. So they work out every pixel first, reading
  // everything they read and making sure (probe) that every page they will
  // write is there, and only then write: a fault comes before the first
  // write, and the instruction runs again as if it had not begun.
  function later(m) {
    const list = [];
    let page = -1;
    return {
      put(a, b) {
        if ((a >>> 12) !== page) { probe(m, a, 1, true); page = a >>> 12; }
        list.push(a, b);
      },
      commit() {
        const wr = writer(m);
        for (let i = 0; i < list.length; i += 2) wr(list[i], list[i + 1]);
      },
    };
  }
  // a blend's index, the new colour over the old, swapped when transrev
  const blend = (c, old, rev) => {
    let v = (c | (old << 8)) & 0xffff;
    if (rev) v = ((v >> 8) | (v << 8)) & 0xffff;
    return v;
  };

  // DA_TVLINE1 (inc, pal, n, v, tex, dest, sh, bpl, transrev, transluc):
  // tvlineasm1, a translucent wall's column of n pixels; gives v
  function tvline1(m) {
    let [inc, pal, n, v, tex, dest, sh, bpl, rev, tl] = args(m, 10);
    sh &= 31;
    const rt = reader(m), rp = reader(m), rd = reader(m), rl = reader(m), out = later(m);
    for (; n !== 0; n = (n - 1) | 0, v = (v + inc) | 0, dest = (dest + bpl) | 0) {
      const t = rt((tex + (v >>> sh)) | 0);
      if (t !== 255) out.put(dest, rl((tl + blend(rp((pal + t) | 0), rd(dest), rev)) | 0));
    }
    out.commit();
    return v;
  }

  // DA_TVLINE2 (ebp, inca, bufa, bufb, i5, i6, incb, edi, sh, pala, palb,
  // bpl, transrev, transluc, pasm1, pasm2): tvlineasm2, two translucent
  // columns side by side at edi + i6, i6 counting up from below zero to
  // its end as the original did; asm1 and asm2 left as it leaves them, and
  // gives how many rows drew both (pixelsAllowed's count)
  function tvline2(m) {
    let [ebp, inca, bufa, bufb, i5, i6, incb, edi, sh, pala, palb, bpl, rev, tl, pasm1, pasm2] = args(m, 16);
    sh &= 31;
    const ra = reader(m), rb = reader(m), rp = reader(m), rd = reader(m), rl = reader(m), w32 = writer32(m), out = later(m);
    let both = 0;
    i6 = (i6 - edi) | 0;
    do {
      const a = ra((bufa + (i5 >>> sh)) | 0), b = rb((bufb + (ebp >>> sh)) | 0);
      i5 = (i5 + inca) | 0;
      ebp = (ebp + incb) | 0;
      const p0 = (i6 + edi) | 0, p1 = (p0 + 1) | 0;
      if (a === 255) {
        if (b !== 255) out.put(p1, rl((tl + blend(rp((palb + b) | 0), rd(p1), rev)) | 0));
      } else if (b === 255) {
        out.put(p0, rl((tl + blend(rp((pala + a) | 0), rd(p0), rev)) | 0));
      } else {
        const l = blend(rp((pala + a) | 0), rd(p0), rev), r = blend(rp((palb + b) | 0), rd(p1), rev);
        out.put(p0, rl((tl + l) | 0));
        out.put(p1, rl((tl + r) | 0));
        both++;
      }
      i6 = (i6 + bpl) | 0;
    } while ((i6 >>> 0) > (((i6 - bpl) | 0) >>> 0));
    probe(m, pasm1, 4, true);
    probe(m, pasm2, 4, true);
    out.commit();
    w32(pasm1, i5);
    w32(pasm2, ebp);
    return both;
  }

  // DA_TSPRITE (i2, n, i4, tex, dest, pal, eax1, eax3, ecx, padder, bpl,
  // transrev, transluc): DrawSpriteVerticalLine, a translucent sprite's
  // column, n - 1 pixels; adder left as it leaves it
  function tsprite(m) {
    let [i2, n, i4, tex, dest, pal, eax1, eax3, ecx, padder, bpl, rev, tl] = args(m, 13);
    const r32 = reader32(m), w32 = writer32(m), rt = reader(m), rp = reader(m), rd = reader(m), rl = reader(m), out = later(m);
    let adder = r32(padder);
    while (n !== 0) {
      n = (n - 1) | 0;
      if (n !== 0) {
        i4 = (i4 + ecx) | 0;
        if ((i4 >>> 0) < (((i4 - ecx) | 0) >>> 0)) adder = eax3;
        const c = rt(tex);
        i2 = (i2 + eax1) | 0;
        if ((i2 >>> 0) < (((i2 - eax1) | 0) >>> 0)) tex = (tex + 1) | 0;
        tex = (tex + adder) | 0;
        if (c !== 255) out.put(dest, rl((tl + blend(rp((pal + c) | 0), rd(dest), rev)) | 0));
        dest = (dest + bpl) | 0;
      }
    }
    probe(m, padder, 4, true);
    out.commit();
    w32(padder, adder);
    return 0;
  }

  // DA_THLINE (i2, i3, i5, i6, tex, pal, inc2, inc5, al, bl, transrev,
  // transluc): thlineskipmodify, a translucent floor sprite's row, left to
  // right, (i3 >> 16) + 1 pixels
  function thline(m) {
    let [i2, i3, i5, i6, tex, pal, inc2, inc5, al, bl, rev, tl] = args(m, 12);
    const rt = reader(m), rp = reader(m), rd = reader(m), rl = reader(m), out = later(m);
    for (let counter = i3 >>> 16; counter >= 0; counter--, i2 = (i2 + inc2) | 0, i5 = (i5 + inc5) | 0, i6 = (i6 + 1) | 0) {
      const ebx = ((i5 >>> (32 - bl)) | ((i2 >>> al) << bl)) >>> 0;
      const t = rt((tex + ebx) | 0);
      if (t !== 255) out.put(i6, rl((tl + blend(rp((pal + t) | 0), rd(i6), rev)) | 0));
    }
    out.commit();
    return 0;
  }

  // DA_SCALE (a, b, c), c not 0: (int32_t)((int64_t)a * b / c), the
  // quotient toward zero and its low word, for any a, b and c: the
  // product's magnitude in four 16-bit digits, divided by |c| a digit at a
  // time (each step a number below 2^47 over one below 2^31 + 1, exact in a
  // double), and the sign put back. (c 0: the game's C, fixedPoint_math.h.)
  function scale(m) {
    const [a, b, c] = args(m, 3);
    const ua = Math.abs(a), ub = Math.abs(b), uc = Math.abs(c);
    const a1 = Math.floor(ua / 65536), a0 = ua % 65536, b1 = Math.floor(ub / 65536), b0 = ub % 65536;
    const lo = a0 * b0, mid = a0 * b1 + a1 * b0, hi = a1 * b1;
    let k = Math.floor(lo / 65536) + mid;
    const d0 = lo % 65536, d1 = k % 65536;
    k = Math.floor(k / 65536) + hi;
    const d2 = k % 65536, d3 = Math.floor(k / 65536);
    let r = 0, q = 0;
    for (const d of [d3, d2, d1, d0]) {
      const x = r * 65536 + d;
      const qd = qdiv(x, uc);
      r = x - qd * uc;
      q = ((q << 16) | qd) | 0;           // the quotient's low word
    }
    return (a < 0) !== (b < 0) !== (c < 0) ? (0 - q) | 0 : q;
  }

  // DA_DMULSCALE (a, b, c, d, s), 0 < s < 32: (int32_t)(((int64_t)a * b +
  // (int64_t)c * d) >> s), the sum in 64 bits wrapping as the C's does
  function dmulscale(m) {
    const [a, b, c, d, s] = args(m, 5);
    const [h1, l1] = mul64(a, b), [h2, l2] = mul64(c, d);
    const lo = (l1 + l2) >>> 0;
    const hi = (h1 + h2 + (lo < l1 ? 1 : 0)) | 0;
    return (lo >>> s) | (hi << (32 - s));
  }

  // DA_MHLINE (n, a, b, dest, sh1, bits, tex, pal, da, db): a masked floor's
  // or ceiling's row left to right, mhlineskipmodify: c = tex[((a >>> sh1)
  // << bits) | (b >>> (32 - bits))], drawn as pal[c] unless 255; a += da;
  // b += db. The shift by 32 - bits is x86's and gcc's, taken mod 32 (shld)
  function mhline(m) {
    let [n, a, b, dest, sh1, bits, tex, pal, da, db] = args(m, 10);
    const rt = reader(m), rp = reader(m), wr = writer(m), sh2 = (32 - bits) & 31;
    sh1 &= 31; bits &= 31;
    for (; n > 0; n--, dest = (dest + 1) | 0, a = (a + da) | 0, b = (b + db) | 0) {
      const c = rt((tex + (((a >>> sh1) << bits) | (b >>> sh2))) | 0);
      if (c !== 255) wr(dest, rp((pal + c) | 0));
    }
    return 0;
  }

  return [m => tcol(m, false), m => tcol(m, true), hline, rhline, slope, mulscale, udiv64, qinterp16, clearb, copyb,
          tvline1, tvline2, tsprite, thline, scale, dmulscale, copyr, mhline];
}

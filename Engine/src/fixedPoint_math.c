// converted from asm to c by Jonof

#include <stdio.h>
#include "platform.h"
#include "fixedPoint_math.h"

void clearbuf(void *d, int32_t c, int32_t a)
{
	int32_t *p = (int32_t*)d;
	while ((c--) > 0) *(p++) = a;
}

#ifdef C4DUKE
/* c4: the same bytes, a's four in turn from its lowest, without two
   table lookups and a shift by a variable for every byte */
void clearbufbyte(void *D, int32_t c, int32_t a)
{
	uint8_t  *p = (uint8_t *)D;
	uint8_t b[4];
	int32_t z = 0;
	b[0] = a & 0xff; b[1] = (a >> 8) & 0xff; b[2] = (a >> 16) & 0xff; b[3] = (a >> 24) & 0xff;
	while ((c--) > 0) {
		*(p++) = b[z];
		z = (z + 1) & 3;
	}
}
#else
void clearbufbyte(void *D, int32_t c, int32_t a)
{ // Cringe City
	uint8_t  *p = (uint8_t *)D;
	int32_t m[4] = { 0xffl,0xff00l,0xff0000l,0xff000000l };
	int32_t n[4] = { 0,8,16,24 };
	int32_t z=0;
	while ((c--) > 0) {
		*(p++) = (uint8_t )((a & m[z])>>n[z]);
		z=(z+1)&3;
	}
}
#endif

void copybuf(void *s, void *d, int32_t c)
{
	int32_t *p = (int32_t*)s, *q = (int32_t*)d;
	while ((c--) > 0) *(q++) = *(p++);
}

void copybufbyte(void *S, void *D, int32_t c)
{
	uint8_t  *p = (uint8_t *)S, *q = (uint8_t *)D;
	while((c--) > 0) *(q++) = *(p++);
}

void copybufreverse(void *S, void *D, int32_t c)
{
	uint8_t  *p = (uint8_t *)S, *q = (uint8_t *)D;
	while((c--) > 0) *(q++) = *(p--);
}

void qinterpolatedown16(int32_t* bufptr, int32_t num, int32_t val, int32_t add)
{ // gee, I wonder who could have provided this...
    int32_t i, *lptr = bufptr;
    for(i=0;i<num;i++) { lptr[i] = (val>>16); val += add; }
}

void qinterpolatedown16short(int32_t* bufptr, int32_t num, int32_t val, int32_t add)
{ // ...maybe the same person who provided this too?
    int32_t i; short *sptr = (short *)bufptr;
    for(i=0;i<num;i++) { sptr[i] = (short)(val>>16); val += add; }
}


#ifdef C4DUKE
/* c4's port: 64-bit products and quotients from 32-bit operations.

   c4's machine has no 64-bit arithmetic: c4lc makes each operation on an
   int64_t a call, and a division a loop of them, a bit at a time, about
   6,000 instructions. These give exactly the values the int64_t code does
   (fixedPoint_math.h uses them when the shorter ways there do not apply)
   with c4's own 32-bit operations. c4 has no logical shift either (c4lc
   makes an unsigned >> a call too), so there are none here: an arithmetic
   shift and a mask instead (C4_LSR). */

/* x >> s, logically, for 0 < s < 32 */
#define C4_LSR(x, s) ((uint32_t)(((int32_t)(x) >> (s)) & (0x7fffffff >> ((s) - 1))))

/* how far b (0 < b < 2^30) shifts left and stays below 2^31 */
static int c4_room (int32_t b)
{
	int k = 0;
	if (b < 0x8000) { b <<= 16; k += 16; }
	if (b < 0x800000) { b <<= 8; k += 8; }
	if (b < 0x8000000) { b <<= 4; k += 4; }
	if (b < 0x20000000) { b <<= 2; k += 2; }
	if (b < 0x40000000) { b <<= 1; k += 1; }
	return k;
}

/* hi:lo / b for 0 < b < 2^30: the quotient's low 32 bits. Long division,
   as many bits at a time as the remainder has room for beside b. */
uint32_t c4_udiv64 (uint32_t hi, uint32_t lo, int32_t b)
{
	int k = c4_room (b), n = 64, take;
	int32_t r = 0;
	uint32_t q = 0;
	if (hi == 0) { hi = lo; lo = 0; n = 32; }
	while (n > 0)
	{
		take = n < k ? n : k;
		r = (r << take) | (int32_t)C4_LSR (hi, 32 - take);
		hi = (hi << take) | C4_LSR (lo, 32 - take);
		lo <<= take;
		q = (q << take) | (uint32_t)(r / b);
		r = r % b;
		n -= take;
	}
	return q;
}

/* |a| * |b|, a and b > INT32_MIN, as hi:lo: 16-bit digits */
static void c4_umul64 (int32_t a, int32_t b, uint32_t *hi, uint32_t *lo)
{
	int32_t a0, a1, b0, b1, p01, p10, p11, t1, t2, t3;
	uint32_t p00;
	if (a < 0) a = -a;
	if (b < 0) b = -b;
	a0 = a & 0xffff; a1 = a >> 16;
	b0 = b & 0xffff; b1 = b >> 16;
	p00 = (uint32_t)a0 * (uint32_t)b0;	/* below 2^32 */
	p01 = a0 * b1;				/* the rest below 2^31 */
	p10 = a1 * b0;
	p11 = a1 * b1;
	t1 = (int32_t)C4_LSR (p00, 16) + (p01 & 0xffff) + (p10 & 0xffff);
	t2 = (t1 >> 16) + (p01 >> 16) + (p10 >> 16) + (p11 & 0xffff);
	t3 = (t2 >> 16) + (p11 >> 16);
	*lo = (p00 & 0xffff) | ((uint32_t)t1 << 16);
	*hi = ((uint32_t)t2 & 0xffff) | ((uint32_t)t3 << 16);
}

/* (int32_t)(((int64_t)a << s) / d), for 0 <= s <= 32; 0 for "not here",
   with *done 0 */
int32_t c4_divscale (int32_t a, int32_t d, int32_t s, int *done)
{
	uint32_t m, hi, lo, q;
	*done = 0;
	if (d == 0 || d >= 0x40000000 || d <= -0x40000000 || a == INT32_MIN || s < 0 || s > 32)
		return 0;
	m = a < 0 ? (uint32_t)-a : (uint32_t)a;
	if (s == 0) { hi = 0; lo = m; }
	else if (s == 32) { hi = m; lo = 0; }
	else { hi = C4_LSR (m, 32 - s); lo = m << s; }
	q = c4_udiv64 (hi, lo, d < 0 ? -d : d);
	*done = 1;
	return (a < 0) != (d < 0) ? (int32_t)(0u - q) : (int32_t)q;
}

/* (int32_t)((int64_t)a * b / c) */
int32_t c4_scale (int32_t a, int32_t b, int32_t c, int *done)
{
	uint32_t hi, lo, q;
	*done = 0;
	if (c == 0 || c >= 0x40000000 || c <= -0x40000000 || a == INT32_MIN || b == INT32_MIN)
		return 0;
	c4_umul64 (a, b, &hi, &lo);
	q = c4_udiv64 (hi, lo, c < 0 ? -c : c);
	*done = 1;
	return ((a < 0) != (b < 0)) != (c < 0) ? (int32_t)(0u - q) : (int32_t)q;
}

/* (int32_t)(((int64_t)a * b) >> s), 0 < s < 32 */
int32_t c4_mulscale (int32_t a, int32_t b, int32_t s, int *done)
{
	uint32_t hi, lo;
	*done = 0;
	if (a == INT32_MIN || b == INT32_MIN || s <= 0 || s >= 32)
		return 0;
	c4_umul64 (a, b, &hi, &lo);
	if ((a < 0) != (b < 0))
	{
		/* the product's two's complement, 64 bits */
		hi = ~hi + (lo == 0);
		lo = 0u - lo;
	}
	*done = 1;
	return (int32_t)(C4_LSR (lo, s) | (hi << (32 - s)));
}
#endif

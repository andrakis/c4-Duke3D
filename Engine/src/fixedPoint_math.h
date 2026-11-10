/*
 * "Build Engine & Tools" Copyright (c) 1993-1997 Ken Silverman
 * Ken Silverman's official web site: "http://www.advsys.net/ken"
 * See the included license file "BUILDLIC.TXT" for license info.
 * This file has been modified from Ken Silverman's original release
 */

#ifndef __PRAGMAS_H__
#define __PRAGMAS_H__

#include "platform.h" 

static __inline void swapchar(uint8_t  *p1, uint8_t  *p2)
{ uint8_t  tmp = *p1; *p1 = *p2; *p2 = tmp; }
static __inline void swapshort(short *p1, short *p2)
{ short tmp = *p1; *p1 = *p2; *p2 = tmp; }
static __inline void swaplong(int32_t *p1, int32_t *p2)
{ int32_t tmp = *p1; *p1 = *p2; *p2 = tmp; }
static __inline void swapchar2(uint8_t  *p1, uint8_t  *p2, int xsiz)
{
    swapchar(p1, p2);
    swapchar(p1 + 1, p2 + xsiz);
}

static __inline int32_t msqrtasm(uint32_t input)
{
	uint32_t a,b;

	a = 0x40000000l;		// mov eax, 0x40000000
	b = 0x20000000l;		// mov ebx, 0x20000000
    
	do {				// begit:
		if (input >= a) {		// cmp ecx, eax	 /  jl skip
			input -= a;		// sub ecx, eax
			a += b*4;	// lea eax, [eax+ebx*4]
		}			// skip:
		a -= b;			// sub eax, ebx
		a >>= 1;		// shr eax, 1
		b >>= 2;		// shr ebx, 2
	}
    while (b);			// jnz begit
    
	if (input >= a)			// cmp ecx, eax
		a++;			// sbb eax, -1
    
	a >>= 1;			// shr eax, 1
    
	return a;
}

void vlin16first (int32_t i1, int32_t i2);

static inline int32_t sqr (int32_t input1) { return input1*input1; }

/* internal use:32x32 = 64bit */
static inline int64_t mul32_64(int32_t i1,int32_t i2)
{
	return (int64_t)i1*i2;
}
#ifdef C4DUKE
/* c4: c4's machine has no 64-bit arithmetic, and every operation on an
   int64_t is a call (a division a long one). Where the product or the
   shifted numerator fits in 32 bits, these work in 32, which gives the
   same value; otherwise as before. */
#define C4_WITHIN(a, n) ((a) >= -(n) && (a) <= (n))
/* fixedPoint_math.c: the int64_t forms' values from 32-bit operations,
   *done 0 where they cannot (operands at the ends of the range) */
int32_t c4_divscale(int32_t a, int32_t d, int32_t s, int *done);
int32_t c4_scale(int32_t a, int32_t b, int32_t c, int *done);
int32_t c4_mulscale(int32_t a, int32_t b, int32_t s, int *done);
static inline int scale (int32_t input1, int32_t input2, int32_t input3)
{
	int done;
	int32_t r;
	if (C4_WITHIN(input1, 46340) && C4_WITHIN(input2, 46340) && input3 != -1)
		return (input1*input2)/input3;
	r = c4_scale(input1, input2, input3, &done);
	if (done)
		return r;
	return (int)(mul32_64(input1,input2)/(int64_t)input3);
}
static inline int mulscale (int32_t input1, int32_t input2, int32_t input3)
{
	int done;
	int32_t r;
	if (C4_WITHIN(input1, 46340) && C4_WITHIN(input2, 46340) && input3 < 32)
		return (input1*input2)>>input3;
	r = c4_mulscale(input1, input2, input3, &done);
	if (done)
		return r;
	return (int)(mul32_64(input1,input2)>>input3);
}
static inline int dmulscale  (int32_t input1, int32_t input2, int32_t input3,int32_t input4,int32_t input5)
{
	if (C4_WITHIN(input1, 32767) && C4_WITHIN(input2, 32767) && C4_WITHIN(input3, 32767) &&
	    C4_WITHIN(input4, 32767) && input5 < 32)
		return (input1*input2 + input3*input4)>>input5;
	return (int)((mul32_64(input1,input2) + mul32_64(input3,input4))>>input5);
}
static inline int tmulscale(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6,int32_t shift)
{
	if (C4_WITHIN(i1, 16383) && C4_WITHIN(i2, 16383) && C4_WITHIN(i3, 16383) && C4_WITHIN(i4, 16383) &&
	    C4_WITHIN(i5, 16383) && C4_WITHIN(i6, 16383) && shift < 32)
		return (i1*i2 + i3*i4 + i5*i6)>>shift;
	return (int)((mul32_64(i1,i2) + mul32_64(i3,i4) + mul32_64(i5,i6))>>shift);
}
static inline int32_t divscale(int32_t i1, int32_t i2, int32_t i3)
{
	int done;
	int32_t r;
	if (i3 < 31 && C4_WITHIN(i1, 0x7fffffff >> i3) && i2 != -1)
		return (int32_t)((uint32_t)i1 << i3) / i2;
	r = c4_divscale(i1, i2, i3, &done);
	if (done)
		return r;
	return (int32_t)(((int64_t)i1<<i3)/i2);
}
#else
static inline int scale (int32_t input1, int32_t input2, int32_t input3)
{
	return (int)(mul32_64(input1,input2)/(int64_t)input3);
}
static inline int mulscale (int32_t input1, int32_t input2, int32_t input3)
{
	return (int)(mul32_64(input1,input2)>>input3);
}
static inline int dmulscale  (int32_t input1, int32_t input2, int32_t input3,int32_t input4,int32_t input5)
{
	return (int)((mul32_64(input1,input2) + mul32_64(input3,input4))>>input5);
}
static inline int tmulscale(int32_t i1, int32_t i2, int32_t i3, int32_t i4, int32_t i5, int32_t i6,int32_t shift)
{
	return (int)((mul32_64(i1,i2) + mul32_64(i3,i4) + mul32_64(i5,i6))>>shift);
}
static inline int32_t divscale(int32_t i1, int32_t i2, int32_t i3)
{
	return (int32_t)(((int64_t)i1<<i3)/i2);
}
#endif

#define DEFFUNCS \
DEFFUN(1)\
DEFFUN(2)\
DEFFUN(3)\
DEFFUN(4)\
DEFFUN(5)\
DEFFUN(6)\
DEFFUN(7)\
DEFFUN(8)\
DEFFUN(9)\
DEFFUN(10)\
DEFFUN(11)\
DEFFUN(12)\
DEFFUN(13)\
DEFFUN(14)\
DEFFUN(15)\
DEFFUN(16)\
DEFFUN(17)\
DEFFUN(18)\
DEFFUN(19)\
DEFFUN(20)\
DEFFUN(21)\
DEFFUN(22)\
DEFFUN(23)\
DEFFUN(24)\
DEFFUN(25)\
DEFFUN(26)\
DEFFUN(27)\
DEFFUN(28)\
DEFFUN(29)\
DEFFUN(30)\
DEFFUN(31)\
DEFFUN(32)

#define DEFFUN(N) \
static __inline int32_t mulscale##N(int32_t input1, int32_t input2) \
{ return mulscale(input1,input2,N); }
DEFFUNCS
#undef DEFFUN

#define DEFFUN(N) \
static __inline int32_t dmulscale##N(int32_t input1, int32_t input2,int32_t input3,int32_t input4) \
{ return dmulscale(input1,input2,input3,input4,N); }
DEFFUNCS
#undef DEFFUN

#define DEFFUN(N) \
static __inline int32_t tmulscale##N(int32_t i1, int32_t i2,int32_t i3,int32_t i4,int32_t i5,int32_t i6) \
{ return tmulscale(i1,i2,i3,i4,i5,i6,N); }
DEFFUNCS
#undef DEFFUN

#define DEFFUN(N) \
static __inline int32_t divscale##N(int32_t input1, int32_t input2) \
{ return divscale(input1,input2,N); }
DEFFUNCS
#undef DEFFUN

static inline int ksgn(int32_t i1)
{
  if (i1 < 0) return -1;
  else if (i1 > 0) return 1;
  else return 0;
}

static inline int sgn(int32_t i1) { return ksgn(i1); }
static inline int klabs (int32_t i1)
{
  if (i1 < 0) i1 = -i1;
  return i1;
}
static inline int mul3 (int32_t i1) { return i1*3; }
static inline int mul5 (int32_t i1) { return i1*5; }
static inline int mul9 (int32_t i1) { return i1*9; }

void copybufreverse(void *S, void *D, int32_t c);
void copybuf(void *s, void *d, int32_t c);
void clearbuf(void *d, int32_t c, int32_t a);
void clearbufbyte(void *D, int32_t c, int32_t a);
void copybufbyte(void *S, void *D, int32_t c);

void qinterpolatedown16 (int32_t* bufptr, int32_t num, int32_t val, int32_t add);
void qinterpolatedown16short (int32_t* bufptr, int32_t num, int32_t val, int32_t add);

#endif /* !defined _INCLUDE_PRAGMAS_H_ */



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

#ifdef C4DUKE
/* c4: c4lc calls a static inline function as it calls any other, so that
   mulscale16(a,b) was two calls, mulscale16's and then mulscale's. As
   macros they are the one call: each uses each of its arguments once, as
   the function it names does, so nothing is worked out twice and an
   argument with a side effect has it once. */
#define mulscale1(a,b) mulscale(a,b,1)
#define mulscale2(a,b) mulscale(a,b,2)
#define mulscale3(a,b) mulscale(a,b,3)
#define mulscale4(a,b) mulscale(a,b,4)
#define mulscale5(a,b) mulscale(a,b,5)
#define mulscale6(a,b) mulscale(a,b,6)
#define mulscale7(a,b) mulscale(a,b,7)
#define mulscale8(a,b) mulscale(a,b,8)
#define mulscale9(a,b) mulscale(a,b,9)
#define mulscale10(a,b) mulscale(a,b,10)
#define mulscale11(a,b) mulscale(a,b,11)
#define mulscale12(a,b) mulscale(a,b,12)
#define mulscale13(a,b) mulscale(a,b,13)
#define mulscale14(a,b) mulscale(a,b,14)
#define mulscale15(a,b) mulscale(a,b,15)
#define mulscale16(a,b) mulscale(a,b,16)
#define mulscale17(a,b) mulscale(a,b,17)
#define mulscale18(a,b) mulscale(a,b,18)
#define mulscale19(a,b) mulscale(a,b,19)
#define mulscale20(a,b) mulscale(a,b,20)
#define mulscale21(a,b) mulscale(a,b,21)
#define mulscale22(a,b) mulscale(a,b,22)
#define mulscale23(a,b) mulscale(a,b,23)
#define mulscale24(a,b) mulscale(a,b,24)
#define mulscale25(a,b) mulscale(a,b,25)
#define mulscale26(a,b) mulscale(a,b,26)
#define mulscale27(a,b) mulscale(a,b,27)
#define mulscale28(a,b) mulscale(a,b,28)
#define mulscale29(a,b) mulscale(a,b,29)
#define mulscale30(a,b) mulscale(a,b,30)
#define mulscale31(a,b) mulscale(a,b,31)
#define mulscale32(a,b) mulscale(a,b,32)
#define dmulscale1(a,b,c,d) dmulscale(a,b,c,d,1)
#define dmulscale2(a,b,c,d) dmulscale(a,b,c,d,2)
#define dmulscale3(a,b,c,d) dmulscale(a,b,c,d,3)
#define dmulscale4(a,b,c,d) dmulscale(a,b,c,d,4)
#define dmulscale5(a,b,c,d) dmulscale(a,b,c,d,5)
#define dmulscale6(a,b,c,d) dmulscale(a,b,c,d,6)
#define dmulscale7(a,b,c,d) dmulscale(a,b,c,d,7)
#define dmulscale8(a,b,c,d) dmulscale(a,b,c,d,8)
#define dmulscale9(a,b,c,d) dmulscale(a,b,c,d,9)
#define dmulscale10(a,b,c,d) dmulscale(a,b,c,d,10)
#define dmulscale11(a,b,c,d) dmulscale(a,b,c,d,11)
#define dmulscale12(a,b,c,d) dmulscale(a,b,c,d,12)
#define dmulscale13(a,b,c,d) dmulscale(a,b,c,d,13)
#define dmulscale14(a,b,c,d) dmulscale(a,b,c,d,14)
#define dmulscale15(a,b,c,d) dmulscale(a,b,c,d,15)
#define dmulscale16(a,b,c,d) dmulscale(a,b,c,d,16)
#define dmulscale17(a,b,c,d) dmulscale(a,b,c,d,17)
#define dmulscale18(a,b,c,d) dmulscale(a,b,c,d,18)
#define dmulscale19(a,b,c,d) dmulscale(a,b,c,d,19)
#define dmulscale20(a,b,c,d) dmulscale(a,b,c,d,20)
#define dmulscale21(a,b,c,d) dmulscale(a,b,c,d,21)
#define dmulscale22(a,b,c,d) dmulscale(a,b,c,d,22)
#define dmulscale23(a,b,c,d) dmulscale(a,b,c,d,23)
#define dmulscale24(a,b,c,d) dmulscale(a,b,c,d,24)
#define dmulscale25(a,b,c,d) dmulscale(a,b,c,d,25)
#define dmulscale26(a,b,c,d) dmulscale(a,b,c,d,26)
#define dmulscale27(a,b,c,d) dmulscale(a,b,c,d,27)
#define dmulscale28(a,b,c,d) dmulscale(a,b,c,d,28)
#define dmulscale29(a,b,c,d) dmulscale(a,b,c,d,29)
#define dmulscale30(a,b,c,d) dmulscale(a,b,c,d,30)
#define dmulscale31(a,b,c,d) dmulscale(a,b,c,d,31)
#define dmulscale32(a,b,c,d) dmulscale(a,b,c,d,32)
#define tmulscale1(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,1)
#define tmulscale2(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,2)
#define tmulscale3(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,3)
#define tmulscale4(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,4)
#define tmulscale5(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,5)
#define tmulscale6(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,6)
#define tmulscale7(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,7)
#define tmulscale8(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,8)
#define tmulscale9(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,9)
#define tmulscale10(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,10)
#define tmulscale11(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,11)
#define tmulscale12(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,12)
#define tmulscale13(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,13)
#define tmulscale14(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,14)
#define tmulscale15(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,15)
#define tmulscale16(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,16)
#define tmulscale17(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,17)
#define tmulscale18(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,18)
#define tmulscale19(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,19)
#define tmulscale20(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,20)
#define tmulscale21(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,21)
#define tmulscale22(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,22)
#define tmulscale23(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,23)
#define tmulscale24(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,24)
#define tmulscale25(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,25)
#define tmulscale26(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,26)
#define tmulscale27(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,27)
#define tmulscale28(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,28)
#define tmulscale29(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,29)
#define tmulscale30(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,30)
#define tmulscale31(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,31)
#define tmulscale32(a,b,c,d,e,f) tmulscale(a,b,c,d,e,f,32)
#define divscale1(a,b) divscale(a,b,1)
#define divscale2(a,b) divscale(a,b,2)
#define divscale3(a,b) divscale(a,b,3)
#define divscale4(a,b) divscale(a,b,4)
#define divscale5(a,b) divscale(a,b,5)
#define divscale6(a,b) divscale(a,b,6)
#define divscale7(a,b) divscale(a,b,7)
#define divscale8(a,b) divscale(a,b,8)
#define divscale9(a,b) divscale(a,b,9)
#define divscale10(a,b) divscale(a,b,10)
#define divscale11(a,b) divscale(a,b,11)
#define divscale12(a,b) divscale(a,b,12)
#define divscale13(a,b) divscale(a,b,13)
#define divscale14(a,b) divscale(a,b,14)
#define divscale15(a,b) divscale(a,b,15)
#define divscale16(a,b) divscale(a,b,16)
#define divscale17(a,b) divscale(a,b,17)
#define divscale18(a,b) divscale(a,b,18)
#define divscale19(a,b) divscale(a,b,19)
#define divscale20(a,b) divscale(a,b,20)
#define divscale21(a,b) divscale(a,b,21)
#define divscale22(a,b) divscale(a,b,22)
#define divscale23(a,b) divscale(a,b,23)
#define divscale24(a,b) divscale(a,b,24)
#define divscale25(a,b) divscale(a,b,25)
#define divscale26(a,b) divscale(a,b,26)
#define divscale27(a,b) divscale(a,b,27)
#define divscale28(a,b) divscale(a,b,28)
#define divscale29(a,b) divscale(a,b,29)
#define divscale30(a,b) divscale(a,b,30)
#define divscale31(a,b) divscale(a,b,31)
#define divscale32(a,b) divscale(a,b,32)
#else
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
#endif

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



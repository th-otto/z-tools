/***************************************************************************/
/*                                                                         */
/*  fttrigon.c                                                             */
/*                                                                         */
/*    FreeType trigonometric functions (body).                             */
/*                                                                         */
/*  Copyright 2001-2005, 2012-2013 by                                      */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, FTL.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/

#include "ftmath.h"
#include "utils.h"

#if defined(_MSC_VER)
#include <intrin.h>
static inline int clz(uint32_t x)
{
	unsigned long r = 0;

	if (_BitScanReverse(&r, x))
		return 31 - r;
	return 32;
}


#define PVG_FT_MSB(x)  (31 - clz(x))
#elif defined(__GNUC__)
#define PVG_FT_MSB(x)  (31 - __builtin_clz(x))
#else
static inline int clz(uint32_t x)
{
	int n = 0;

	if (x == 0)
		return 32;
	if (x <= 0x0000FFFFUL)
	{
		n += 16;
		x <<= 16;
	}
	if (x <= 0x00FFFFFFUL)
	{
		n += 8;
		x <<= 8;
	}
	if (x <= 0x0FFFFFFFUL)
	{
		n += 4;
		x <<= 4;
	}
	if (x <= 0x3FFFFFFFUL)
	{
		n += 2;
		x <<= 2;
	}
	if (x <= 0x7FFFFFFFUL)
	{
		n += 1;
	}
	return n;
}

#define PVG_FT_MSB(x)  (31 - clz(x))
#endif

#define PVG_FT_PAD_FLOOR(x, n) ((x) & ~((n)-1))
#define PVG_FT_PAD_ROUND(x, n) PVG_FT_PAD_FLOOR((x) + ((n) / 2), n)
#define PVG_FT_PAD_CEIL(x, n) PVG_FT_PAD_FLOOR((x) + ((n)-1), n)

#define PVG_FT_BEGIN_STMNT do {
#define PVG_FT_END_STMNT } while (0)

/* transfer sign leaving a positive number */
#define PVG_FT_MOVE_SIGN(x, s) \
    PVG_FT_BEGIN_STMNT         \
    if (x < 0) {              \
        x = -x;               \
        s = -s;               \
    }                         \
    PVG_FT_END_STMNT

#ifdef __PUREC__

typedef struct
{
	PVG_FT_UInt32 hi;
	PVG_FT_UInt32 lo;
} PVG_FT_Int64;

static void ft_multo64(PVG_FT_UInt32 x, PVG_FT_UInt32 y, PVG_FT_Int64 *z)
{
	PVG_FT_UInt32 lo1;
	PVG_FT_UInt32 hi1;
	PVG_FT_UInt32 lo2;
	PVG_FT_UInt32 hi2;
	PVG_FT_UInt32 lo;
	PVG_FT_UInt32 hi;
	PVG_FT_UInt32 i1;
	PVG_FT_UInt32 i2;

	lo1 = x & 0x0000FFFFUL;
	hi1 = x >> 16;
	lo2 = y & 0x0000FFFFUL;
	hi2 = y >> 16;

	lo = lo1 * lo2;
	i1 = lo1 * hi2;
	i2 = lo2 * hi1;
	hi = hi1 * hi2;

	/* Check carry overflow of i1 + i2 */
	i1 += i2;
	hi += (PVG_FT_UInt32) (i1 < i2) << 16;

	hi += i1 >> 16;
	i1 = i1 << 16;

	/* Check carry overflow of i1 + lo */
	lo += i1;
	hi += (lo < i1);

	z->lo = lo;
	z->hi = hi;
}


static PVG_FT_UInt32 ft_div64by32(PVG_FT_UInt32 hi, PVG_FT_UInt32 lo, PVG_FT_UInt32 y)
{
	PVG_FT_UInt32 r;
	PVG_FT_UInt32 q;
	int i;

	if (hi >= y)
		return 0x7FFFFFFFL;

	/* We shift as many bits as we can into the high register, perform     */
	/* 32-bit division with modulo there, then work through the remaining  */
	/* bits with long division. This optimization is especially noticeable */
	/* for smaller dividends that barely use the high register.            */

	i = 31 - PVG_FT_MSB(hi);
	r = (hi << i) | (lo >> (32 - i));
	lo <<= i;							/* left 64-bit shift */
	q = r / y;
	r -= q * y;							/* remainder */

	i = 32 - i;							/* bits remaining in low register */
	do
	{
		q <<= 1;
		r = (r << 1) | (lo >> 31);
		lo <<= 1;

		if (r >= y)
		{
			r -= y;
			q |= 1;
		}
	} while (--i);

	return q;
}


static void PVG_FT_Add64(PVG_FT_Int64 *x, PVG_FT_Int64 *y, PVG_FT_Int64 *z)
{
	PVG_FT_UInt32 lo;
	PVG_FT_UInt32 hi;

	lo = x->lo + y->lo;
	hi = x->hi + y->hi + (lo < x->lo);

	z->lo = lo;
	z->hi = hi;
}


PVG_FT_Long PVG_FT_MulFix(PVG_FT_Long a_, PVG_FT_Long b_)
{
	int s = 1;
	PVG_FT_UInt32 a;
	PVG_FT_UInt32 b;

	/* XXX: this function does not allow 64-bit arguments */

	a = a_;
	b = b_;

	PVG_FT_MOVE_SIGN(a_, s);
	PVG_FT_MOVE_SIGN(b_, s);

	if (a + (b >> 8) <= 8190UL)
		a = (a * b + 0x8000UL) >> 16;
	else
	{
		PVG_FT_UInt32 al = a & 0xFFFFUL;

		a = (a >> 16) * b + al * (b >> 16) + ((al * (b & 0xFFFFUL) + 0x8000UL) >> 16);
	}

	a_ = (PVG_FT_Long) a;

	return s < 0 ? -a_ : a_;
}


PVG_FT_Long PVG_FT_MulDiv(PVG_FT_Long a_, PVG_FT_Long b_, PVG_FT_Long c_)
{
	int s = 1;
	PVG_FT_UInt32 a;
	PVG_FT_UInt32 b;
	PVG_FT_UInt32 c;

	/* XXX: this function does not allow 64-bit arguments */

	a = a_;
	b = b_;
	c = c_;

	PVG_FT_MOVE_SIGN(a_, s);
	PVG_FT_MOVE_SIGN(b_, s);
	PVG_FT_MOVE_SIGN(c_, s);

	if (c == 0)
		a = 0x7FFFFFFFUL;

	else if (a + b <= 129894UL - (c >> 17))
		a = (a * b + (c >> 1)) / c;

	else
	{
		PVG_FT_Int64 temp;
		PVG_FT_Int64 temp2;

		ft_multo64(a, b, &temp);

		temp2.hi = 0;
		temp2.lo = c >> 1;

		PVG_FT_Add64(&temp, &temp2, &temp);

		/* last attempt to ditch long division */
		a = (temp.hi == 0) ? temp.lo / c : ft_div64by32(temp.hi, temp.lo, c);
	}

	a_ = (PVG_FT_Long) a;

	return s < 0 ? -a_ : a_;
}


PVG_FT_Long PVG_FT_DivFix(PVG_FT_Long a_, PVG_FT_Long b_)
{
	int s = 1;
	PVG_FT_UInt32 a;
	PVG_FT_UInt32 b;
	PVG_FT_UInt32 q;
	PVG_FT_Long q_;

	/* XXX: this function does not allow 64-bit arguments */

	a = a_;
	b = b_;

	PVG_FT_MOVE_SIGN(a_, s);
	PVG_FT_MOVE_SIGN(b_, s);

	if (b == 0)
	{
		/* check for division by 0 */
		q = 0x7FFFFFFFUL;
	} else if (a <= 65535UL - (b >> 17))
	{
		/* compute result directly */
		q = ((a << 16) + (b >> 1)) / b;
	} else
	{
		/* we need more bits; we have to do it by hand */
		PVG_FT_Int64 temp, temp2;

		temp.hi = a >> 16;
		temp.lo = a << 16;
		temp2.hi = 0;
		temp2.lo = b >> 1;

		PVG_FT_Add64(&temp, &temp2, &temp);
		q = ft_div64by32(temp.hi, temp.lo, b);
	}

	q_ = (PVG_FT_Long) q;

	return s < 0 ? -q_ : q_;
}

#else


PVG_FT_Long PVG_FT_MulFix(PVG_FT_Long a, PVG_FT_Long b)
{
	int s = 1;
	PVG_FT_Long c;

	PVG_FT_MOVE_SIGN(a, s);
	PVG_FT_MOVE_SIGN(b, s);

	c = (PVG_FT_Long) (((PVG_FT_Int64) a * b + 0x8000L) >> 16);

	return (s > 0) ? c : -c;
}

PVG_FT_Long PVG_FT_MulDiv(PVG_FT_Long a, PVG_FT_Long b, PVG_FT_Long c)
{
	int s = 1;
	PVG_FT_Long d;

	PVG_FT_MOVE_SIGN(a, s);
	PVG_FT_MOVE_SIGN(b, s);
	PVG_FT_MOVE_SIGN(c, s);

	d = (PVG_FT_Long) (c > 0 ? ((PVG_FT_Int64) a * b + (c >> 1)) / c : 0x7FFFFFFFL);

	return (s > 0) ? d : -d;
}

PVG_FT_Long PVG_FT_DivFix(PVG_FT_Long a, PVG_FT_Long b)
{
	int s = 1;
	PVG_FT_Long q;

	PVG_FT_MOVE_SIGN(a, s);
	PVG_FT_MOVE_SIGN(b, s);

	q = (PVG_FT_Long) (b > 0 ? (((PVG_FT_UInt64) a << 16) + (b >> 1)) / b : 0x7FFFFFFFL);

	return (s < 0 ? -q : q);
}

#endif

/*************************************************************************/
/*                                                                       */
/* This is a fixed-point CORDIC implementation of trigonometric          */
/* functions as well as transformations between Cartesian and polar      */
/* coordinates.  The angles are represented as 16.16 fixed-point values  */
/* in degrees, i.e., the angular resolution is 2^-16 degrees.  Note that */
/* only vectors longer than 2^16*180/pi (or at least 22 bits) on a       */
/* discrete Cartesian grid can have the same or better angular           */
/* resolution.  Therefore, to maintain this precision, some functions    */
/* require an interim upscaling of the vectors, whereas others operate   */
/* with 24-bit long vectors directly.                                    */
/*                                                                       */
/*************************************************************************/

/* the Cordic shrink factor 0.858785336480436 * 2^32 */
#define PVG_FT_TRIG_SCALE 0xDBD95B16UL

/* the highest bit in overflow-safe vector components, */
/* MSB of 0.858785336480436 * sqrt(0.5) * 2^30         */
#define PVG_FT_TRIG_SAFE_MSB 29

/* this table was generated for PVG_FT_PI = 180L << 16, i.e. degrees */
#define PVG_FT_TRIG_MAX_ITERS 23

static const PVG_FT_Fixed ft_trig_arctan_table[] = {
	1740967L, 919879L, 466945L, 234379L, 117304L, 58666L, 29335L, 14668L,
	7334L, 3667L, 1833L, 917L, 458L, 229L, 115L, 57L,
	29L, 14L, 7L, 4L, 2L, 1L
};

/* multiply a given value by the CORDIC shrink factor */
static PVG_FT_Fixed ft_trig_downscale(PVG_FT_Fixed val)
{
	PVG_FT_Fixed s;
	PVG_FT_Int64 v;

	s = val;
	val = PVG_FT_ABS(val);

#ifdef __PUREC__
	{
		PVG_FT_Int64 temp;
		
		ft_multo64(val, PVG_FT_TRIG_SCALE, &v);
		temp.hi = 1;
		temp.lo = 0;
		PVG_FT_Add64(&v, &temp, &v);
		val = v.hi;
	}
#else
	v = (val * (PVG_FT_Int64) PVG_FT_TRIG_SCALE) + 0x100000000ULL;
	val = (PVG_FT_Fixed) (v >> 32);
#endif

	return (s >= 0) ? val : -val;
}

/* undefined and never called for zero vector */
static PVG_FT_Int ft_trig_prenorm(PVG_FT_Vector *vec)
{
	PVG_FT_Pos x, y;
	PVG_FT_Int shift;

	x = vec->x;
	y = vec->y;

	shift = PVG_FT_MSB(PVG_FT_ABS(x) | PVG_FT_ABS(y));

	if (shift <= PVG_FT_TRIG_SAFE_MSB)
	{
		shift = PVG_FT_TRIG_SAFE_MSB - shift;
		vec->x = (PVG_FT_Pos) ((PVG_FT_ULong) x << shift);
		vec->y = (PVG_FT_Pos) ((PVG_FT_ULong) y << shift);
	} else
	{
		shift -= PVG_FT_TRIG_SAFE_MSB;
		vec->x = x >> shift;
		vec->y = y >> shift;
		shift = -shift;
	}

	return shift;
}

static void ft_trig_pseudo_rotate(PVG_FT_Vector *vec, PVG_FT_Angle theta)
{
	PVG_FT_Int i;
	PVG_FT_Fixed x, y, xtemp, b;
	const PVG_FT_Fixed *arctanptr;

	x = vec->x;
	y = vec->y;

	/* Rotate inside [-PI/4,PI/4] sector */
	while (theta < -PVG_FT_ANGLE_PI4)
	{
		xtemp = y;
		y = -x;
		x = xtemp;
		theta += PVG_FT_ANGLE_PI2;
	}

	while (theta > PVG_FT_ANGLE_PI4)
	{
		xtemp = -y;
		y = x;
		x = xtemp;
		theta -= PVG_FT_ANGLE_PI2;
	}

	arctanptr = ft_trig_arctan_table;

	/* Pseudorotations, with right shifts */
	for (i = 1, b = 1; i < PVG_FT_TRIG_MAX_ITERS; b <<= 1, i++)
	{
		PVG_FT_Fixed v1 = ((y + b) >> i);
		PVG_FT_Fixed v2 = ((x + b) >> i);

		if (theta < 0)
		{
			xtemp = x + v1;
			y = y - v2;
			x = xtemp;
			theta += *arctanptr++;
		} else
		{
			xtemp = x - v1;
			y = y + v2;
			x = xtemp;
			theta -= *arctanptr++;
		}
	}

	vec->x = x;
	vec->y = y;
}

static void ft_trig_pseudo_polarize(PVG_FT_Vector *vec)
{
	PVG_FT_Angle theta;
	PVG_FT_Int i;
	PVG_FT_Fixed x, y, xtemp, b;
	const PVG_FT_Fixed *arctanptr;

	x = vec->x;
	y = vec->y;

	/* Get the vector into [-PI/4,PI/4] sector */
	if (y > x)
	{
		if (y > -x)
		{
			theta = PVG_FT_ANGLE_PI2;
			xtemp = y;
			y = -x;
			x = xtemp;
		} else
		{
			theta = y > 0 ? PVG_FT_ANGLE_PI : -PVG_FT_ANGLE_PI;
			x = -x;
			y = -y;
		}
	} else
	{
		if (y < -x)
		{
			theta = -PVG_FT_ANGLE_PI2;
			xtemp = -y;
			y = x;
			x = xtemp;
		} else
		{
			theta = 0;
		}
	}

	arctanptr = ft_trig_arctan_table;

	/* Pseudorotations, with right shifts */
	for (i = 1, b = 1; i < PVG_FT_TRIG_MAX_ITERS; b <<= 1, i++)
	{
		PVG_FT_Fixed v1 = ((y + b) >> i);
		PVG_FT_Fixed v2 = ((x + b) >> i);

		if (y > 0)
		{
			xtemp = x + v1;
			y = y - v2;
			x = xtemp;
			theta += *arctanptr++;
		} else
		{
			xtemp = x - v1;
			y = y + v2;
			x = xtemp;
			theta -= *arctanptr++;
		}
	}

	/* round theta */
	if (theta >= 0)
		theta = PVG_FT_PAD_ROUND(theta, 32);
	else
		theta = -PVG_FT_PAD_ROUND(-theta, 32);

	vec->x = x;
	vec->y = theta;
}

/* documentation is in fttrigon.h */

PVG_FT_Fixed PVG_FT_Cos(PVG_FT_Angle angle)
{
	PVG_FT_Vector v;

	v.x = PVG_FT_TRIG_SCALE >> 8;
	v.y = 0;
	ft_trig_pseudo_rotate(&v, angle);

	return (v.x + 0x80L) >> 8;
}

/* documentation is in fttrigon.h */

PVG_FT_Fixed PVG_FT_Sin(PVG_FT_Angle angle)
{
	return PVG_FT_Cos(PVG_FT_ANGLE_PI2 - angle);
}

/* documentation is in fttrigon.h */

PVG_FT_Fixed PVG_FT_Tan(PVG_FT_Angle angle)
{
	PVG_FT_Vector v;

	v.x = PVG_FT_TRIG_SCALE >> 8;
	v.y = 0;
	ft_trig_pseudo_rotate(&v, angle);

	return PVG_FT_DivFix(v.y, v.x);
}

/* documentation is in fttrigon.h */

PVG_FT_Angle PVG_FT_Atan2(PVG_FT_Fixed dx, PVG_FT_Fixed dy)
{
	PVG_FT_Vector v;

	if (dx == 0 && dy == 0)
		return 0;

	v.x = dx;
	v.y = dy;
	ft_trig_prenorm(&v);
	ft_trig_pseudo_polarize(&v);

	return v.y;
}

/* documentation is in fttrigon.h */

void PVG_FT_Vector_Unit(PVG_FT_Vector *vec, PVG_FT_Angle angle)
{
	vec->x = PVG_FT_TRIG_SCALE >> 8;
	vec->y = 0;
	ft_trig_pseudo_rotate(vec, angle);
	vec->x = (vec->x + 0x80L) >> 8;
	vec->y = (vec->y + 0x80L) >> 8;
}

void PVG_FT_Vector_Rotate(PVG_FT_Vector *vec, PVG_FT_Angle angle)
{
	PVG_FT_Int shift;
	PVG_FT_Vector v = *vec;

	if (v.x == 0 && v.y == 0)
		return;

	shift = ft_trig_prenorm(&v);
	ft_trig_pseudo_rotate(&v, angle);
	v.x = ft_trig_downscale(v.x);
	v.y = ft_trig_downscale(v.y);

	if (shift > 0)
	{
		PVG_FT_Int32 half = (PVG_FT_Int32) 1L << (shift - 1);


		vec->x = (v.x + half - (v.x < 0)) >> shift;
		vec->y = (v.y + half - (v.y < 0)) >> shift;
	} else
	{
		shift = -shift;
		vec->x = (PVG_FT_Pos) ((PVG_FT_ULong) v.x << shift);
		vec->y = (PVG_FT_Pos) ((PVG_FT_ULong) v.y << shift);
	}
}

/* documentation is in fttrigon.h */

PVG_FT_Fixed PVG_FT_Vector_Length(PVG_FT_Vector *vec)
{
	PVG_FT_Int shift;
	PVG_FT_Vector v;

	v = *vec;

	/* handle trivial cases */
	if (v.x == 0)
	{
		return PVG_FT_ABS(v.y);
	} else if (v.y == 0)
	{
		return PVG_FT_ABS(v.x);
	}

	/* general case */
	shift = ft_trig_prenorm(&v);
	ft_trig_pseudo_polarize(&v);

	v.x = ft_trig_downscale(v.x);

	if (shift > 0)
		return (v.x + (1L << (shift - 1))) >> shift;

	return (PVG_FT_Fixed) ((PVG_FT_UInt32) v.x << -shift);
}

/* documentation is in fttrigon.h */

void PVG_FT_Vector_Polarize(PVG_FT_Vector *vec, PVG_FT_Fixed *length, PVG_FT_Angle *angle)
{
	PVG_FT_Int shift;
	PVG_FT_Vector v;

	v = *vec;

	if (v.x == 0 && v.y == 0)
		return;

	shift = ft_trig_prenorm(&v);
	ft_trig_pseudo_polarize(&v);

	v.x = ft_trig_downscale(v.x);

	*length = (shift >= 0) ? (v.x >> shift) : (PVG_FT_Fixed) ((PVG_FT_UInt32) v.x << -shift);
	*angle = v.y;
}

/* documentation is in fttrigon.h */

void PVG_FT_Vector_From_Polar(PVG_FT_Vector *vec, PVG_FT_Fixed length, PVG_FT_Angle angle)
{
	vec->x = length;
	vec->y = 0;

	PVG_FT_Vector_Rotate(vec, angle);
}

/* documentation is in fttrigon.h */

PVG_FT_Angle PVG_FT_Angle_Diff(PVG_FT_Angle angle1, PVG_FT_Angle angle2)
{
	PVG_FT_Angle delta = angle2 - angle1;

	while (delta <= -PVG_FT_ANGLE_PI)
		delta += PVG_FT_ANGLE_2PI;

	while (delta > PVG_FT_ANGLE_PI)
		delta -= PVG_FT_ANGLE_2PI;

	return delta;
}

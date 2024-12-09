/*
  FastLZ - Byte-aligned LZ77 compression library
  Copyright (C) 2005-2020 Ariya Hidayat <ariya.hidayat@gmail.com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include "plugin.h"
#include "zvplugin.h"
#include "fastlz.h"

/*
 * Specialize custom 64-bit implementation for speed improvements.
 */
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
#define FLZ_ARCH64
#endif

#define FASTLZ_BOUND_CHECK(cond) if (!(cond)) return 0

#define MAX_L1_DISTANCE 8192
#define MAX_L2_DISTANCE 8191
#define MAX_FARDISTANCE (65535UL + MAX_L2_DISTANCE - 1)


/*
 * Workaround for DJGPP to find uint8_t, uint16_t, etc.
 */
#if defined(__MSDOS__) && defined(__GNUC__)
#include <stdint-gcc.h>
#endif

static size_t fastlz1_decompress(const void *input, size_t length, void *output, size_t maxout)
{
	const uint8_t *ip = (const uint8_t *) input;
	const uint8_t *ip_limit = ip + length;
	const uint8_t *ip_bound = ip_limit - 2;
	uint8_t *op = (uint8_t *) output;
	uint8_t *op_limit = op + maxout;
	uint32_t ctrl = (*ip++) & 31;

	for (;;)
	{
		if (ctrl >= 32)
		{
			uint32_t len = (ctrl >> 5) - 1;
			uint32_t ofs = (ctrl & 31) << 8;
			const uint8_t *ref = op - ofs - 1;

			if (len == 7 - 1)
			{
				FASTLZ_BOUND_CHECK(ip <= ip_bound);
				len += *ip++;
			}
			ref -= *ip++;
			len += 3;
			FASTLZ_BOUND_CHECK(op + len <= op_limit);
			FASTLZ_BOUND_CHECK(ref >= (uint8_t *) output);
			memmove(op, ref, len);
			op += len;
		} else
		{
			ctrl++;
			FASTLZ_BOUND_CHECK(op + ctrl <= op_limit);
			FASTLZ_BOUND_CHECK(ip + ctrl <= ip_limit);
			memcpy(op, ip, ctrl);
			ip += ctrl;
			op += ctrl;
		}

		if (ip > ip_bound)
			break;
		ctrl = *ip++;
	}

	return op - (uint8_t *) output;
}


static size_t fastlz2_decompress(const void *input, size_t length, void *output, size_t maxout)
{
	const uint8_t *ip = (const uint8_t *) input;
	const uint8_t *ip_limit = ip + length;
	const uint8_t *ip_bound = ip_limit - 2;
	uint8_t *op = (uint8_t *) output;
	uint8_t *op_limit = op + maxout;
	uint32_t ctrl = (*ip++) & 31;

	for (;;)
	{
		if (ctrl >= 32)
		{
			uint32_t len = (ctrl >> 5) - 1;
			uint32_t ofs = (ctrl & 31) << 8;
			const uint8_t *ref = op - ofs - 1;
			uint8_t code;

			if (len == 7 - 1)
			{
				do
				{
					FASTLZ_BOUND_CHECK(ip <= ip_bound);
					code = *ip++;
					len += code;
				} while (code == 255);
			}
			code = *ip++;
			ref -= code;
			len += 3;

			/* match from 16-bit distance */
			if (code == 255)
			{
				if (ofs == (31 << 8))
				{
					FASTLZ_BOUND_CHECK(ip < ip_bound);
					ofs = (*ip++) << 8;
					ofs += *ip++;
					ref = op - ofs - MAX_L2_DISTANCE - 1;
				}
			}

			FASTLZ_BOUND_CHECK(op + len <= op_limit);
			FASTLZ_BOUND_CHECK(ref >= (uint8_t *) output);
			memmove(op, ref, len);
			op += len;
		} else
		{
			ctrl++;
			FASTLZ_BOUND_CHECK(op + ctrl <= op_limit);
			FASTLZ_BOUND_CHECK(ip + ctrl <= ip_limit);
			memcpy(op, ip, ctrl);
			ip += ctrl;
			op += ctrl;
		}

		if (ip >= ip_limit)
			break;
		ctrl = *ip++;
	}

	return op - (uint8_t *) output;
}


size_t fastlz_decompress(const void *input, size_t length, void *output, size_t maxout)
{
	/* magic identifier for compression level */
	int level = ((*(const uint8_t *) input) >> 5) + 1;

	if (level == 1)
		return fastlz1_decompress(input, length, output, maxout);
	if (level == 2)
		return fastlz2_decompress(input, length, output, maxout);

	/* unknown level, trigger error */
	return 0;
}


#if 0 /* compression not needed */

#define MAX_COPY 32
#define MAX_LEN 264						/* 256 + 8 */

#define HASH_LOG 13
#define HASH_SIZE (1 << HASH_LOG)
#define HASH_MASK (HASH_SIZE - 1)

static uint32_t htab[HASH_SIZE];

#if defined(FLZ_ARCH64)

static uint32_t flz_readu32(const void *ptr)
{
	return *(const uint32_t *) ptr;
}

static uint32_t flz_cmp(const uint8_t *p, const uint8_t *q, const uint8_t *r)
{
	const uint8_t *start = p;

	if (flz_readu32(p) == flz_readu32(q))
	{
		p += 4;
		q += 4;
	}
	while (q < r)
		if (*p++ != *q++)
			break;
	return p - start;
}

#else

static uint32_t flz_readu32(const void *ptr)
{
	const uint8_t *p = (const uint8_t *) ptr;

	return ((uint32_t)p[3] << 24) | ((uint32_t)p[2] << 16) | ((uint32_t)p[1] << 8) | (uint32_t)p[0];
}

static uint32_t flz_cmp(const uint8_t *p, const uint8_t *q, const uint8_t *r)
{
	const uint8_t *start = p;

	while (q < r)
		if (*p++ != *q++)
			break;
	return p - start;
}

#endif /* !FLZ_ARCH64 */


static uint8_t *flz1_match(uint32_t len, uint32_t distance, uint8_t *op)
{
	--distance;
	if (len > MAX_LEN - 2)
	{
		while (len > MAX_LEN - 2)
		{
			*op++ = (7 << 5) + (distance >> 8);
			*op++ = MAX_LEN - 2 - 7 - 2;
			*op++ = (distance & 255);
			len -= MAX_LEN - 2;
		}
	}
	if (len < 7)
	{
		*op++ = (len << 5) + (distance >> 8);
		*op++ = (distance & 255);
	} else
	{
		*op++ = (7 << 5) + (distance >> 8);
		*op++ = len - 7;
		*op++ = (distance & 255);
	}
	return op;
}


/* special case of memcpy: exactly MAX_COPY bytes */
static void flz_maxcopy(void *dest, const void *src)
{
#if defined(FLZ_ARCH64)
	const uint32_t *p = (const uint32_t *) src;
	uint32_t *q = (uint32_t *) dest;

	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
#else
	memcpy(dest, src, MAX_COPY);
#endif
}


/* special case of memcpy: at most MAX_COPY bytes */
static void flz_smallcopy(uint8_t *dest, const uint8_t *src, uint32_t count)
{
#if defined(FLZ_ARCH64)
	if (count >= 4)
	{
		const uint32_t *p = (const uint32_t *) src;
		uint32_t *q = (uint32_t *) dest;

		while (count > 4)
		{
			*q++ = *p++;
			count -= 4;
			dest += 4;
			src += 4;
		}
	}
#endif
	memcpy(dest, src, count);
}


static uint8_t *flz_literals(uint32_t runs, const uint8_t *src, uint8_t *dest)
{
	while (runs >= MAX_COPY)
	{
		*dest++ = MAX_COPY - 1;
		flz_maxcopy(dest, src);
		src += MAX_COPY;
		dest += MAX_COPY;
		runs -= MAX_COPY;
	}
	if (runs > 0)
	{
		*dest++ = runs - 1;
		flz_smallcopy(dest, src, runs);
		dest += runs;
	}
	return dest;
}


static uint16_t flz_hash(uint32_t v)
{
	uint32_t h = (v * 2654435769L) >> (32 - HASH_LOG);

	return h & HASH_MASK;
}


static size_t fastlz1_compress(const void *input, size_t length, void *output)
{
	const uint8_t *ip = (const uint8_t *) input;
	const uint8_t *ip_start = ip;
	const uint8_t *ip_bound = ip + length - 4;	/* because readU32 */
	const uint8_t *ip_limit = ip + length - 12 - 1;
	uint8_t *op = (uint8_t *) output;
	uint32_t seq, hash;
	const uint8_t *anchor;
	uint32_t copy;

	/* initializes hash table */
	for (hash = 0; hash < HASH_SIZE; ++hash)
		htab[hash] = 0;

	/* we start with literal copy */
	anchor = ip;

	ip += 2;

	/* main loop */
	while (ip < ip_limit)
	{
		const uint8_t *ref;
		uint32_t distance, cmp;
		uint32_t len;

		/* find potential match */
		do
		{
			seq = flz_readu32(ip) & 0xffffffUL;
			hash = flz_hash(seq);
			ref = ip_start + htab[hash];
			htab[hash] = ip - ip_start;
			distance = ip - ref;
			cmp = distance < MAX_L1_DISTANCE ? flz_readu32(ref) & 0xffffffUL : 0x1000000UL;
			if (ip >= ip_limit)
				break;
			++ip;
		} while (seq != cmp);

		if (ip >= ip_limit)
			break;
		--ip;

		if (ip > anchor)
		{
			op = flz_literals(ip - anchor, anchor, op);
		}

		len = flz_cmp(ref + 3, ip + 3, ip_bound);

		op = flz1_match(len, distance, op);

		/* update the hash at match boundary */
		ip += len;
		seq = flz_readu32(ip);
		hash = flz_hash(seq & 0xffffffUL);
		htab[hash] = ip++ - ip_start;
		seq >>= 8;
		hash = flz_hash(seq);
		htab[hash] = ip++ - ip_start;

		anchor = ip;
	}

	copy = (const uint8_t *) input + length - anchor;

	op = flz_literals(copy, anchor, op);

	return op - (uint8_t *) output;
}


static uint8_t *flz2_match(uint32_t len, uint32_t distance, uint8_t *op)
{
	--distance;
	if (distance < MAX_L2_DISTANCE)
	{
		if (len < 7)
		{
			*op++ = (len << 5) + (distance >> 8);
			*op++ = (distance & 255);
		} else
		{
			*op++ = (7 << 5) + (distance >> 8);
			for (len -= 7; len >= 255; len -= 255)
				*op++ = 255;
			*op++ = len;
			*op++ = (distance & 255);
		}
	} else
	{
		/* far away, but not yet in the another galaxy... */
		if (len < 7)
		{
			distance -= MAX_L2_DISTANCE;
			*op++ = (len << 5) + 31;
			*op++ = 255;
			*op++ = distance >> 8;
			*op++ = distance & 255;
		} else
		{
			distance -= MAX_L2_DISTANCE;
			*op++ = (7 << 5) + 31;
			for (len -= 7; len >= 255; len -= 255)
				*op++ = 255;
			*op++ = len;
			*op++ = 255;
			*op++ = distance >> 8;
			*op++ = distance & 255;
		}
	}
	return op;
}


static size_t fastlz2_compress(const void *input, size_t length, void *output)
{
	const uint8_t *ip = (const uint8_t *) input;
	const uint8_t *ip_start = ip;
	const uint8_t *ip_bound = ip + length - 4;	/* because readU32 */
	const uint8_t *ip_limit = ip + length - 12 - 1;
	uint8_t *op = (uint8_t *) output;
	uint32_t seq, hash;
	const uint8_t *anchor;
	uint32_t copy;

	/* initializes hash table */
	for (hash = 0; hash < HASH_SIZE; ++hash)
		htab[hash] = 0;

	/* we start with literal copy */
	anchor = ip;

	ip += 2;

	/* main loop */
	while (ip < ip_limit)
	{
		const uint8_t *ref;
		uint32_t distance, cmp;
		uint32_t len;

		/* find potential match */
		do
		{
			seq = flz_readu32(ip) & 0xffffffUL;
			hash = flz_hash(seq);
			ref = ip_start + htab[hash];
			htab[hash] = ip - ip_start;
			distance = ip - ref;
			cmp = distance < MAX_FARDISTANCE ? flz_readu32(ref) & 0xffffffUL : 0x1000000UL;
			if (ip >= ip_limit)
				break;
			++ip;
		} while (seq != cmp);

		if (ip >= ip_limit)
			break;

		--ip;

		/* far, needs at least 5-byte match */
		if (distance >= MAX_L2_DISTANCE)
		{
			if (ref[3] != ip[3] || ref[4] != ip[4])
			{
				++ip;
				continue;
			}
		}

		if (ip > anchor)
		{
			op = flz_literals(ip - anchor, anchor, op);
		}

		len = flz_cmp(ref + 3, ip + 3, ip_bound);

		op = flz2_match(len, distance, op);

		/* update the hash at match boundary */
		ip += len;
		seq = flz_readu32(ip);
		hash = flz_hash(seq & 0xffffffUL);
		htab[hash] = ip++ - ip_start;
		seq >>= 8;
		hash = flz_hash(seq);
		htab[hash] = ip++ - ip_start;

		anchor = ip;
	}

	copy = (const uint8_t *) input + length - anchor;

	op = flz_literals(copy, anchor, op);

	/* marker for fastlz2 */
	*(uint8_t *) output |= (1 << 5);

	return op - (uint8_t *) output;
}


size_t fastlz_compress(const void *input, size_t length, void *output)
{
	/* for short block, choose fastlz1 */
	if (length < 65536UL)
		length = fastlz1_compress(input, length, output);
	else
		length = fastlz2_compress(input, length, output);
	return length;
}


size_t fastlz_compress_level(int level, const void *input, size_t length, void *output)
{
	if (level == 1)
		length = fastlz1_compress(input, length, output);
	else if (level == 2)
		length = fastlz2_compress(input, length, output);
	else
		length = 0;

	return length;
}

#endif

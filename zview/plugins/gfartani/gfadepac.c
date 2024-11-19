__attribute__((noinline))
static void gfa_depac(uint16_t *src, uint16_t *dst, uint16_t count)
{
	int bitcount;
	uint16_t inval;
	uint16_t outval;
	int bits_to_read;
	uint16_t bitbuffer;
	
#define SHIFT() \
	bitbuffer = *src; \
	do { \
		inval <<= 1; \
		if (bitbuffer & 0x8000) \
			inval |= 1; \
		bitbuffer <<= 1; \
		if (--bitcount < 0) \
		{ \
			bitcount = 15; \
			src++; \
			bitbuffer = *src; \
		} \
	} while (--bits_to_read >= 0); \
	*src = bitbuffer
	
	
	inval = *src++;
	bitcount = 15;
	do
	{
		if (!(inval & 0x8000))
		{
			outval = 0;
			bits_to_read = 0;
		} else if (!(inval & 0x4000))
		{
			outval = -1;
			bits_to_read = 1;
		} else
		{
			bits_to_read = 1;
			SHIFT();
			outval = inval;
			bits_to_read = 15;
		}
		SHIFT();
		*dst++ = outval;
		count -= 2;
	} while (count != 0);
#undef SHIFT
}


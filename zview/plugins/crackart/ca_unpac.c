__attribute__((noinline))
static void ca_decompress(const uint8_t *filedata, uint8_t *screen)
{
	uint8_t *dst;
	uint8_t *end;
	uint8_t esc;
	uint8_t c;
	long offset;
	long runs;
	int count;
	
	dst = screen;
	end = screen + SCREEN_SIZE;
	esc = *filedata++;
	c = *filedata++;
	offset = *filedata++;
	offset <<= 8;
	offset |= *filedata++;
	offset &= 0x7fff;
	
	memset(screen, c, SCREEN_SIZE);
	runs = offset;
	if (--runs < 0)
		return;
	for (;;)
	{
		c = *filedata++;
		if (c == esc)
		{
			/* get 2nd byte */
			c = *filedata++;
			if (c == esc)
			{
				*dst = c;
				dst += offset;
				if (dst >= end)
				{
					screen++;
					dst = screen;
					if (--runs < 0)
						return;
				}
			} else
			{
				if (c == 0)
				{
					/* COMP0 */
					count = *filedata++;
					c = *filedata++;
					for (;;)
					{
						*dst = c;
						dst += offset;
						if (dst >= end)
						{
							screen++;
							dst = screen;
							if (--runs < 0)
								return;
						}
						if (--count < 0)
							break;
					}
				} else if (c == 1)
				{
					/* COMP1 */
					count = *filedata++;
					count <<= 8;
					count |= *filedata++;
					c = *filedata++;
					for (;;)
					{
						*dst = c;
						dst += offset;
						if (dst >= end)
						{
							screen++;
							dst = screen;
							if (--runs < 0)
								return;
						}
						if (--count < 0)
							break;
					}
				} else if (c == 2)
				{
					count = *filedata++;
					/* Abbruchcode ESC 02 00 */
					if (count == 0)
						return;
					count <<= 8;
					count |= *filedata++;
					for (;;)
					{
						dst += offset;
						if (dst >= end)
						{
							screen++;
							dst = screen;
							if (--runs < 0)
								return;
						}
						if (--count < 0)
							break;
					}
				} else
				{
					count = c;
					c = *filedata++;
					for (;;)
					{
						*dst = c;
						dst += offset;
						if (dst >= end)
						{
							screen++;
							dst = screen;
							if (--runs < 0)
								return;
						}
						if (--count < 0)
							break;
					}
				}
			}
		} else
		{
			/* write esc */
			*dst = c;
			dst += offset;
			if (dst >= end)
			{
				screen++;
				dst = screen;
				if (--runs < 0)
					return;
			}
		}
	}
}



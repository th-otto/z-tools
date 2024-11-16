/* dst, src, width, height, planes (basically iff to st format) v1.00 */
static void tangle_bitplanes(uint8_t *dst, uint8_t *src, int32_t xres, int32_t yres, int32_t bits)
{
	int32_t xwords = (xres + 15) >> 4;	/* (xres+15)/16 */

	if (bits == 1)
	{
		memcpy(dst, src, (xwords * 2) * yres);
	} else
	{
		int32_t i = yres;

		while (i > 0)
		{
			int32_t j;

			for (j = 0; j < xwords; j++)
			{
				int32_t planes;

				for (planes = 0; planes < bits; planes++)
				{
					*dst++ = src[2 * (planes * xwords + j)];
					*dst++ = src[2 * (planes * xwords + j) + 1];
				}
			}
			src += 2 * bits * xwords;
			i--;
		}
	}
}

/*
PackBits Compression Algorithm

The following packing algorithm originated on the Mac, was adopted by
Electronic Arts/Commodore for use in the IFF standard, and then by Tom Hudson
for use in DEGAS Elite. The algorithm is currently used in MacPaint, IFF, and
DEGAS Elite compressed file formats. Each scan line is packed separately, and
packing never extends beyond a scan line.

For a given control byte 'n':
    0 <= n <= 127   : use the next n + 1 bytes literally (no repetition).
 -127 <= n <= -1    : use the next byte -n + 1 times.
         n = -128   : no operation, not used.
*/

static void decode_packbits(unsigned char *dst, unsigned char *src, long int cnt)
{
	/*
	 * decode packbits
	 *  0 ..  127  n+1 literal bytes
	 * -1 .. -127 -n+1 copy's of next charachter
	 * stop decoding as soon as cnt bytes are done
	 */
	while (cnt > 0)
	{
		signed char cmd = (signed char) *src++;

		if (cmd >= 0)
		{								/* literals */
			int i = 1 + (int) cmd;

			if (i > cnt)
			{
				i = (int) cnt;
			}
			cnt -= i;
			while (i > 0)
			{
				*dst++ = *src++;
				i--;
			}
		} else if (cmd != -128)
		{
			int i = 1 - (int) cmd;
			unsigned char c = *src++;

			if (i > cnt)
			{
				i = (int) cnt;
			}
			cnt -= i;
			while (i > 0)
			{
				*dst++ = c;
				i--;
			}
		}
	}
}

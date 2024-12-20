#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
Imagic Picture    *.IC1 (st low resolution)
                  *.IC2 (st medium resolution)
                  *.IC3 (st high resolution)

4 bytes         'IMDC'
1 word          resolution (0 = low res, 1 = medium res, 2 = high res)
16 words        palette
1 word          date (GEMDOS format)
1 word          time (GEMDOS format)
8 bytes         name of base picture file (for delta compression), or zeroes
1 word          length of data?
1 long          registration number
8 bytes         reserved
1 byte          column size (H)
1 byte          number of columns / 80 (W)
1 byte          escape byte (ESC)
--------
67 bytes        total for header

? bytes         compressed data

H and W determine the order of bytes in the 32000-byte bitmap.
If H=255, W is ignored and the bytes are stored in natural order.
Otherwise, the bytes are stored in W*80 columns, H bytes each.
The sample files have H=200, W=2, meaning 160 columns of 200 bytes,
so the bitmap (regardless of resolution) is stored in this order:
0, 160, 320, 480, ..., 31840,
1, 161, 321, 481, ..., 31841,
...

Decoding compressed data:

Read byte x.

x != ESC
    Use x literally.

X == ESC
    Read one more byte: y.

    If y == ESC, output literal ESC byte.

    If y == 0, read two more bytes: n and d,
        Repeat d (n + 1) times.

    If y == 1, keep reading the bytes while they are equal 1.
        Ignore the first non-1 byte. Let c be the count of ones,
        including y. Read two more bytes: n and d.
        Repeat d (c * 256 + n + 1) times.

    If y == 2, read next byte: z.
        If z == 0, end of decoding.
        If z == 1, count the ones as above, then read n.
            Take (c * 256 + n + 1) corresponding bytes from the base picture.
        If z == 2, skip the next bytes until a zero is found.
            Ignore this "ESC 2 non-zeros 0" sequence.
        If z >= 3, read next byte: n.
            Take (n + 1) corresponding bytes from the base picture.

    If y >= 3, read next byte: d.
        Repeat d (y + 1) times.

###############################################################################

Imagic Film    *.IC1 (st low resolution)
               *.IC2 (st medium resolution)
               *.IC3 (st high resolution)

4 bytes         file ID, 'IMDC'
1 word          resolution (0 = low res, 1 = medium res, 2 = high res)
16 words        palette
1 word          date (GEMDOS format)
1 word          time (GEMDOS format)
8 bytes         name of base picture file (for delta compression), or zeroes
1 word          length of data?
1 long          registration number
8 bytes         reserved
1 byte          compressed? (0 = no, 1 = yes)

If compressed {
1 byte          delta-compressed? (-1 = no, > -1 = yes)
1 byte          ?
1 byte          escape byte
}
--------
65 bytes        total for header (68 bytes if compressed)

? bytes         data

Compressed data may be either stand-alone or delta-compressed (relative to the
base picture named in the header). Delta compression involves storing only how
the picture differs from the base picture (i.e., only portions of the screen
that have changed are stored). This is used to to encode animated sequences
efficiently.

Compressed data, stand-alone:

For each byte x in the data section:

        x = escape byte         Read one more byte, n.  (n is unsigned).

                                If n >= 2, use the next byte n times.
                                If n = 1, keep reading bytes until a
                                byte k not equal to 1 is encountered.
                                Then read the next byte d.
                                If the number of 1 bytes encountered is o,
                                use d (256 * o + k) times.  I.e.,

                                if (n == 1) {
                                        o = 0;
                                        while (n == 1) {
                                                o++;
                                                n = next byte;
                                        }

                                        k = n;
                                        d = next byte;

                                        Use d (256 * o + k) times.
                                }
                                else {
                                        d = next byte;
                                        Use d (n) times.
                                }

        x != escape byte        Use x literally.

Compressed data, delta compressed:

For each byte x in the data section:

        x = escape byte         Read one more byte, n.  (n is unsigned).

                                If n >= 3, use the next byte n times.
                                If n = 1, do the same as for n = 1 in
                                stand-alone compression (above).
                                If n = 2, then set n = next byte.
                                        If n = 0, end of picture.
                                        If n >= 2, take n bytes from base
                                        picture.
                                        If n = 1, do the same as for n = 1
                                        in stand-alone compression (above),
                                        but take (256 * o + k) bytes from
                                        base picture.

        x != escape byte        Use x literally.

*/


#ifdef PLUGIN_SLB

long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long) (EXTENSIONS);

	case INFO_NAME:
		return (long)NAME;
	case INFO_VERSION:
		return VERSION;
	case INFO_DATETIME:
		return (long)DATE;
	case INFO_AUTHOR:
		return (long)AUTHOR;
#ifdef MISC_INFO
	case INFO_MISC:
		return (long)MISC_INFO;
#endif
	case INFO_COMPILER:
		return (long)(COMPILER_VERSION_STRING);
	}
	return -ENOSYS;
}
#endif

#define SCREEN_SIZE 32000L

#undef time

struct file_header {
	uint32_t magic;
	uint16_t resolution;
	uint16_t palette[16];
	uint16_t date;
	uint16_t time;
	char base_name[8];
	uint16_t data_length;
	uint32_t registration;
	uint8_t reserved[8];
};

void __CDECL depack_imagic(uint8_t *src, uint8_t *dst) ASM_NAME("depack_imagic");

#ifdef __GNUC__
void __CDECL depack_imagic(uint8_t *src, uint8_t *dst)
{
	__asm__ __volatile__(
	" movea.l    %0,%%a0\n"	/* src_addr */
	" movea.l    %1,%%a1\n"	/* dst_addr */
	" move.l     #32000,%%d6\n"
	" move.l     %%a1,start_adr\n"
	" movea.l    %%a1,%%a3\n"
#ifdef __mcoldfire__
	" adda.l     %%d6,%%a3\n"
#else
	" adda.w     %%d6,%%a3\n"
#endif
	" move.l     %%a3,end_adr\n"
	" moveq.l    #0,%%d1\n"
	" moveq.l    #0,%%d2\n"
	" move.b     (%%a0)+,%%d1\n"  /* column size */
	" move.b     (%%a0)+,%%d2\n"  /* number of colums */
	" move.b     (%%a0)+,%%d7\n"  /* escape byte */
	" mulu.w     #80,%%d2\n"
	" cmpi.b     #0xFF,%%d1\n"
	" jbne       depack_imagic_1\n"
	" move.w     %%d6,%%d1\n"
	" move.w     #1,%%d2\n"
"depack_imagic_1:\n"
	" movea.w    %%d2,%%a4\n"
	" move.w     %%d2,%%d6\n"
#ifdef __mcoldfire__
	" subq.l     #1,%%d6\n"
	" move.l     %%d1,%%d5\n"
	" subq.l     #1,%%d5\n"
	" movea.l    %%d5,%%a3\n"
	" neg.l      %%d1\n"
#else
	" subq.w     #1,%%d6\n"
	" move.w     %%d1,%%d5\n"
	" subq.w     #1,%%d5\n"
	" movea.w    %%d5,%%a3\n"
	" neg.w      %%d1\n"
#endif
	" muls.w     %%d2,%%d1\n"
	" addq.l     #1,%%d1\n"
	" movea.l    %%d1,%%a5\n"
	" muls.w     %%d5,%%d2\n"
	" movea.l    %%d2,%%a2\n"
	" moveq.l    #1,%%d1\n"
	" moveq.l    #3,%%d2\n"
	" moveq.l    #2,%%d4\n"
	" moveq.l    #0,%%d0\n"
"depack_imagic_4:\n"
	" move.b     (%%a0)+,%%d0\n"
	" cmp.b      %%d0,%%d7\n"
	" jbeq       depack_imagic_2\n"
"depack_imagic_5:\n"
	" cmpa.l     start_adr(%%pc),%%a1\n"
	" jbmi       depack_imagic_end\n"
	" cmpa.l     end_adr(%%pc),%%a1\n"
	" jbpl       depack_imagic_end\n"
	" move.b     %%d0,(%%a1)\n"
	" adda.l     %%a4,%%a1\n"
#ifdef __mcoldfire__
	" subq.l     #1,%%d5\n"
	" jbpl       depack_imagic_4\n"
	" move.l     %%a3,%%d5\n"
#else
	" dbf        %%d5,depack_imagic_4\n"
	" move.w     %%a3,%%d5\n"
#endif
	" adda.l     %%a5,%%a1\n"
#ifdef __mcoldfire__
	" subq.l     #1,%%d6\n"
	" jbpl       depack_imagic_4\n"
#else
	" dbf        %%d6,depack_imagic_4\n"
#endif
	" move.w     %%a4,%%d6\n"
#ifdef __mcoldfire__
	" subq.l     #1,%%d6\n"
#else
	" subq.w     #1,%%d6\n"
#endif
	" adda.l     %%a2,%%a1\n"
	" jbra       depack_imagic_4\n"
"depack_imagic_2:\n"
	" move.b     (%%a0)+,%%d0\n"
	" cmp.b      %%d0,%%d7\n"
	" jbeq       depack_imagic_5\n"
	" moveq.l    #0,%%d3\n"
	" cmp.w      %%d2,%%d0\n"
	" jbpl       depack_imagic_6\n"
	" cmp.b      %%d4,%%d0\n"
	" jbne       depack_imagic_7\n"
	" move.b     (%%a0)+,%%d0\n"
	" jbeq       depack_imagic_end\n"
	" cmp.w      %%d2,%%d0\n"
	" jbpl       depack_imagic_8\n"
	" cmp.b      %%d4,%%d0\n"
	" jbeq       depack_imagic_9\n"
"depack_imagic_11:\n"
	" cmp.b      %%d1,%%d0\n"
	" jbne       depack_imagic_10\n"
#ifdef __mcoldfire__
	" addi.l     #256,%%d3\n"
#else
	" addi.w     #256,%%d3\n"
#endif
	" move.b     (%%a0)+,%%d0\n"
	" jbra       depack_imagic_11\n"
"depack_imagic_10:\n"
	" move.b     (%%a0)+,%%d0\n"
"depack_imagic_8:\n"
#ifdef __mcoldfire__
	" add.l      %%d0,%%d3\n"
#else
	" add.w      %%d0,%%d3\n"
#endif
"depack_imagic_13:\n"
	" adda.l     %%a4,%%a1\n"
#ifdef __mcoldfire__
	" subq.l     #1,%%d5\n"
	" bpl        depack_imagic_12\n"
	" move.l     %%a3,%%d5\n"
#else
	" dbf        %%d5,depack_imagic_12\n"
	" move.w     %%a3,%%d5\n"
#endif
	" adda.l     %%a5,%%a1\n"
#ifdef __mcoldfire__
	" subq.l     #1,%%d6\n"
	" bpl        depack_imagic_12\n"
#else
	" dbf        %%d6,depack_imagic_12\n"
#endif
	" move.w     %%a4,%%d6\n"
#ifdef __mcoldfire__
	" subq.l     #1,%%d6\n"
#else
	" subq.w     #1,%%d6\n"
#endif
	" adda.l     %%a2,%%a1\n"
"depack_imagic_12:\n"
#ifdef __mcoldfire__
	" subq.l     #1,%%d3\n"
	" bpl        depack_imagic_13\n"
#else
	" dbf        %%d3,depack_imagic_13\n"
#endif
	" jbra       depack_imagic_4\n"
"depack_imagic_7:\n"
	" cmp.b      %%d1,%%d0\n"
	" jbne       depack_imagic_14\n"
#ifdef __mcoldfire__
	" addi.l     #256,%%d3\n"
#else
	" addi.w     #256,%%d3\n"
#endif
	" move.b     (%%a0)+,%%d0\n"
	" jbra       depack_imagic_7\n"
"depack_imagic_14:\n"
	" move.b     (%%a0)+,%%d0\n"
"depack_imagic_6:\n"
#ifdef __mcoldfire__
	" add.l      %%d0,%%d3\n"
#else
	" add.w      %%d0,%%d3\n"
#endif
	" move.b     (%%a0)+,%%d0\n"
"depack_imagic_16:\n"
	" cmpa.l     start_adr(%%pc),%%a1\n"
	" jbmi       depack_imagic_end\n"
	" cmpa.l     end_adr(%%pc),%%a1\n"
	" jbpl       depack_imagic_end\n"
	" move.b     %%d0,(%%a1)\n"
	" adda.l     %%a4,%%a1\n"
#ifdef __mcoldfire__
	" subq.l     #1,%%d5\n"
	" bpl        depack_imagic_15\n"
	" move.l     %%a3,%%d5\n"
#else
	" dbf        %%d5,depack_imagic_15\n"
	" move.w     %%a3,%%d5\n"
#endif
	" adda.l     %%a5,%%a1\n"
#ifdef __mcoldfire__
	" subq.l     #1,%%d6\n"
	" bpl        depack_imagic_15\n"
#else
	" dbf        %%d6,depack_imagic_15\n"
#endif
	" move.w     %%a4,%%d6\n"
#ifdef __mcoldfire__
	" subq.l     #1,%%d6\n"
#else
	" subq.w     #1,%%d6\n"
#endif
	" adda.l     %%a2,%%a1\n"
"depack_imagic_15:\n"
#ifdef __mcoldfire__
	" subq.l     #1,%%d3\n"
	" bpl        depack_imagic_16\n"
#else
	" dbf        %%d3,depack_imagic_16\n"
#endif
	" jbra       depack_imagic_4\n"
"depack_imagic_9:\n"
	" move.b     (%%a0)+,%%d0\n"
	" beq        depack_imagic_4\n"
	" jbra       depack_imagic_9\n"

"start_adr: .dc.l 0\n"
"end_adr: .dc.l 0\n"

"depack_imagic_end:\n"

	:
	: "g"(src), "g"(dst)
	: "a0", "a1", "a2", "a3", "a4", "a5", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "cc" AND_MEMORY);
}
#endif

/*==================================================================================*
 * boolean __CDECL reader_init:														*
 *		Open the file "name", fit the "info" struct. ( see zview.h) and make others	*
 *		things needed by the decoder.												*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		name		->	The file to open.											*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	size_t file_size;
	int16_t handle;
	struct file_header header;
	uint8_t *bmap;
	uint8_t *temp;

	handle = (int)Fopen(name, FO_READ);
	if (handle < 0)
	{
		return FALSE;
	}
	if (Fread(handle, sizeof(header), &header) != sizeof(header) ||
		header.magic != 0x494D4443L) /* 'IMDC' */
	{
		Fclose(handle);
		return FALSE;
	}
	switch (header.resolution)
	{
	case 0:
		info->width = 320;
		info->height = 200;
		info->planes = 4;
		info->components = 3;
		info->indexed_color = TRUE;
		break;
	case 1:
		info->width = 640;
		info->height = 200;
		info->planes = 2;
		info->components = 3;
		info->indexed_color = TRUE;
		break;
	case 2:
		info->width = 640;
		info->height = 400;
		info->planes = 1;
		info->components = 1;
		info->indexed_color = FALSE;
		break;
	default:
		Fclose(handle);
		return FALSE;
	}

	file_size = Fseek(0, handle, SEEK_END);
	Fseek(sizeof(header), handle, SEEK_SET);
	file_size -= sizeof(header);
	temp = malloc(file_size + 2);
	if (temp == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	if ((size_t)Fread(handle, file_size, temp + 1) != file_size)
	{
		free(temp);
		Fclose(handle);
		return FALSE;
	}
	Fclose(handle);

	if (temp[1] != 0)
	{
		bmap = malloc(SCREEN_SIZE);
		if (bmap == NULL)
		{
			free(temp);
			return FALSE;
		}
		depack_imagic(temp + 1, bmap);
		strcpy(info->compression, "RLE");
		free(temp);
		info->_priv_ptr_more = bmap;
	} else
	{
		bmap = temp;
		info->_priv_ptr_more = bmap + 2;
		strcpy(info->compression, "None");
	}

	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "Imagic (Image)");
	
	{
		int i;

		for (i = 0; i < 16; i++)
		{
			info->palette[i].red = (((header.palette[i] >> 7) & 0x0e) + ((header.palette[i] >> 11) & 0x01)) * 17;
			info->palette[i].green = (((header.palette[i] >> 3) & 0x0e) + ((header.palette[i] >> 7) & 0x01)) * 17;
			info->palette[i].blue = (((header.palette[i] << 1) & 0x0e) + ((header.palette[i] >> 3) & 0x01)) * 17;
		}
	}
	
	return TRUE;
}


/*==================================================================================*
 * boolean __CDECL reader_read:														*
 *		This function fits the buffer with image data								*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		buffer		->	The destination buffer.										*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      TRUE if all ok else FALSE.													*
 *==================================================================================*/
boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	switch (info->planes)
	{
	case 1:
		{
			uint8_t *src;
			int16_t x;
			uint8_t byte;

			src = (uint8_t *)info->_priv_ptr_more;
			x = info->width >> 3;
			do
			{
				byte = *src++;
				*buffer++ = (byte >> 7) & 1;
				*buffer++ = (byte >> 6) & 1;
				*buffer++ = (byte >> 5) & 1;
				*buffer++ = (byte >> 4) & 1;
				*buffer++ = (byte >> 3) & 1;
				*buffer++ = (byte >> 2) & 1;
				*buffer++ = (byte >> 1) & 1;
				*buffer++ = (byte >> 0) & 1;
			} while (--x > 0);
			info->_priv_ptr_more = src;
		}
		break;
	
	case 2:
		{
			uint16_t *src;
			int16_t x;
			int bit;
			uint16_t plane0;
			uint16_t plane1;

			src = (uint16_t *)(info->_priv_ptr_more);
			x = (info->width + 15) >> 4;
			do
			{
				plane0 = *src++;
				plane1 = *src++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2);
					plane0 <<= 1;
					plane1 <<= 1;
				}
			} while (--x > 0);
			info->_priv_ptr_more = src;
		}
		break;

	case 4:
		{
			uint16_t *src;
			int16_t x;
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;

			src = (uint16_t *)(info->_priv_ptr_more);
			x = (info->width + 15) >> 4;
			do
			{
				plane0 = *src++;
				plane1 = *src++;
				plane2 = *src++;
				plane3 = *src++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2) |
						((plane2 >> 13) & 4) |
						((plane3 >> 12) & 8);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
					plane3 <<= 1;
				}
			} while (--x > 0);
			info->_priv_ptr_more = src;
		}
		break;
	}

	return TRUE;
}


/*==================================================================================*
 * boolean __CDECL reader_quit:														*
 *		This function makes the last job like close the file handler and free the	*
 *		allocated memory.															*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL reader_quit(IMGINFO info)
{
	free(info->_priv_ptr);
	info->_priv_ptr = 0;
}


/*==================================================================================*
 * boolean __CDECL reader_get_txt													*
 *		This function , like other function must be always present.					*
 *		It fills txtdata struct. with the text present in the picture ( if any).	*
 *----------------------------------------------------------------------------------*
 * input:																			*
 *		txtdata		->	The destination text buffer.								*
 *		info		->	The IMGINFO struct. to fit.									*
 *----------------------------------------------------------------------------------*
 * return:	 																		*
 *      --																			*
 *==================================================================================*/
void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}

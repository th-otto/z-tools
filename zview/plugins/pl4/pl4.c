#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
PL4    *.PL4

Created by Sascha Springer: http://blog.anides.de/

These are essentially 2 st low resolution screens interlaced together.
When viewed the human eye merges the two alternating images into one.

Image size is always 320x200 pixels and they are compressed using LZ4.

After decompression:

1 word         resolution [0 = low]
16 words       palette 1
32000 bytes    screen 1
1 word         ?
1 word         resolution [0 = low]
16 words       palette 2
32000 bytes    screen 2
-----------
64070 bytes    total

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


#ifdef __GNUC__
/*
 * Written by Sascha Springer
 * Ported to PureC by Lonny Pursell
 * Ported to GNU-C by Thorsten Otto
 */
static void decode_lz4(uint8_t *src, uint8_t *dst)
{
	__asm__ __volatile__(
	" movea.l	%0,%%a0\n"
	" movea.l	%1,%%a1\n"
	" addq.l	#7,%%a0\n"
	" move.l	%%a7,%%a2\n"

	" moveq.l 	#0,%%d2\n"
"data_block_loop:\n"
	" move.b	(%%a0)+,-(%%a2)\n"
	" move.b	(%%a0)+,-(%%a2)\n"
	" move.b	(%%a0)+,-(%%a2)\n"
	" move.b	(%%a0)+,-(%%a2)\n"
	" move.l	(%%a2)+,%%d3\n"
	" bmi.s		block_copy\n"
	" beq.s		end_of_stream\n"

	" lea		(%%a0,%%d3.l),%%a4\n"

"lz4_block_loop:\n"
	" moveq.l	#0,%%d0\n"
	" move.b	(%%a0)+,%%d0\n"

#ifdef __mcoldfire__
	" moveq		#0,%%d1\n"
	" move.w	%%d0,%%d1\n"
	" lsr.l		#4,%%d1\n"
	" beq.s		no_literals\n"
	" subq.l	#1,%%d1\n"
#else
	" move.w	%%d0,%%d1\n"
	" lsr.w		#4,%%d1\n"
	" beq.s		no_literals\n"
	" subq.w	#1,%%d1\n"
#endif

	" cmp.w		#15-1,%%d1\n"
	" bne.s		literals_copy_loop\n"

"additional_literals_length:\n"
	" move.b	(%%a0)+,%%d2\n"
#ifdef __mcoldfire__
	" add.l		%%d2,%%d1\n"
	" not.l		%%d2\n"
	" mvz.b		%%d2,%%d2\n"
#else
	" add.w		%%d2,%%d1\n"
	" not.b		%%d2\n"
#endif
	" beq.s		additional_literals_length\n"

"literals_copy_loop:\n"
	" move.b	(%%a0)+,(%%a1)+\n"

#ifdef __mcoldfire__
	" subq.l    #1,%%d1\n"
	" bpl       literals_copy_loop\n"
#else
	" dbf		%%d1,literals_copy_loop\n"
#endif

"no_literals:\n"
	" cmp.l		%%a4,%%a0\n"
	" beq.s		data_block_loop\n"

	" moveq.l	#0,%%d3\n"
	" move.b	(%%a0)+,-(%%a2)\n"
	" move.b	(%%a0)+,-(%%a2)\n"
	" move.w	(%%a2)+,%%d3\n"
	" neg.l		%%d3\n"
	" lea		(%%a1,%%d3.l),%%a3\n"

#ifdef __mcoldfire__
	" and.l		#15,%%d0\n"
	" addq.l	#4-1,%%d0\n"
#else
	" and.w		#15,%%d0\n"
	" addq.w	#4-1,%%d0\n"
#endif
	" cmp.w		#15+4-1,%%d0\n"
	" bne.s		copy_match_loop\n"

"additional_match_length:\n"
	" move.b	(%%a0)+,%%d2\n"
#ifdef __mcoldfire__
	" add.l		%%d2,%%d0\n"
	" not.l		%%d2\n"
	" mvz.b		%%d2,%%d2\n"
#else
	" add.w		%%d2,%%d0\n"
	" not.b		%%d2\n"
#endif
	" beq.s		additional_match_length\n"

"copy_match_loop:\n"
	" move.b	(%%a3)+,(%%a1)+\n"

#ifdef __mcoldfire__
	" subq.l    #1,%%d0\n"
	" bpl       copy_match_loop\n"
#else
	" dbf		%%d0,copy_match_loop\n"
#endif

	" bra.s		lz4_block_loop\n"

"block_copy:\n"
	" and.l		#0x7fffffff,%%d3\n"

"block_copy_loop:\n"
	" move.b	(%%a0)+,(%%a1)+\n"

	" subq.l	#1,%%d3\n"
	" jbne  	block_copy_loop\n"

	" jbra		data_block_loop\n"

"end_of_stream:\n"
	
	:
	: "g"(src), "g"(dst)
	:  "a0", "a1", "a2", "a3", "a4", "d0", "d1", "d2", "d3", "cc" AND_MEMORY);
}
#else
void __CDECL decode_lz4(uint8_t *src, uint8_t *dst) ASM_NAME("decode_lz4");
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
	int16_t handle;
	uint8_t *bmap;
	uint8_t *temp;
	size_t file_size;
	uint16_t *palette0;
	uint16_t *palette1;
	uint8_t red0[16];
	uint8_t green0[16];
	uint8_t blue0[16];
	uint8_t red1[16];
	uint8_t green1[16];
	uint8_t blue1[16];

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);
	
	bmap = malloc(2 * SCREEN_SIZE + 580);
	temp = malloc(file_size + 256);
	if (temp == NULL || bmap == NULL)
	{
		free(temp);
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if ((size_t)Fread(handle, file_size, temp) != file_size)
	{
		free(temp);
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	Fclose(handle);
	decode_lz4(temp, bmap);
	free(temp);

	palette0 = (uint16_t *)(bmap + 2);
	palette1 = (uint16_t *)(bmap + SCREEN_SIZE + 38);
	
	{	
		int i, j, n;

		for (i = 0; i < 16; i++)
		{
			red0[i] = (((palette0[i] >> 7) & 0x0e) + ((palette0[i] >> 11) & 0x01)) * 17;
			green0[i] = (((palette0[i] >> 3) & 0x0e) + ((palette0[i] >> 7) & 0x01)) * 17;
			blue0[i] = (((palette0[i] << 1) & 0x0e) + ((palette0[i] >> 3) & 0x01)) * 17;
			red1[i] = (((palette1[i] >> 7) & 0x0e) + ((palette1[i] >> 11) & 0x01)) * 17;
			green1[i] = (((palette1[i] >> 3) & 0x0e) + ((palette1[i] >> 7) & 0x01)) * 17;
			blue1[i] = (((palette1[i] << 1) & 0x0e) + ((palette1[i] >> 3) & 0x01)) * 17;
		}
		n = 0;
		for (i = 0; i < 16; i++)
		{
			for (j = 0; j < 16; j++, n++)
			{
				info->palette[n].red = (red0[i] + red1[j]) >> 1;
				info->palette[n].green = (green0[i] + green1[j]) >> 1;
				info->palette[n].blue = (blue0[i] + blue1[j]) >> 1;
			}
		}
	}

	info->width = 320;
	info->height = 200;
	info->indexed_color = TRUE;
	info->components = 1;
	info->planes = 8;
	info->colors = 1L << 8;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	RETURN_SUCCESS();
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
	int x;
	int i;
	uint16_t byte1;
	uint16_t byte2;
	int16_t plane;
	size_t pos = info->_priv_var;
	uint8_t *bmap = info->_priv_ptr;
	const uint16_t *screen0 = (const uint16_t *)(bmap + 34);
	const uint16_t *screen1 = (const uint16_t *)(bmap + SCREEN_SIZE + 70);
	
	x = info->width >> 4;
	do
	{
		for (i = 15; i >= 0; i--)
		{
			for (plane = byte1 = 0; plane < 4; plane++)
			{
				if ((screen0[pos + plane] >> i) & 1)
					byte1 |= 1 << plane;
			}
			for (plane = byte2 = 0; plane < 4; plane++)
			{
				if ((screen1[pos + plane] >> i) & 1)
					byte2 |= 1 << plane;
			}
			*buffer++ = byte1 * 16 + byte2;
		}
		pos += 4;
	} while (--x > 0);
	info->_priv_var = pos;
	
	RETURN_SUCCESS();
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

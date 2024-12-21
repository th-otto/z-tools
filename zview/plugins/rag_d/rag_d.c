#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"

/*
                           --------------------------
                           *- Rag - D Picture File -*
                           --------------------------

Header :

Name of Part -|- Length in Bytes  -|- Kind of Data  -|- Function
--------------+--------------------+-----------------+-------------------------
Identify      |  6 Bytes           | Ascii           | Header. MUST be "RAG-D!"
--------------+--------------------+-----------------+-------------------------
PackAlgo      |  2 Bytes           | 1 Word          | used Pack-Algorithm
--------------+--------------------+-----------------+ 0 = unpacked.
..                                                   | rest not used till yet
--------------+--------------------+-----------------+-------------------------
PicLength     |  4 Bytes           | 1 Long unsigned | IMPORTANT !!!
..            |                    |                 | READ APPENDIX A !!
--------------+--------------------+-----------------+-------------------------
PicColumns    |  2 Bytes           | 1 Word unsigned | X - Size of the picture
..            |                    |                 | eg. 320,640 aso, is
..            |                    |                 | always dividable by 16!
--------------+--------------------+-----------------+-------------------------
PicRows       |  2 Bytes           | 1 Word unsigned | Y - Size of the picture
..            |                    |                 | eg. 200,240,480 aso.
--------------+--------------------+-----------------+-------------------------
Planes        |  2 Bytes           | 1 Word unsigned | Number of Bitplains
..            |                    |                 | eg. 1,2,4,8,16 aso
--------------+--------------------+-----------------+-------------------------
PalLength     |  4 Bytes           | 1 Long unsigned | Length of Palette Data
..            |                    |                 | eg. 1024 or 32
--------------+--------------------+-----------------+-------------------------
Contrl_One    |  2 Bytes           | 1 Word unsigned | Control Bits |76543210|
--------------+--------------------+-----------------+ Bit 0 = 1 --> there is
..                                                   | a 370 Bytes long picture
..                                                   | documentation after the
..                                                   | picture - data
..                                                   | Bit 1 = 1 --> there is
..                                                   | a 8 Bytes long info
..                                                   | with date of creation
..                                                   | and working-time behind
..                                                   | the pic-data (and docu-
..                                                   | mentation if exist )
--------------+--------------------+-----------------+-------------------------
Contrl_Two    |  2 Bytes           | 1 Word unsigned | -unused-
--------------+--------------------+-----------------+-------------------------
Contrl_Three  |  2 Bytes           | 1 Word unsigned | -unused-
--------------+--------------------+-----------------+-------------------------
Contrl_Four   |  2 Bytes           | 1 Word unsigned | -unused-
--------------+--------------------+-----------------+-------------------------
PaletteData   |  PalLength Bytes   | Words or Longs  | when Length is = 32 you
--------------+--------------------+-----------------| the ST - Hardware-Format
..                                                   | have been used and if
..                                                   | PalLength= 1024 the F030
..                                                   | Hardware- Registers have
..                                                   | been saved. Just copy
..                                                   | into the registers.
..                                                   | If the picture is a True
..                                                   | Color 16 bit Picture,
..                                                   | here follows a palette
..                                                   | with : <0>,<R>,<G>,<B>
..                                                   | from 0-255 (not 0-31!)
--------------+--------------------+-----------------+-------------------------
PicData       | see Appendix A !   | X - Bytes       | Picture Data as RAW
--------------+--------------------+-----------------+ eg.  : Plane 0 Word 0..
..                                                   |        Plane 1 Word 0..
..                                                   | or in True color :
..                                                   | pixel 0, pixel 1, aso.
--------------+--------------------+-----------------+-------------------------
PicDokument   | 370 Bytes          | ASCII           | description of picture
..                                                   | eg. Name of painter aso
--------------+--------------------+-----------------+-------------------------
PicDate       | 2 Bytes & 1 Word   | all unsigned    | 1. Byte = Day
--------------+--------------------+-----------------+ 2. Byte = Month
..                                                   | 3. & 4. = Year
--------------+--------------------+-----------------+-------------------------
PicWorkTime   | 4 Bytes            | Long unsigned   | Seconds of Worktime
--------------+--------------------+-----------------+-------------------------


APPENDIX A
----------

To get the correct length of the picture data you should do this:

for loading :
NewPicLen = PicLength/PicRows*(PicRows+1)

if you save RAG-D pictures calculate like this :
PicLength = PicColumns/8*(PicRows-1)*Bitplanes

else my program won't read your pictures ! ( or loads and crashes ! )

Don't let the scene die !                           +-------------------------+
                                                    | for more infos write to |
                                                    | Tobias Bonnke           |
                                                    | Postfach 1171           |
                                                    | 72564 Bad Urach         |
                                                    | Germany / Earth         |
                                                    +-------------------------+
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

struct file_header {
	char magic[6];
	int16_t compression;
	uint32_t pic_length;
	uint16_t width;
	uint16_t height;
	uint16_t planes;
	uint32_t pal_length;
	uint16_t control_one;
	uint16_t control_two;
	uint16_t control_three;
	uint16_t control_four;
};

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
	size_t bytes_per_row;
	size_t image_size;
	uint16_t ste_palette[16];
	uint8_t falcon_palette[256 * 4];
	struct file_header header;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	if (memcmp(header.magic, "RAG-D!", 6) != 0)
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	switch (header.planes)
	{
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
		break;
	default:
		Fclose(handle);
		RETURN_ERROR(EC_PixelDepth);
	}
	if (header.planes >= 2 && header.planes <= 8)
		info->indexed_color = TRUE;
	else
		info->indexed_color = FALSE;

	if (header.pal_length == sizeof(ste_palette))
	{
		if (header.planes > 4 && header.planes <= 8)
		{
			Fclose(handle);
			RETURN_ERROR(EC_ColorMapLength);
		}
		if (Fread(handle, sizeof(ste_palette), ste_palette) != sizeof(ste_palette))
		{
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
	} else if (header.pal_length == sizeof(falcon_palette))
	{
		if (Fread(handle, sizeof(falcon_palette), falcon_palette) != sizeof(falcon_palette))
		{
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
	} else if (info->indexed_color)
	{
		Fclose(handle);
		RETURN_ERROR(EC_ColorMapLength);
	}
	
	bytes_per_row = (((size_t)header.width + 15) >> 4) * 2 * header.planes;
	image_size = bytes_per_row * header.height;
	if (image_size != header.pic_length)
	{
		Fclose(handle);
		RETURN_ERROR(EC_BitmapLength);
	}
	header.height++;
	image_size += bytes_per_row;

	bmap = malloc(image_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if ((size_t)Fread(handle, image_size, bmap) != image_size)
	{
		free(bmap);
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	/*
	 * TODO: get description from end of file?
	 */
	Fclose(handle);

	info->planes = header.planes;
	info->width = header.width;
	info->height = header.height;
	info->components = header.planes == 1 ? 1 : 3;
	info->colors = 1L << header.planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_ptr = bmap;

	strcpy(info->info, "Rag-D (Risk Aided Graphics Design)");
	strcpy(info->compression, "None");

	if (info->indexed_color)
	{
		if (header.pal_length == sizeof(ste_palette))
		{
			int i;
		
			for (i = 0; i < 16; i++)
			{
				info->palette[i].red = (((ste_palette[i] >> 7) & 0x0e) + ((ste_palette[i] >> 11) & 0x01)) * 17;
				info->palette[i].green = (((ste_palette[i] >> 3) & 0x0e) + ((ste_palette[i] >> 7) & 0x01)) * 17;
				info->palette[i].blue = (((ste_palette[i] << 1) & 0x0e) + ((ste_palette[i] >> 3) & 0x01)) * 17;
			}
		} else if (header.pal_length == sizeof(falcon_palette))
		{
			int i, j;
		
			for (j = 0, i = 0; i < 256; i++)
			{
				info->palette[i].red = falcon_palette[j++];
				info->palette[i].green = falcon_palette[j++];
				j++;
				info->palette[i].blue = falcon_palette[j++];
			}
		}
	}
	
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
	switch (info->planes)
	{
	case 1:
		{
			int x;
			int16_t byte;
			uint8_t *bmap = info->_priv_ptr;
			size_t src = info->_priv_var;
		
			x = ((info->width + 15) >> 4) << 1;
			info->_priv_var += x;
			do
			{
				byte = bmap[src++];
				*buffer++ = (byte >> 7) & 1;
				*buffer++ = (byte >> 6) & 1;
				*buffer++ = (byte >> 5) & 1;
				*buffer++ = (byte >> 4) & 1;
				*buffer++ = (byte >> 3) & 1;
				*buffer++ = (byte >> 2) & 1;
				*buffer++ = (byte >> 1) & 1;
				*buffer++ = (byte >> 0) & 1;
			} while (--x > 0);
		}
		break;

	case 2:
		{
			int x;
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t *ptr;
			
			ptr = (uint16_t *)((uint8_t *)info->_priv_ptr + info->_priv_var);
			x = (info->width + 15) >> 4;
			info->_priv_var += x << 2;
			do
			{
				plane0 = *ptr++;
				plane1 = *ptr++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2);
					plane0 <<= 1;
					plane1 <<= 1;
				}
			} while (--x > 0);
		}
		break;

	case 4:
		{
			int x;
			int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t *ptr;
			
			ptr = (uint16_t *)((uint8_t *)info->_priv_ptr + info->_priv_var);
			x = (info->width + 15) >> 4;
			info->_priv_var += x << 3;
			do
			{
				plane0 = *ptr++;
				plane1 = *ptr++;
				plane2 = *ptr++;
				plane3 = *ptr++;
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
		}
		break;

	case 8:
		{
			int x;
			unsigned int bit;
			uint16_t plane0;
			uint16_t plane1;
			uint16_t plane2;
			uint16_t plane3;
			uint16_t plane4;
			uint16_t plane5;
			uint16_t plane6;
			uint16_t plane7;
			uint16_t *ptr;
			
			ptr = (uint16_t *)((uint8_t *)info->_priv_ptr + info->_priv_var);
			x = (info->width + 15) >> 4;
			info->_priv_var += x << 4;
			do
			{
				plane0 = *ptr++;
				plane1 = *ptr++;
				plane2 = *ptr++;
				plane3 = *ptr++;
				plane4 = *ptr++;
				plane5 = *ptr++;
				plane6 = *ptr++;
				plane7 = *ptr++;
				for (bit = 0; bit < 16; bit++)
				{
					*buffer++ =
						((plane0 >> 15) & 1) |
						((plane1 >> 14) & 2) |
						((plane2 >> 13) & 4) |
						((plane3 >> 12) & 8) |
						((plane4 >> 11) & 16) |
						((plane5 >> 10) & 32) |
						((plane6 >> 9) & 64) |
						((plane7 >> 8) & 128);
					plane0 <<= 1;
					plane1 <<= 1;
					plane2 <<= 1;
					plane3 <<= 1;
					plane4 <<= 1;
					plane5 <<= 1;
					plane6 <<= 1;
					plane7 <<= 1;
				}
			} while (--x > 0);
		}
		break;

	case 16:
		{
			int x;
			const uint16_t *screen16;
			uint16_t color;
			
			screen16 = (const uint16_t *)info->_priv_ptr + info->_priv_var;
			x = (info->width + 15) & -16;
			info->_priv_var += x;
			do
			{
				color = *screen16++;
				*buffer++ = ((color >> 11) & 0x1f) << 3;
				*buffer++ = ((color >> 5) & 0x3f) << 2;
				*buffer++ = ((color) & 0x1f) << 3;
			} while (--x > 0);
		}
		break;
	}
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

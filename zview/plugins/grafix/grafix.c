#include "plugin.h"
#include "zvplugin.h"
/* only need the meta-information here */
#define LIBFUNC(a,b,c)
#define NOFUNC
#include "exports.h"

/*
http://downloads.atari-home.de/Public_Domain/Serie_ST-Computer/600-699/PDATS696.msa
*/


#ifdef PLUGIN_SLB
long __CDECL get_option(zv_int_t which)
{
	switch (which)
	{
	case OPTION_CAPABILITIES:
		return CAN_DECODE;
	case OPTION_EXTENSIONS:
		return (long)(EXTENSIONS);

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


typedef struct {
	uint32_t magic;				/* 'GRXP' */
	uint16_t version;			/* currently 1.01 */
	char info[22];				/* any text */
	uint16_t compress;			/* compression flag */
	uint16_t pic_w;				/* width */
	uint16_t pic_h;				/* height */
	uint16_t ncolors;			/* number of colors */
	uint16_t palette[256][3];	/* color info, in VDI order */
	uint16_t TC_normal;			/* reserved */
	uint32_t orig;				/* original size */
	uint32_t b_size[2];			/* size of the two compressed blocks */
} GRX_HEAD;

struct code {
	uint16_t code;
	uint8_t nextc;
	uint8_t thisc;
};


#if 0
static void init_codes(struct code *offsets)
{
	unsigned int i;
	
	for (i = 0; i <= 255; i++)
	{
		offsets[i].nextc = i;
		offsets[i].thisc = i;
		offsets[i].code = -1;
	}
}


static void getcode(int *bits, int codeBits, uint8_t **source, size_t *src_length, unsigned int *code, int *eof)
{
	int needed;
	uint32_t accum;
	int bitsCount;
	uint32_t tmp;
	
	needed = *bits + codeBits;
	accum = 0;
	bitsCount = 0;
	do
	{
		if (*src_length == 0)
		{
			*eof = -1;
			return;
		}
		tmp = *(*source)++;
		tmp <<= bitsCount;
		accum |= tmp;
		bitsCount += 8;
		(*src_length)--;
		needed -= 8;
	} while (needed > 0);
	if (needed != 0)
	{
		--(*source);
		++(*src_length);
	}
	accum >>= *bits;
	*bits = (*bits + codeBits) % 8;
	accum &= ~(-1u << codeBits);
	*code = (unsigned int)accum;
}


static void putcode(unsigned int code, struct code *offsets, uint8_t **dst, size_t *dst_length, size_t *decompressed)
{
	int d1;
	uint16_t thiscode;
	uint16_t prevcode;
	
	d1 = 0;
	thiscode = offsets[code].code;
	prevcode = 0xffff;
	while (d1 == 0 || thiscode != 0xffff)
	{
		if (thiscode == 0xffff)
		{
			uint16_t tmp;
			
			d1 = -1;
			tmp = thiscode;
			thiscode = prevcode;
			prevcode = tmp;
		} else
		{
			offsets[code].code = prevcode;
			prevcode = code;
			code = thiscode;
			thiscode = offsets[code].code;
		}
		if (d1 != 0)
		{
			if (*dst_length != 0)
			{
				*(*dst)++ = offsets[code].thisc;
				--(*dst_length);
			}
			++(*decompressed);
		}
	}
	offsets[code].code = prevcode;
}



static void unpack_grx(uint8_t *source, size_t src_length, uint8_t *dst, size_t dst_length, size_t *decompressed)
{
	struct code offsets[1024];
	unsigned int code;
	int bits;
	int eof;
	int codeBits;
	unsigned int codes;
	unsigned int prevcode;
	
	*decompressed = 0;
	if (src_length == 0)
		return;
	if (dst == NULL)
		dst_length = 0;
	bits = 0;
	codeBits = 9;
	codes = 258;
	init_codes(offsets);
	eof = 0;
	getcode(&bits, codeBits, &source, &src_length, &code, &eof);
	prevcode = code;
	putcode(prevcode, offsets, &dst, &dst_length, decompressed);
	if (eof == 0)
		getcode(&bits, codeBits, &source, &src_length, &code, &eof);
	while (eof == 0)
	{
		if (code == 256)
		{
			++codeBits;
		} else if (code == 257)
		{
			codeBits = 9;
			codes = 258;
			getcode(&bits, codeBits, &source, &src_length, &code, &eof);
			if (eof == 0)
			{
				prevcode = code;
				putcode(prevcode, offsets, &dst, &dst_length, decompressed);
			}
		} else
		{
			offsets[codes].code = prevcode;
			offsets[codes].nextc = offsets[prevcode].nextc;
			offsets[codes].thisc = offsets[code].nextc;
			codes++;
			prevcode = code;
			putcode(prevcode, offsets, &dst, &dst_length, decompressed);
		}
		getcode(&bits, codeBits, &source, &src_length, &code, &eof);
	}
}

#else

static int unpack_grx(uint8_t *src, size_t src_length, uint8_t *dst, size_t dst_length)
{
	uint8_t *dst_end = dst + dst_length;
	uint8_t *src_end = src + src_length;
	uint8_t *offsets[1024];
	int bits = 0;
	int bitsCount = 0;
	int codes = 258;
	int codeBits = 9;
	
	while (dst < dst_end)
	{
		int code;
		
		while (bitsCount < codeBits)
		{
			if (src >= src_end)
				return FALSE;
			bits |= *src++ << bitsCount;
			bitsCount += 8;
		}
		code = bits & ((1 << codeBits) - 1);
		bits >>= codeBits;
		bitsCount -= codeBits;
		switch (code)
		{
		case 256:
			if (codeBits == 10)
				return FALSE;
			codeBits = 10;
			break;
		case 257:
			codes = 258;
			codeBits = 9;
			break;
		default:
			if (codes >= 1024)
				return FALSE;
			offsets[codes] = dst;
			if (code < 256)
			{
				*dst++ = code;
			} else if (code >= codes)
			{
				return FALSE;
			} else
			{
				uint8_t *source = offsets[code];
				uint8_t *end = offsets[code + 1];
				if (dst + (end - source) >= dst_end)
					return FALSE;
				do
					*dst++ = *source++;
				while (source <= end);
			}
			codes++;
			break;
		}
	}
	return TRUE;
}
#endif



static int vdi2bios(int idx, int planes)
{
	int xbios;
	
	switch (idx)
	{
	case 1:
		xbios = (1 << planes) - 1;
		break;
	case 2:
		xbios = 1;
		break;
	case 3:
		xbios = 2;
		break;
	case 4:
		xbios = idx;
		break;
	case 5:
		xbios = 6;
		break;
	case 6:
		xbios = 3;
		break;
	case 7:
		xbios = 5;
		break;
	case 8:
		xbios = 7;
		break;
	case 9:
		xbios = 8;
		break;
	case 10:
		xbios = 9;
		break;
	case 11:
		xbios = 10;
		break;
	case 12:
		xbios = idx;
		break;
	case 13:
		xbios = 14;
		break;
	case 14:
		xbios = 11;
		break;
	case 15:
		xbios = 13;
		break;
	case 255:
		xbios = 15;
		break;
	case 0:
	default:
		xbios = idx;
		break;
	}
	return xbios;
}


boolean __CDECL reader_init(const char *name, IMGINFO info)
{
	int16_t handle;
	uint8_t *bmap;
	size_t file_size;
	size_t image_size;
	GRX_HEAD header;

	if ((handle = (int16_t)Fopen(name, FO_READ)) < 0)
	{
		RETURN_ERROR(EC_Fopen);
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (Fread(handle, sizeof(header), &header) != sizeof(header))
	{
		Fclose(handle);
		RETURN_ERROR(EC_Fread);
	}
	file_size -= sizeof(header);
	if (header.magic != 0x47525850L) /* 'GRXP' */
	{
		Fclose(handle);
		RETURN_ERROR(EC_FileId);
	}
	if (header.version != 0x101)
	{
		Fclose(handle);
		RETURN_ERROR(EC_HeaderVersion);
	}
	info->indexed_color = TRUE;
	switch (header.ncolors)
	{
	case 2:
		info->planes = 1;
		info->indexed_color = FALSE;
		break;
	case 4:
		info->planes = 2;
		break;
	case 16:
		info->planes = 4;
		break;
	case 256:
		info->planes = 8;
		break;
	default:
		Fclose(handle);
		RETURN_ERROR(EC_ColorCount);
	}

	image_size = (((size_t)header.pic_w + 15) >> 4) * 2 * info->planes * header.pic_h;
	if (image_size != header.orig)
	{
		Fclose(handle);
		RETURN_ERROR(EC_BitmapLength);
	}
	bmap = malloc(image_size);
	if (bmap == NULL)
	{
		Fclose(handle);
		RETURN_ERROR(EC_Malloc);
	}
	if (header.compress)
	{
		uint8_t *file;
		size_t size;
#if 0
		size_t decompressed;
#endif

		size = image_size >> 1;
		file = malloc(MAX(header.b_size[0], header.b_size[1]));
		if (file == NULL)
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Malloc);
		}
		if ((size_t) Fread(handle, header.b_size[0], file) != header.b_size[0])
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
#if 0
		unpack_grx(file, header.b_size[0], bmap, size, &decompressed);
		if (decompressed != size)
#else
		if (!unpack_grx(file, header.b_size[0], bmap, size))
#endif
		{
			free(file);
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_DecompError);
		}
		if ((size_t) Fread(handle, header.b_size[1], file) != header.b_size[1])
		{
			free(file);
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
#if 0
		unpack_grx(file, header.b_size[1], bmap + size, size, &decompressed);
		if (decompressed != size)
#else
		if (!unpack_grx(file, header.b_size[1], bmap + size, size))
#endif
		{
			free(file);
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_DecompError);
		}
		free(file);
		strcpy(info->compression, "LZW");
	} else
	{
		if ((size_t)Fread(handle, image_size, bmap) != image_size)
		{
			free(bmap);
			Fclose(handle);
			RETURN_ERROR(EC_Fread);
		}
		strcpy(info->compression, "None");
	}
	Fclose(handle);

	info->width = header.pic_w;
	info->height = header.pic_h;
	info->colors = 1L << info->planes;
	info->components = 1;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = 1;						/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	strcpy(info->info, "Grafix");

	info->_priv_var = 0;
	info->_priv_ptr = bmap;

	{	
		int i;

		for (i = 0; i < (int)info->colors; i++)
		{
			info->palette[vdi2bios(i, info->planes)].red = ((((long)header.palette[i][0] << 8) - header.palette[i][0]) + 500) / 1000;
			info->palette[vdi2bios(i, info->planes)].green = ((((long)header.palette[i][1] << 8) - header.palette[i][1]) + 500) / 1000;
			info->palette[vdi2bios(i, info->planes)].blue = ((((long)header.palette[i][2] << 8) - header.palette[i][2]) + 500) / 1000;
		}
	}

	RETURN_SUCCESS();
}


boolean __CDECL reader_read(IMGINFO info, uint8_t *buffer)
{
	switch (info->planes)
	{
	case 1:
		{
			uint8_t *bmap = info->_priv_ptr;
			int16_t byte;
			int x;
			
			bmap += info->_priv_var;
			x = ((info->width + 15) >> 4) << 1;
			info->_priv_var += x;
			do
			{
				byte = *bmap++;
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
			int bit;
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
	}

	RETURN_SUCCESS();
}


void __CDECL reader_quit(IMGINFO info)
{
	void *p = info->_priv_ptr;
	info->_priv_ptr = NULL;
	free(p);
}


void __CDECL reader_get_txt(IMGINFO info, txt_data *txtdata)
{
	(void)info;
	(void)txtdata;
}

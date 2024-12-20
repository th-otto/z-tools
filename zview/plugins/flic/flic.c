/*
See https://www.compuphase.com/flic.htm
*/

#include "plugin.h"
#include "zvplugin.h"
#include "exports.h"
#define NF_DEBUG 0
#include "nfdebug.h"

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


typedef struct {
	uint32_t size;          /* Size of FLIC including this header */
	uint16_t type;          /* File type 0xAF11, 0xAF12, 0xAF30, 0xAF44, ... */
	uint16_t frames;        /* Number of frames in first segment */
	uint16_t width;         /* FLIC width in pixels */
	uint16_t height;        /* FLIC height in pixels */
	uint16_t depth;         /* Bits per pixel (usually 8) */
	uint16_t flags;         /* Set to zero or to three */
	uint32_t speed;         /* Delay between frames */
	uint16_t reserved1;     /* Set to zero */
	uint32_t created;       /* Date of FLIC creation (FLC only) */
	uint32_t creator;       /* Serial number or compiler id (FLC only) */
	uint32_t updated;       /* Date of FLIC update (FLC only) */
	uint32_t updater;       /* Serial number (FLC only), see creator */
	uint16_t aspect_dx;     /* Width of square rectangle (FLC only) */
	uint16_t aspect_dy;     /* Height of square rectangle (FLC only) */
	uint16_t ext_flags;     /* EGI: flags for specific EGI extensions */
	uint16_t keyframes;     /* EGI: key-image frequency */
	uint16_t totalframes;   /* EGI: total number of frames (segments) */
	uint32_t req_memory;    /* EGI: maximum chunk size (uncompressed) */
	uint16_t max_regions;   /* EGI: max. number of regions in a CHK_REGION chunk */
	uint16_t transp_num;    /* EGI: number of transparent levels */
	char reserved2[24];     /* Set to zero */
	uint32_t oframe1;       /* Offset to frame 1 (FLC only) */
	uint32_t oframe2;       /* Offset to frame 2 (FLC only) */
	char reserved3[40];     /* Set to zero */
} FLIC_HEADER;

typedef struct {
	uint32_t size;          /* Size of the chunk, including subchunks */
	uint16_t type;          /* Chunk type: 0xF100 */
	uint16_t chunks;        /* Number of subchunks */
	char reserved[8];       /* Reserved, set to 0 */
} PREFIX_HDR;

typedef struct {
	uint32_t size;          /* Size of the chunk, always 64 */
	uint16_t type;          /* Chunk type: 3 */
	int16_t center_x;       /* Coordinates of the cel centre or origin */
	int16_t center_y;
	uint16_t stretch_x;     /* Stretch amounts */
	uint16_t stretch_y;
	uint16_t rot_x;         /* Rotation in x-axis (always 0) */
	uint16_t rot_y;         /* Rotation in y-axis (always 0) */
	uint16_t rot_z;         /* z-axis rotation, 0-5760=0-360 degrees */
	uint16_t cur_frame;     /* Current frame in cel file */
	char reserved1[2];      /* Reserved, set to 0 */
	uint16_t transparent;   /* Transparent colour index */
	uint16_t overlay[16];   /* Frame overlay numbers */
	char reserved2[6];      /* Reserved, set to 0 */
} CEL_DATA;

typedef struct {
	uint32_t size;          /* Size of the chunk, including subchunks */
	uint16_t type;          /* Chunk type: 0xF1FA */
	uint16_t chunks;        /* Number of subchunks */
	uint16_t delay;         /* Delay in milliseconds */
	int16_t reserved;       /* Always zero */
	uint16_t width;         /* Frame width override (if non-zero) */
	uint16_t height;        /* Frame height override (if non-zero) */
} FRAME_TYPE;

#define PREFIX_CHUNK  0xf100
#define SCRIPT_CHUNK  0xf1e0
#define FRAME_CHUNK   0xf1fa
#define SEGMENT_CHUNK 0xf1fb
#define HUFFMAN_CHUNK 0xf1fc


typedef struct {
	uint32_t size;          /* Size of the chunk, including subchunks */
	uint16_t type;          /* Chunk type */
} CHUNK_HDR;

static uint8_t const color64_table[64] = {
	0x00, 0x04,
	0x08, 0x0c,
	0x10, 0x14,
	0x18, 0x1c,
	0x20, 0x24,
	0x28, 0x2c,
	0x30, 0x34,
	0x38, 0x3c,
	0x40, 0x44,
	0x48, 0x4c,
	0x50, 0x55,
	0x59, 0x5d,
	0x61, 0x65,
	0x69, 0x6d,
	0x71, 0x75,
	0x79, 0x7d,
	0x81, 0x85,
	0x89, 0x8d,
	0x91, 0x95,
	0x99, 0x9d,
	0xa1, 0xa5,
	0xaa, 0xae,
	0xb2, 0xb6,
	0xba, 0xbe,
	0xc2, 0xc6,
	0xca, 0xce,
	0xd2, 0xd6,
	0xda, 0xde,
	0xe2, 0xe6,
	0xea, 0xee,
	0xf2, 0xf6,
	0xfa, 0xff
};

static uint8_t const color_table[256 * 3] = {
	0x00, 0x00, 0x00,
	0x00, 0x00, 0xaa,
	0x00, 0xaa, 0x00,
	0x00, 0xaa, 0xaa,
	0xaa, 0x00, 0x00,
	0xaa, 0x00, 0xaa,
	0xaa, 0x55, 0x00,
	0xaa, 0xaa, 0xaa,
	0x55, 0x55, 0x55,
	0x55, 0x55, 0xff,
	0x55, 0xff, 0x55,
	0x55, 0xff, 0xff,
	0xff, 0x55, 0x55,
	0xff, 0x55, 0xff,
	0xff, 0xff, 0x55,
	0xff, 0xff, 0xff,
	0x00, 0x00, 0x00,
	0x14, 0x14, 0x14,
	0x20, 0x20, 0x20,
	0x2c, 0x2c, 0x2c,
	0x38, 0x38, 0x38,
	0x45, 0x45, 0x45,
	0x51, 0x51, 0x51,
	0x61, 0x61, 0x61,
	0x71, 0x71, 0x71,
	0x82, 0x82, 0x82,
	0x92, 0x92, 0x92,
	0xa2, 0xa2, 0xa2,
	0xb6, 0xb6, 0xb6,
	0xcb, 0xcb, 0xcb,
	0xe3, 0xe3, 0xe3,
	0xff, 0xff, 0xff,
	0x00, 0x00, 0xff,
	0x41, 0x00, 0xff,
	0x7d, 0x00, 0xff,
	0xbe, 0x00, 0xff,
	0xff, 0x00, 0xff,
	0xff, 0x00, 0xbe,
	0xff, 0x00, 0x7d,
	0xff, 0x00, 0x41,
	0xff, 0x00, 0x00,
	0xff, 0x41, 0x00,
	0xff, 0x7d, 0x00,
	0xff, 0xbe, 0x00,
	0xff, 0xff, 0x00,
	0xbe, 0xff, 0x00,
	0x7d, 0xff, 0x00,
	0x41, 0xff, 0x00,
	0x00, 0xff, 0x00,
	0x00, 0xff, 0x41,
	0x00, 0xff, 0x7d,
	0x00, 0xff, 0xbe,
	0x00, 0xff, 0xff,
	0x00, 0xbe, 0xff,
	0x00, 0x7d, 0xff,
	0x00, 0x41, 0xff,
	0x7d, 0x7d, 0xff,
	0x9e, 0x7d, 0xff,
	0xbe, 0x7d, 0xff,
	0xdf, 0x7d, 0xff,
	0xff, 0x7d, 0xff,
	0xff, 0x7d, 0xdf,
	0xff, 0x7d, 0xbe,
	0xff, 0x7d, 0x9e,
	0xff, 0x7d, 0x7d,
	0xff, 0x9e, 0x7d,
	0xff, 0xbe, 0x7d,
	0xff, 0xdf, 0x7d,
	0xff, 0xff, 0x7d,
	0xdf, 0xff, 0x7d,
	0xbe, 0xff, 0x7d,
	0x9e, 0xff, 0x7d,
	0x7d, 0xff, 0x7d,
	0x7d, 0xff, 0x9e,
	0x7d, 0xff, 0xbe,
	0x7d, 0xff, 0xdf,
	0x7d, 0xff, 0xff,
	0x7d, 0xdf, 0xff,
	0x7d, 0xbe, 0xff,
	0x7d, 0x9e, 0xff,
	0xb6, 0xb6, 0xff,
	0xc7, 0xb6, 0xff,
	0xdb, 0xb6, 0xff,
	0xeb, 0xb6, 0xff,
	0xff, 0xb6, 0xff,
	0xff, 0xb6, 0xeb,
	0xff, 0xb6, 0xdb,
	0xff, 0xb6, 0xc7,
	0xff, 0xb6, 0xb6,
	0xff, 0xc7, 0xb6,
	0xff, 0xdb, 0xb6,
	0xff, 0xeb, 0xb6,
	0xff, 0xff, 0xb6,
	0xeb, 0xff, 0xb6,
	0xdb, 0xff, 0xb6,
	0xc7, 0xff, 0xb6,
	0xb6, 0xdf, 0xb6,
	0xb6, 0xff, 0xc7,
	0xb6, 0xff, 0xdb,
	0xb6, 0xff, 0xeb,
	0xb6, 0xff, 0xff,
	0xb6, 0xeb, 0xff,
	0xb6, 0xdb, 0xff,
	0xb6, 0xc7, 0xff,
	0x00, 0x00, 0x71,
	0x1c, 0x00, 0x71,
	0x38, 0x00, 0x71,
	0x55, 0x00, 0x71,
	0x71, 0x00, 0x71,
	0x71, 0x00, 0x55,
	0x71, 0x00, 0x38,
	0x71, 0x00, 0x1c,
	0x71, 0x00, 0x00,
	0x71, 0x1c, 0x00,
	0x71, 0x38, 0x00,
	0x71, 0x55, 0x00,
	0x71, 0x71, 0x00,
	0x55, 0x71, 0x00,
	0x38, 0x71, 0x00,
	0x1c, 0x71, 0x00,
	0x00, 0x71, 0x00,
	0x00, 0x71, 0x1c,
	0x00, 0x71, 0x38,
	0x00, 0x71, 0x55,
	0x00, 0x71, 0x71,
	0x00, 0x55, 0x71,
	0x00, 0x38, 0x71,
	0x00, 0x1c, 0x71,
	0x38, 0x38, 0x71,
	0x45, 0x38, 0x71,
	0x55, 0x38, 0x71,
	0x61, 0x38, 0x71,
	0x71, 0x38, 0x71,
	0x71, 0x38, 0x61,
	0x71, 0x38, 0x55,
	0x71, 0x38, 0x45,
	0x71, 0x38, 0x38,
	0x71, 0x45, 0x38,
	0x71, 0x55, 0x38,
	0x71, 0x61, 0x38,
	0x71, 0x71, 0x38,
	0x61, 0x71, 0x38,
	0x55, 0x71, 0x38,
	0x45, 0x71, 0x38,
	0x38, 0x71, 0x38,
	0x38, 0x71, 0x45,
	0x38, 0x71, 0x55,
	0x38, 0x71, 0x61,
	0x38, 0x71, 0x71,
	0x38, 0x61, 0x71,
	0x38, 0x55, 0x71,
	0x38, 0x45, 0x71,
	0x51, 0x51, 0x71,
	0x59, 0x51, 0x71,
	0x61, 0x51, 0x71,
	0x69, 0x51, 0x71,
	0x71, 0x51, 0x71,
	0x71, 0x51, 0x69,
	0x71, 0x51, 0x61,
	0x71, 0x51, 0x59,
	0x71, 0x51, 0x51,
	0x71, 0x59, 0x51,
	0x71, 0x61, 0x51,
	0x71, 0x69, 0x51,
	0x71, 0x71, 0x51,
	0x69, 0x71, 0x51,
	0x61, 0x71, 0x51,
	0x59, 0x71, 0x51,
	0x51, 0x71, 0x51,
	0x51, 0x71, 0x59,
	0x51, 0x71, 0x61,
	0x51, 0x71, 0x69,
	0x51, 0x71, 0x71,
	0x51, 0x69, 0x71,
	0x51, 0x61, 0x71,
	0x51, 0x59, 0x71,
	0x00, 0x00, 0x41,
	0x10, 0x00, 0x41,
	0x20, 0x00, 0x41,
	0x30, 0x00, 0x41,
	0x41, 0x00, 0x41,
	0x41, 0x00, 0x30,
	0x41, 0x00, 0x20,
	0x41, 0x00, 0x10,
	0x41, 0x00, 0x00,
	0x41, 0x10, 0x00,
	0x41, 0x20, 0x00,
	0x41, 0x30, 0x00,
	0x41, 0x41, 0x00,
	0x30, 0x41, 0x00,
	0x20, 0x41, 0x00,
	0x10, 0x41, 0x00,
	0x00, 0x41, 0x00,
	0x00, 0x41, 0x10,
	0x00, 0x41, 0x20,
	0x00, 0x41, 0x30,
	0x00, 0x41, 0x41,
	0x00, 0x30, 0x41,
	0x00, 0x20, 0x41,
	0x00, 0x10, 0x41,
	0x20, 0x20, 0x41,
	0x28, 0x20, 0x41,
	0x30, 0x20, 0x41,
	0x38, 0x20, 0x41,
	0x41, 0x20, 0x41,
	0x41, 0x20, 0x38,
	0x41, 0x20, 0x30,
	0x41, 0x20, 0x28,
	0x41, 0x20, 0x20,
	0x41, 0x28, 0x20,
	0x41, 0x30, 0x20,
	0x41, 0x38, 0x20,
	0x41, 0x41, 0x20,
	0x38, 0x41, 0x20,
	0x30, 0x41, 0x20,
	0x28, 0x41, 0x20,
	0x20, 0x41, 0x20,
	0x20, 0x41, 0x28,
	0x20, 0x41, 0x30,
	0x20, 0x41, 0x38,
	0x20, 0x41, 0x41,
	0x20, 0x38, 0x41,
	0x20, 0x30, 0x41,
	0x20, 0x28, 0x41,
	0x2c, 0x2c, 0x41,
	0x30, 0x2c, 0x41,
	0x34, 0x2c, 0x41,
	0x3c, 0x2c, 0x41,
	0x41, 0x2c, 0x41,
	0x41, 0x2c, 0x3c,
	0x41, 0x2c, 0x34,
	0x41, 0x2c, 0x30,
	0x41, 0x2c, 0x2c,
	0x41, 0x30, 0x2c,
	0x41, 0x34, 0x2c,
	0x41, 0x3c, 0x2c,
	0x41, 0x41, 0x2c,
	0x3c, 0x41, 0x2c,
	0x34, 0x41, 0x2c,
	0x30, 0x41, 0x2c,
	0x2c, 0x41, 0x2c,
	0x2c, 0x41, 0x30,
	0x2c, 0x41, 0x34,
	0x2c, 0x41, 0x3c,
	0x2c, 0x41, 0x41,
	0x2c, 0x3c, 0x41,
	0x2c, 0x34, 0x41,
	0x2c, 0x30, 0x41,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00,
	0x00, 0x00, 0x00
};

/* FIXME: statics */
static uint32_t frame_positions[ZVIEW_MAX_IMAGES];
static size_t image_size;
static uint16_t delay;


static uint16_t swap16(uint16_t w)
{
	return (w >> 8) | (w << 8);
}


static uint32_t swap32(uint32_t l)
{
	return ((l >> 24) & 0xffL) | ((l << 8) & 0xff0000L) | ((l >> 8) & 0xff00L) | ((l << 24) & 0xff000000UL);
}


static uint16_t read16(int16_t handle)
{
	uint16_t w;
	
	if (Fread(handle, sizeof(w), &w) != sizeof(w))
		return (uint16_t)-1;
	return swap16(w);
}


static int read_flic_header(int16_t handle, FLIC_HEADER *flic_header)
{
	if (Fread(handle, sizeof(*flic_header), flic_header) != sizeof(*flic_header))
		return FALSE;
	flic_header->size = swap32(flic_header->size);
	flic_header->type = swap16(flic_header->type);
	flic_header->frames = swap16(flic_header->frames);
	flic_header->width = swap16(flic_header->width);
	flic_header->height = swap16(flic_header->height);
	flic_header->depth = swap16(flic_header->depth);
	flic_header->flags = swap16(flic_header->flags);
	flic_header->speed = swap32(flic_header->speed);
	flic_header->reserved1 = swap16(flic_header->reserved1);
	flic_header->created = swap32(flic_header->created);
	/* leave creator alone: it is a 4byte ASCII signature */
	flic_header->updated = swap32(flic_header->updated);
	/* leave updater alone: it is a 4byte ASCII signature */
	flic_header->aspect_dx = swap16(flic_header->aspect_dx);
	flic_header->aspect_dy = swap16(flic_header->aspect_dy);
	flic_header->ext_flags = swap16(flic_header->ext_flags);
	flic_header->keyframes = swap16(flic_header->keyframes);
	flic_header->totalframes = swap16(flic_header->totalframes);
	flic_header->req_memory = swap32(flic_header->req_memory);
	flic_header->max_regions = swap16(flic_header->max_regions);
	flic_header->transp_num = swap16(flic_header->transp_num);
	flic_header->oframe1 = swap32(flic_header->oframe1);
	flic_header->oframe2 = swap32(flic_header->oframe2);
	return TRUE;
}


static int read_frame_header(int16_t handle, FRAME_TYPE *frame_header)
{
	if (Fread(handle, sizeof(*frame_header), frame_header) != sizeof(*frame_header))
		return FALSE;
	frame_header->size = swap32(frame_header->size);
	frame_header->type = swap16(frame_header->type);
	frame_header->chunks = swap16(frame_header->chunks);
	frame_header->delay = swap16(frame_header->delay);
	frame_header->reserved = swap16(frame_header->reserved);
	frame_header->width = swap16(frame_header->width);
	frame_header->height = swap16(frame_header->height);
	return TRUE;
}


static int read_chunk_header(int16_t handle, CHUNK_HDR *chunk_header)
{
	if (Fread(handle, sizeof(*chunk_header), chunk_header) != sizeof(*chunk_header))
		return FALSE;
	chunk_header->size = swap32(chunk_header->size);
	chunk_header->type = swap16(chunk_header->type);
	return TRUE;
}


static int read_color_256(int16_t handle, IMGINFO info)
{
	int skip;
	uint8_t c[2];
	uint16_t num_packets;
	uint16_t copy_count;
	uint8_t rgb[3];

	if ((num_packets = read16(handle)) == (uint16_t)-1)
		return FALSE;
	skip = 0;
	while (num_packets > 0)
	{
		if (Fread(handle, sizeof(c), c) != sizeof(c))
			return FALSE;
		skip += c[0];
		copy_count = c[1];
		if (copy_count == 0)
			copy_count = 256;
		while (copy_count > 0)
		{
			if (Fread(handle, sizeof(rgb), rgb) != sizeof(rgb))
				return FALSE;
			if (skip < 256)
			{
				info->palette[skip].red = rgb[0];
				info->palette[skip].green = rgb[1];
				info->palette[skip].blue = rgb[2];
			}
			skip++;
			copy_count--;
		}
		num_packets--;
	}
	return TRUE;
}


static int read_delta_flc(int16_t handle, IMGINFO info)
{
	uint8_t c[2];
	int16_t type;
	uint16_t opcode;
	int16_t count;
	uint16_t num_lines;
	size_t pos;
	uint8_t *bmap = info->_priv_ptr;

	pos = 0;
	if ((num_lines = read16(handle)) == (uint16_t)-1)
		return FALSE;
	do
	{
		if (Fread(handle, sizeof(opcode), &opcode) != sizeof(opcode))
			return FALSE;
		count = swap16(opcode);
		type = (count >> 14) & 3;
		switch (type)
		{
		case 0:
			{
				size_t linepos;
				int8_t rle_count;
	
				linepos = pos;
				for (; count > 0; count++)
				{
					if (Fread(handle, sizeof(c), c) != sizeof(c))
						return FALSE;
					rle_count = c[1];
					linepos += c[0];
					if (rle_count < 0)
					{
						rle_count = -rle_count;
						if (Fread(handle, sizeof(c), c) != sizeof(c))
							return FALSE;
						while (rle_count > 0)
						{
							bmap[linepos++] = c[0];
							bmap[linepos++] = c[1];
							rle_count--;
						}
					} else
					{
						if ((size_t)Fread(handle, (size_t)rle_count * 2, &bmap[linepos]) != (size_t)rle_count * 2)
							return FALSE;
						linepos += (size_t)rle_count * 2;
					}
				}
				num_lines--;
				pos += info->width;
			}
			break;
		case 1:
			nf_debugprintf(("bit error at DTA_LC\n"));
			return FALSE;
		case 2:
			bmap[pos + (info->width - 1)] = count & 0xff;
			pos += info->width;
			break;
		case 3:
			count = -count;
			pos += count * (size_t)info->width;
			break;
		}
	} while (num_lines > 0);
	return TRUE;
}


static int read_color_64(int16_t handle, IMGINFO info)
{
	int skip;
	uint8_t c[2];
	uint16_t num_packets;
	uint16_t count;
	uint8_t rgb[3];

	if ((num_packets = read16(handle)) == (uint16_t)-1)
		return FALSE;
	skip = 0;
	while (num_packets > 0)
	{
		if (Fread(handle, sizeof(c), c) != sizeof(c))
			return FALSE;
		skip += c[0];
		count = c[1];
		if (count == 0)
			count = 256;
		while (count > 0)
		{
			if (Fread(handle, sizeof(rgb), rgb) != sizeof(rgb))
				return FALSE;
			if (skip < 256)
			{
				info->palette[skip].red = color64_table[rgb[0] & 63];
				info->palette[skip].green = color64_table[rgb[1] & 63];
				info->palette[skip].blue = color64_table[rgb[2] & 63];
			}
			skip++;
			count--;
		}
		num_packets--;
	}
	return TRUE;
}


static int read_delta_fli(int16_t handle, IMGINFO info)
{
	size_t pos;
	size_t linepos;
	int8_t rle_count;
	uint8_t num_packets;
	uint8_t c[2];
	uint16_t skip;
	uint16_t num_lines;
	uint8_t *bmap = info->_priv_ptr;

	if ((skip = read16(handle)) == (uint16_t)-1)
		return FALSE;
	pos = skip;
	pos *= info->width;
	if ((num_lines = read16(handle)) == (uint16_t)-1)
		return FALSE;
	do
	{
		if (Fread(handle, sizeof(num_packets), &num_packets) != sizeof(num_packets))
			return FALSE;
		linepos = pos;
		for (; num_packets > 0; num_packets--)
		{
			if (Fread(handle, sizeof(c), c) != sizeof(c))
				return FALSE;
			rle_count = c[1];
			linepos += c[0];
			if (rle_count < 0)
			{
				uint8_t val;

				rle_count = -rle_count;
				if (Fread(handle, sizeof(val), &val) != sizeof(val))
					return FALSE;
				while (rle_count > 0)
				{
					bmap[linepos++] = val;
					rle_count--;
				}
			} else
			{
				if ((size_t)Fread(handle, (size_t)rle_count, &bmap[linepos]) != (size_t)rle_count)
					return FALSE;
				linepos += rle_count;
			}
		}
		num_lines--;
		pos += info->width;
	} while (num_lines > 0);
	return TRUE;
}


static int read_image(int16_t handle, IMGINFO info)
{
	size_t pos;
	uint16_t y;
	uint16_t width;
	int8_t rle_count;
	uint8_t packet_count;
	uint8_t *bmap = info->_priv_ptr;

	pos = 0;
	for (y = 0; y < info->height; y++)
	{
		if (Fread(handle, sizeof(packet_count), &packet_count) != sizeof(packet_count)) /* packet count ignored */
			return FALSE;
		width = info->width;
		do
		{
			if (Fread(handle, sizeof(rle_count), &rle_count) != sizeof(rle_count))
				return FALSE;
			if (rle_count <= 0)
			{
				rle_count = -rle_count;
				if (rle_count > width)
					rle_count = width;
				if ((size_t)Fread(handle, (size_t)rle_count, &bmap[pos]) != (size_t)rle_count)
					return FALSE;
				pos += rle_count;
				width -= rle_count;
			} else
			{
				uint8_t val;
				
				if (Fread(handle, sizeof(val), &val) != sizeof(val))
					return FALSE;
				if (rle_count > width)
					rle_count = width;
				width -= rle_count;
				while (rle_count > 0)
				{
					bmap[pos++] = val;
					rle_count--;
				}
			}
		} while (width > 0);
	}
	return TRUE;
}


static int read_image16(int16_t handle, IMGINFO info)
{
	size_t pos;
	uint16_t width;
	uint16_t y;
	int8_t rle_count;
	uint8_t packet_count;
	uint16_t *bmap16;
	
	bmap16 = (uint16_t *)info->_priv_ptr;
	pos = 0;
	for (y = 0; y < info->height; y++)
	{
		if (Fread(handle, sizeof(packet_count), &packet_count) != sizeof(packet_count)) /* packet count ignored */
			return FALSE;
		width = info->width;
		do
		{
			if (Fread(handle, sizeof(rle_count), &rle_count) != sizeof(rle_count))
				return FALSE;
			if (rle_count <= 0)
			{
				rle_count = -rle_count;
				if (rle_count > width)
					rle_count = width;
				if ((size_t)Fread(handle, (size_t)rle_count * 2, &bmap16[pos]) != (size_t)rle_count * 2)
					return FALSE;
				pos += rle_count;
				width -= rle_count;
			} else
			{
				uint16_t val;
				
				Fread(handle, sizeof(val), &val);
				if (rle_count > width)
					rle_count = width;
				width -= rle_count;
				while (rle_count > 0)
				{
					bmap16[pos] = val;
					pos++;
					rle_count--;
				}
			}
		} while (width > 0);
	}
	return TRUE;
}


static int read_delta_flc16(int16_t handle, IMGINFO info)
{
	int16_t type;
	uint16_t opcode;
	uint8_t c[2];
	size_t pos;
	uint16_t num_lines;
	int16_t count;
	uint16_t *bmap16;
	
	bmap16 = (uint16_t *)info->_priv_ptr;
	pos = 0;
	if ((num_lines = read16(handle)) == (uint16_t)-1)
		return FALSE;
	do
	{
		if (Fread(handle, sizeof(opcode), &opcode) != sizeof(opcode))
			return FALSE;
		count = swap16(opcode);
		type = (count >> 14) & 3;
		switch (type)
		{
		case 0:
			{
				size_t linepos;
				int8_t rle_count;
	
				linepos = pos;
				for (; count > 0; count--)
				{
					if (Fread(handle, sizeof(c), c) != sizeof(c))
						return FALSE;
					rle_count = c[1];
					linepos += c[0];
					if (rle_count < 0)
					{
						uint16_t val;
	
						rle_count = -rle_count;
						if (Fread(handle, sizeof(val), &val) != sizeof(val))
							return FALSE;
						while (rle_count > 0)
						{
							bmap16[linepos++] = val;
							rle_count--;
						}
					} else
					{
						if ((size_t)Fread(handle, (size_t)rle_count * 2, &bmap16[linepos]) != (size_t)rle_count * 2)
							return FALSE;
						linepos += (long)rle_count;
					}
				}
				num_lines--;
				pos += info->width;
			}
			break;
		case 1:
			nf_debugprintf(("bit error at DTA_LC\n"));
			return FALSE;
		case 2:
			{
				uint16_t val;
	
				val = (count << 8) | (count & 0xff);
				bmap16[pos + info->width - 1] = val;
				pos += info->width;
			}
			break;
		case 3:
			count = -count;
			pos += count * (size_t)info->width;
			break;
		}
	} while (num_lines > 0);
	return TRUE;
}


static int read_frame(int16_t handle, IMGINFO info)
{
	uint16_t chunk;
	FRAME_TYPE frame_header;
	CHUNK_HDR chunk_header;

	if (read_frame_header(handle, &frame_header) == FALSE)
		return FALSE;
	nf_debugprintf(("frm_type=0x%x subcnt=%u size=%lu\n", frame_header.type, frame_header.chunks, (unsigned long)frame_header.size));
	if (frame_header.chunks == 0 && frame_header.size == sizeof(FRAME_TYPE))
		return TRUE;
	
	switch (frame_header.type)
	{
	case PREFIX_CHUNK:
		break;
	case FRAME_CHUNK:
		for (chunk = 0; chunk < frame_header.chunks; chunk++)
		{
			if (read_chunk_header(handle, &chunk_header) == FALSE)
				return FALSE;
			nf_debugprintf(("frame %u: chunk_type=%u pos=0x%lx size=0x%lx\n",
				info->page_wanted, chunk_header.type,
				(unsigned long)Fseek(0, handle, SEEK_CUR) - sizeof(chunk_header),
				(unsigned long)chunk_header.size));
			switch (chunk_header.type)
			{
			case 11: /* COLOR_64 */
				if (read_color_64(handle, info) == FALSE)
					return FALSE;
				break;
			case 4: /* COLOR_256 */
			case 36: /* KEY_PAL */
				if (read_color_256(handle, info) == FALSE)
					return FALSE;
				break;
			case 7: /* DELTA_FLC (FLI_SS2) */
				nf_debugprintf(("frame %u: delta_lfc\n", info->page_wanted));
				if (read_delta_flc(handle, info) == FALSE)
					return FALSE;
				break;
			case 12: /* DELTA_FLI (FLI_LC) */
				nf_debugprintf(("frame %u: delta_fli\n", info->page_wanted));
				if (read_delta_fli(handle, info) == FALSE)
					return FALSE;
				break;
			case 15: /* BYTE_RUN (FLI_BRUN) */
				nf_debugprintf(("frame %u: byte_run\n", info->page_wanted));
				if (read_image(handle, info) == FALSE)
					return FALSE;
				break;
			case 25: /* DTA_BRUN */
				nf_debugprintf(("frame %u: dta_brun\n", info->page_wanted));
				switch (info->planes)
				{
				case 15:
				case 16:
					if (read_image16(handle, info) == FALSE)
						return FALSE;
					break;
				default:
					nf_debugprintf(("DTA_BRUN not supported at color depth=%u\n", info->planes));
					return FALSE;
				}
				break;
			case 27: /* DTA_LC */
				nf_debugprintf(("frame %u: dta_lfc\n", info->page_wanted));
				switch (info->planes)
				{
				case 15:
				case 16:
					if (read_delta_flc16(handle, info) == FALSE)
						return FALSE;
					break;
				default:
					nf_debugprintf(("DTA_LC not supported at color depth=%u\n", info->planes));
					return FALSE;
				}
				break;
			case 16: /* FLI_COPY */
			case 26: /* DTA_COPY */
				if ((size_t)Fread(handle, image_size, info->_priv_ptr) != image_size)
					return FALSE;
				break;
			case 13: /* BLACK */
				memset(info->_priv_ptr, 0, image_size);
				break;
			case 18: /* PSTAMP */
			case 38: /* WAVE */
			case 39: /* USERSTRING */
			case 41: /* LABELEX */
			case 35: /* KEY_IMAGE */
				Fseek(chunk_header.size - sizeof(chunk_header), handle, SEEK_CUR);
				break;
			default:
				nf_debugprintf(("unknown sub chunk type=%u\n", chunk_header.type));
				return FALSE;
			}
		}
		break;
	default:
		nf_debugprintf(("unknown frame type=0x%x\n", frame_header.type));
		return FALSE;
	}

	return TRUE;
}


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
	uint32_t file_size;
	uint16_t num_frames;
	uint16_t real_frames;
	uint16_t i;
	int16_t handle;
	uint8_t *bmap;
	FLIC_HEADER flic_header;
	FRAME_TYPE frame_header;

	if ((handle = (int16_t) Fopen(name, FO_READ)) < 0)
	{
		return FALSE;
	}
	file_size = Fseek(0, handle, SEEK_END);
	Fseek(0, handle, SEEK_SET);

	if (read_flic_header(handle, &flic_header) == FALSE)
	{
		return FALSE;
	}
	switch (flic_header.type)
	{
	case 0xaf11:
	case 0xaf12:
	case 0xaf44:
		break;
	default:
		nf_debugprintf(("unsupported flic format=0x%x\n", flic_header.type));
		Fclose(handle);
		return FALSE;
	}
	if (file_size < flic_header.size)
	{
		nf_debugprintf(("file size < header size - short file=%lu\n", (unsigned long)flic_header.size));
		Fclose(handle);
		return FALSE;
	}
	
	info->width = flic_header.width;
	info->height = flic_header.height;
	info->planes = flic_header.depth;
	
	num_frames = flic_header.frames + 1;
	switch (info->planes)
	{
	case 8:
		image_size = (size_t)info->width * info->height;
		break;
	case 15:
	case 16:
		image_size = (size_t)info->width * info->height * 2;
		break;
	case 24:
		image_size = (size_t)info->width * info->height * 3;
		break;
	default:
		nf_debugprintf(("unsupported depth %u\n", info->planes));
		Fclose(handle);
		return FALSE;
	}
	bmap = malloc(image_size + 256);
	if (bmap == NULL)
	{
		Fclose(handle);
		return FALSE;
	}
	
	real_frames = 0;
	for (i = 0; i < num_frames; i++)
	{
		uint32_t pos;
		
		pos = Fseek(0, handle, SEEK_CUR);
		if (read_frame_header(handle, &frame_header) == FALSE)
		{
			free(bmap);
			Fclose(handle);
			return FALSE;
		}
		if (frame_header.type == FRAME_CHUNK)
		{
			nf_debugprintf(("offsets[%u]=%lu size=%lu\n", i, (unsigned long)pos, (unsigned long)frame_header.size));
			if (real_frames < ZVIEW_MAX_IMAGES)
				frame_positions[real_frames++] = pos;
		}
		if (frame_header.size & 1)
		{
			uint8_t c;
			
			nf_debugprintf(("!odd frm_size=%lu\n", (unsigned long)frame_header.size));
			/* check type field of next frame */
			Fseek(frame_header.size - 12, handle, SEEK_CUR);
			if (Fread(handle, sizeof(c), &c) != sizeof(c))
			{
				free(bmap);
				Fclose(handle);
				return FALSE;
			}
			if (c != 0xfa)
			{
				frame_header.size++;
				nf_debugprintf(("frame size rounded up\n"));
			} else
			{
				nf_debugprintf(("odd frame size left as is\n"));
			}
			Fseek(pos + sizeof(frame_header), handle, SEEK_SET);
		}
		Fseek(frame_header.size - sizeof(frame_header), handle, SEEK_CUR);
	}
	
	if (flic_header.type == 0xaf11)
	{
		delay = (uint16_t)flic_header.speed * 3;
	} else
	{
		delay = ((uint16_t)flic_header.speed * 20 + 5) / 50; /* / 2.5 */
	}
	if (delay == 0)
		delay = 10;
	memset(bmap, 0, image_size);
	if (flic_header.depth <= 8)
	{
		int j;
		
		j = 0;
		for (i = 0; i < 256; i++)
		{
			info->palette[i].red = color_table[j];
			info->palette[i].green = color_table[j + 1];
			info->palette[i].blue = color_table[j + 2];
			j += 3;
		}
	}
	
	info->indexed_color = FALSE;
	info->components = 3;
	info->colors = 1L << info->planes;
	info->real_width = info->width;
	info->real_height = info->height;
	info->memory_alloc = TT_RAM;
	info->page = real_frames;			/* required - more than 1 = animation */
	info->orientation = UP_TO_DOWN;
	info->num_comments = 0;				/* required - disable exif tab */
	info->_priv_var = 0;				/* y position in bmap */
	info->_priv_var_more = 0;			/* current line number */
	info->_priv_ptr = bmap;
	info->_priv_ptr_more = (void *)(intptr_t)handle;

	strcpy(info->compression, "FLIC");
	strcpy(info->info, "Autodesk");
	if (flic_header.creator == 0x45474900L)
		strcat(info->info, " (EGI)");
	else if (flic_header.creator == 0x464C4942L)
		strcat(info->info, " (FlicLib)");
	else if (flic_header.creator == 0x42494C46L)
		strcat(info->info, " (FlicLib release 2)");
	else if (flic_header.creator == 0x41544542L)
		strcat(info->info, " (Micrografx Simply 3D)");
	else if (flic_header.creator == 0x30314C46L)
		strcat(info->info, " (3D Studio MAX)");
	else if (flic_header.creator == 0x41504558L)
		strcat(info->info, " (Apex Media)");
	else
		strcat(info->info, " Animator FLIC");
	
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
	size_t pos;
	
	pos = info->_priv_var;
	if (info->_priv_var_more == 0)
	{
		int16_t handle;
		
		handle = (int16_t)(intptr_t)info->_priv_ptr_more;
		Fseek(frame_positions[info->page_wanted], handle, SEEK_SET);
		if (read_frame(handle, info) == FALSE)
		{
			return FALSE;
		}
		info->delay = delay;
		pos = 0;
	}
	
	switch (info->planes)
	{
	case 8:
		{
			uint16_t x;
			uint16_t byte;
			uint8_t *bmap = info->_priv_ptr;
		
			for (x = 0; x < info->width; x++)
			{
				byte = bmap[pos++];
				*buffer++ = info->palette[byte].red;
				*buffer++ = info->palette[byte].green;
				*buffer++ = info->palette[byte].blue;
			}
		}
		break;
	case 15:
		{
			uint16_t x;
			uint16_t color;
			uint16_t *bmap16 = (uint16_t *)info->_priv_ptr;
			
			for (x = 0; x < info->width; x++)
			{
				color = bmap16[pos++];
				*buffer++ = ((color >> 10) & 0x1f) << 3;
				*buffer++ = ((color >> 5) & 0x1f) << 3;
				*buffer++ = ((color) & 0x1f) << 3;
			}
		}
		break;
	case 16:
		{
			uint16_t x;
			uint16_t color;
			uint16_t *bmap16 = (uint16_t *)info->_priv_ptr;
			
			for (x = 0; x < info->width; x++)
			{
				color = bmap16[pos++];
				*buffer++ = ((color >> 11) & 0x1f) << 3;
				*buffer++ = ((color >> 5) & 0x3f) << 2;
				*buffer++ = ((color) & 0x1f) << 3;
			}
		}
		break;
	case 24:
		{
			uint8_t *bmap = info->_priv_ptr;
			size_t linesize = (size_t)info->width * 3;
			memcpy(buffer, &bmap[pos], linesize);
			pos += linesize;
		}
		break;
	}
	
	++info->_priv_var_more;
	if (info->_priv_var_more == info->height)
	{
		info->_priv_var_more = 0;
		pos = 0;
	}
	
	info->_priv_var = pos;
	
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
	if (info->_priv_ptr_more != 0)
	{
		Fclose((int16_t)(intptr_t)info->_priv_ptr_more);
		info->_priv_ptr_more = 0;
	}
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

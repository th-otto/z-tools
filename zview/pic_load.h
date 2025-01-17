#include "plugins/common/imginfo.h"
#include "plugins.h"

typedef struct _dec_data *DECDATA;

typedef struct _dec_data
{
	uint8  		*RowBuf;
	void   		*DthBuf;
	uint8		*DstBuf;
	uint16    	DthWidth;
	uint16    	PixMask;
	int32   	LnSize;
	uint32   	IncXfx;
	uint32   	IncYfx;
	uint32    	Pixel[256];
} dec_data;


extern void (*raster)(DECDATA, void *dst);
extern void (*raster_cmap)(DECDATA, void *);
extern void (*raster_true)(DECDATA, void *);
extern void (*rasterize_32)(DECDATA, void *);
extern void (*cnvpal_color)(IMGINFO, DECDATA);
extern void (*raster_gray)(DECDATA, void *);

CODEC *get_codec(const char *file);
CODEC *find_codec(const char *file);
boolean get_pic_info(const char *file, IMGINFO info);

boolean pic_load(const char *file, IMAGE *img, boolean quiet);

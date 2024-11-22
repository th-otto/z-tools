struct BitMap {
	uint16_t BytesPerRow;
	uint16_t Rows;
	uint8_t pad;
	uint8_t Depth;
	uint16_t reserved;
	uint8_t *Planes[8];
};

struct DCTVCvtHandle {
	uint8_t *Red, *Green, *Blue, *Chunky;
	struct BitMap *BitMap;
	uint16_t Width;
	uint16_t Height;
	uint8_t Depth;
	uint8_t Lace;
	int16_t LineNum;
	uint8_t *FBuf1[4];
	uint8_t *FBuf2[4];
	uint8_t Palette[16];
	/* private data follows (10*width) */
	/* width bytes of Red */
	/* width bytes of Green */
	/* width bytes of Blue */
	/* width bytes of Chunky */
	/* 3*width bytes of luma/chroma (first field) */
	/* 3*width bytes of luma/chroma (second field) */
};

void ConvertDCTVLine(struct DCTVCvtHandle *cvt);
int CheckDCTV(struct BitMap *bmp);
void SetmapDCTV(struct DCTVCvtHandle *cvt, const uint16_t *cmap);
struct DCTVCvtHandle *AllocDCTV(struct BitMap *bmp, int interlace);


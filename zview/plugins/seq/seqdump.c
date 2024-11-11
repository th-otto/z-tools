#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <endian.h>



struct file_header {
	uint16_t id;
	uint16_t version;
	uint32_t num_frames;
	uint16_t speed;
	uint8_t resolution;
	uint8_t flags;
	uint8_t reserved[52];
	uint8_t pad[64];
};

struct frame_header {
	uint16_t type;
	uint16_t resolution;
	uint16_t palette[16];
	char filename[12];
	uint16_t segment;
	uint8_t active;
	uint8_t speed;
	uint16_t slide;
	uint16_t x_offset;
	uint16_t y_offset;
	uint16_t width;
	uint16_t height;
	uint8_t operation;
	uint8_t compression;
	uint32_t datasize;
	uint8_t reserved[30];
	uint8_t pad[30];
};

struct file_header file_header;
struct frame_header frame_header;
uint32_t *frame_positions;

int main(int argc, char **argv)
{
	uint32_t i;
	FILE *fp;
	char *filename;
	uint32_t num_frames;
	
	while (--argc > 0)
	{
		filename = *++argv;
		fp = fopen(filename, "rb");
		if (fp == NULL)
			return 1;
		fread(&file_header, sizeof(file_header), 1, fp);
		if (be16toh(file_header.id) != 0xfedc && be16toh(file_header.id) != 0xfedb)
		{
			fprintf(stderr, "%s: wrong filetype\n", filename);
			return 1;
		}
		num_frames = be32toh(file_header.num_frames);
		printf("%s:%u frames\n", filename, num_frames);
		frame_positions = malloc(num_frames * sizeof(*frame_positions));
		fread(frame_positions, sizeof(*frame_positions), num_frames, fp);
		for (i = 0; i < num_frames; i++)
			frame_positions[i] = be32toh(frame_positions[i]);
		for (i = 0; i < num_frames; i++)
		{
			int type;
			
			fseek(fp, frame_positions[i], SEEK_SET);
			fread(&frame_header, sizeof(frame_header), 1, fp);
			type = (frame_header.operation * 2) | frame_header.compression;
			if (type != 1 && type != 3)
				printf("%s: %3u: %8u %u %u\n", filename, i, frame_positions[i], type, be32toh(frame_header.datasize));
		}
		fclose(fp);
	}
	return 0;
}

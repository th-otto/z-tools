#include <stdio.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <zlib.h>
#include "plutosvg.h"

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
static uint32_t swap32(uint32_t l)
{
	return ((l >> 24) & 0xffL) | ((l << 8) & 0xff0000L) | ((l >> 8) & 0xff00L) | ((l << 24) & 0xff000000UL);
}
#endif

int main(int argc, char *argv[])
{
	FILE *fp;
	uint8_t magic[2];
	char *data;
	uint32_t file_size;
	plutosvg_document_t *document = NULL;
	plutovg_surface_t *surface = NULL;
	const char *input;
	const char *output;
	const char *id = NULL;

	if (argc < 2)
	{
		fprintf(stderr, "Usage: svg2png input [output [id]]\n");
		return 1;
	}

	input = argv[1];
	if (argc >= 3)
	{
		output = argv[2];
	} else
	{
		char *tmp = malloc(strlen(input) + 5);
		strcpy(tmp, input);
		strcat(tmp, ".png");
		output = tmp;
	}
	if (argc >= 4)
	{
		id = argv[3];
	}

	fp = fopen(input, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "%s: %s\n", input, strerror(errno));
		return 1;
	}

	if (fread(magic, 1, 2, fp) == 2 && magic[0] == 31 && magic[1] == 139)
	{
		gzFile gzfile;

		fseek(fp, -4, SEEK_END);
		fread(&file_size, 1, sizeof(file_size), fp);
#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
		file_size = swap32(file_size);
#endif
		data = malloc(file_size + 1);
		if (data == NULL)
		{
			fprintf(stderr, "%s: %s\n", input, strerror(errno));
			return 1;
		}
		gzfile = gzdopen(fileno(fp), "r");
		if (gzfile == NULL)
		{
			fprintf(stderr, "%s: %s\n", input, strerror(errno));
			return 1;
		}

		/*
		 * Use gzfread rather than gzread, because parameter for gzread
		 * may be 16bit only
		 */
		if (gzfread(data, 1, file_size, gzfile) != file_size)
		{
			gzclose(gzfile);
			return 1;
		}
		gzclose(gzfile);
	} else
	{
		fseek(fp, 0, SEEK_END);
		file_size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		data = malloc(file_size + 1);
		if (fread(data, 1, file_size, fp) != file_size)
		{
			fprintf(stderr, "%s: %s\n", input, strerror(errno));
			return 1;
		}
		fclose(fp);
	}
	data[file_size] = '\0';				/* Must be null terminated. */

	document = plutosvg_document_load_from_data(data, file_size, -1, -1, NULL, NULL);

	if (document == NULL)
	{
		fprintf(stderr, "Unable to load '%s'\n", input);
		goto cleanup;
	}

	surface = plutosvg_document_render_to_surface(document, id, -1, -1, NULL, NULL, NULL);
	if (surface == NULL)
	{
		fprintf(stderr, "Unable to render '%s'\n", input);
		goto cleanup;
	}

	if (!plutovg_surface_write_to_png(surface, output))
	{
		fprintf(stderr, "Unable to write '%s'\n", output);
		goto cleanup;
	}

	fprintf(stdout, "Finished writing '%s'\n", output);
  cleanup:
	plutovg_surface_destroy(surface);
	plutosvg_document_destroy(document);
	return 0;
}

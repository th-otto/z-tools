#include "private.h"
#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "image.h"

static plutovg_surface_t *plutovg_surface_load_from_image(stbi_uc *image, plutovg_int_t width, plutovg_int_t height)
{
	plutovg_surface_t *surface = plutovg_surface_create_uninitialized(width, height);

	if (surface)
		plutovg_convert_rgba_to_argb(surface->data, image, surface->width, surface->height, surface->stride);
	stbi_image_free(image);
	return surface;
}

plutovg_surface_t *plutovg_surface_load_from_image_file(const char *filename)
{
	plutovg_int_t width, height;
	int channels;
	stbi_uc *image = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

	if (image == NULL)
		return NULL;
	return plutovg_surface_load_from_image(image, width, height);
}

plutovg_surface_t *plutovg_surface_load_from_image_data(const void *data, plutovg_int_t length)
{
	plutovg_int_t width, height;
	int channels;
	stbi_uc *image = stbi_load_from_memory(data, length, &width, &height, &channels, STBI_rgb_alpha);

	if (image == NULL)
		return NULL;
	return plutovg_surface_load_from_image(image, width, height);
}

static const uint8_t base64_table[128] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00, 0x3F,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
	0x3C, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
	0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
	0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,
	0x17, 0x18, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
	0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
	0x31, 0x32, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00
};

plutovg_surface_t *plutovg_surface_load_from_image_base64(const char *data, plutovg_int_t length)
{
	plutovg_surface_t *surface = NULL;
	uint8_t *output_data = NULL;
	size_t output_length = 0;
	size_t equals_sign_count = 0;
	size_t sidx = 0;
	size_t didx = 0;
	plutovg_int_t i;
	
	if (length == -1)
		length = strlen(data);
	output_data = malloc(length);
	if (output_data == NULL)
		return NULL;
	for (i = 0; i < length; ++i)
	{
		uint8_t cc = data[i];

		if (cc == '=')
		{
			++equals_sign_count;
		} else if (cc == '+' || cc == '/' || PLUTOVG_IS_ALNUM(cc))
		{
			if (equals_sign_count > 0)
				goto cleanup;
			output_data[output_length++] = base64_table[cc];
		} else if (!PLUTOVG_IS_WS(cc))
		{
			goto cleanup;
		}
	}

	if (output_length == 0 || equals_sign_count > 2 || (output_length % 4) == 1)
		goto cleanup;
	output_length -= (output_length + 3) / 4;
	if (output_length == 0)
	{
		goto cleanup;
	}

	if (output_length > 1)
	{
		while (didx < output_length - 2)
		{
			output_data[didx + 0] = (((output_data[sidx + 0] << 2) & 255) | ((output_data[sidx + 1] >> 4) & 003));
			output_data[didx + 1] = (((output_data[sidx + 1] << 4) & 255) | ((output_data[sidx + 2] >> 2) & 017));
			output_data[didx + 2] = (((output_data[sidx + 2] << 6) & 255) | ((output_data[sidx + 3] >> 0) & 077));
			sidx += 4;
			didx += 3;
		}
	}

	if (didx < output_length)
		output_data[didx] = (((output_data[sidx + 0] << 2) & 255) | ((output_data[sidx + 1] >> 4) & 003));
	if (++didx < output_length)
	{
		output_data[didx] = (((output_data[sidx + 1] << 4) & 255) | ((output_data[sidx + 2] >> 2) & 017));
	}

	surface = plutovg_surface_load_from_image_data(output_data, output_length);
  cleanup:
	free(output_data);
	return surface;
}


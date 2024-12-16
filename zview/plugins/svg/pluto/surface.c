#include "private.h"
#include "utils.h"

plutovg_surface_t *plutovg_surface_create_uninitialized(plutovg_int_t width, plutovg_int_t height)
{
	size_t size;
	plutovg_surface_t *surface;

	if (width > STBI_MAX_DIMENSIONS || height > STBI_MAX_DIMENSIONS)
		return NULL;
	size = width * height * 4;
	surface = malloc(size + sizeof(plutovg_surface_t));

	if (surface != NULL)
	{
		surface->ref_count = 1;
		surface->width = width;
		surface->height = height;
		surface->stride = width * 4;
		surface->data = (uint8_t *) (surface + 1);
	}
	return surface;
}

plutovg_surface_t *plutovg_surface_create(plutovg_int_t width, plutovg_int_t height)
{
	plutovg_surface_t *surface = plutovg_surface_create_uninitialized(width, height);

	if (surface)
		memset(surface->data, 0, surface->height * surface->stride);
	return surface;
}

plutovg_surface_t *plutovg_surface_create_for_data(unsigned char *data, plutovg_int_t width, plutovg_int_t height, plutovg_int_t stride)
{
	plutovg_surface_t *surface = malloc(sizeof(plutovg_surface_t));

	if (surface != NULL)
	{
		surface->ref_count = 1;
		surface->width = width;
		surface->height = height;
		surface->stride = stride;
		surface->data = data;
	}
	return surface;
}

plutovg_surface_t *plutovg_surface_reference(plutovg_surface_t *surface)
{
	if (surface == NULL)
		return NULL;
	++surface->ref_count;
	return surface;
}

void plutovg_surface_destroy(plutovg_surface_t *surface)
{
	if (surface == NULL)
		return;
	if (--surface->ref_count == 0)
	{
		free(surface);
	}
}

plutovg_int_t plutovg_surface_get_reference_count(const plutovg_surface_t *surface)
{
	if (surface)
		return surface->ref_count;
	return 0;
}

unsigned char *plutovg_surface_get_data(const plutovg_surface_t *surface)
{
	return surface->data;
}

plutovg_int_t plutovg_surface_get_width(const plutovg_surface_t *surface)
{
	return surface->width;
}

plutovg_int_t plutovg_surface_get_height(const plutovg_surface_t *surface)
{
	return surface->height;
}

plutovg_int_t plutovg_surface_get_stride(const plutovg_surface_t *surface)
{
	return surface->stride;
}

void plutovg_surface_clear(plutovg_surface_t *surface, const plutovg_color_t *color)
{
	uint32_t pixel = plutovg_premultiply_argb(plutovg_color_to_argb32(color));
	plutovg_int_t y;
	
	for (y = 0; y < surface->height; y++)
	{
		uint32_t *pixels = (uint32_t *) (surface->data + surface->stride * y);

		plutovg_memfill32(pixels, surface->width, pixel);
	}
}

void plutovg_convert_argb_to_rgba(unsigned char *dst, const unsigned char *src, plutovg_int_t width, plutovg_int_t height, plutovg_int_t stride)
{
	plutovg_int_t x, y;
	
	for (y = 0; y < height; y++)
	{
		const uint32_t *src_row = (const uint32_t *) (src + stride * y);
		uint32_t *dst_row = (uint32_t *) (dst + stride * y);

		for (x = 0; x < width; x++)
		{
			uint32_t pixel = src_row[x];
			uint32_t a = (pixel >> 24) & 0xFF;

			if (a == 0)
			{
				dst_row[x] = 0x00000000;
			} else
			{
				uint32_t r = (pixel >> 16) & 0xFF;
				uint32_t g = (pixel >> 8) & 0xFF;
				uint32_t b = (pixel >> 0) & 0xFF;

				if (a != 255)
				{
					r = (r * 255) / a;
					g = (g * 255) / a;
					b = (b * 255) / a;
				}

				dst_row[x] = (a << 24) | (b << 16) | (g << 8) | r;
			}
		}
	}
}

void plutovg_convert_rgba_to_argb(unsigned char *dst, const unsigned char *src, plutovg_int_t width, plutovg_int_t height, plutovg_int_t stride)
{
	plutovg_int_t x, y;
	
	for (y = 0; y < height; y++)
	{
		const uint32_t *src_row = (const uint32_t *) (src + stride * y);
		uint32_t *dst_row = (uint32_t *) (dst + stride * y);

		for (x = 0; x < width; x++)
		{
			uint32_t pixel = src_row[x];
			uint32_t a = (pixel >> 24) & 0xFF;

			if (a == 0)
			{
				dst_row[x] = 0x00000000;
			} else
			{
				uint32_t b = (pixel >> 16) & 0xFF;
				uint32_t g = (pixel >> 8) & 0xFF;
				uint32_t r = (pixel >> 0) & 0xFF;

				if (a != 255)
				{
					r = (r * a) / 255;
					g = (g * a) / 255;
					b = (b * a) / 255;
				}

				dst_row[x] = (a << 24) | (r << 16) | (g << 8) | b;
			}
		}
	}
}

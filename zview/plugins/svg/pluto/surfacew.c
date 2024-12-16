#include "private.h"
#include "utils.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "imagewr.h"

static void plutovg_surface_write_begin(const plutovg_surface_t *surface)
{
	plutovg_convert_argb_to_rgba(surface->data, surface->data, surface->width, surface->height, surface->stride);
}

static void plutovg_surface_write_end(const plutovg_surface_t *surface)
{
	plutovg_convert_rgba_to_argb(surface->data, surface->data, surface->width, surface->height, surface->stride);
}

bool plutovg_surface_write_to_png(const plutovg_surface_t *surface, const char *filename)
{
	int success;

	plutovg_surface_write_begin(surface);
	success = stbi_write_png(filename, surface->width, surface->height, 4, surface->data, surface->stride);

	plutovg_surface_write_end(surface);
	return success;
}

bool plutovg_surface_write_to_png_stream(const plutovg_surface_t *surface, plutovg_write_func_t write_func, void *closure)
{
	int success;

	plutovg_surface_write_begin(surface);
	success = stbi_write_png_to_func(write_func, closure, surface->width, surface->height, 4, surface->data, surface->stride);
	plutovg_surface_write_end(surface);
	return success;
}

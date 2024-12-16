#ifndef PLUTOVG_PRIVATE_H
#define PLUTOVG_PRIVATE_H

#define PLUTOVG_BUILD 1

#include <stddef.h>
#include "plutovg.h"

#ifndef STBI_MAX_DIMENSIONS
#define STBI_MAX_DIMENSIONS ((plutovg_int_t)1 << 15)
#endif

#ifdef __PUREC__
#  define DIM_MAX LONG_MAX /* actually max value of plutovg_int_t */
#  define DIM_MIN LONG_MIN /* actually max value of plutovg_int_t */
#else
#  define DIM_MAX INT_MAX
#  define DIM_MIN INT_MIN
#endif

struct plutovg_surface
{
	plutovg_int_t ref_count;
	plutovg_int_t width;
	plutovg_int_t height;
	plutovg_int_t stride;
	unsigned char *data;
};

struct plutovg_path
{
	plutovg_int_t ref_count;
	plutovg_int_t num_curves;
	plutovg_int_t num_contours;
	plutovg_int_t num_points;
	plutovg_point_t start_point;
	struct
	{
		plutovg_path_element_t *data;
		plutovg_int_t size;
		plutovg_int_t capacity;
	} elements;
};

typedef enum
{
	PLUTOVG_PAINT_TYPE_COLOR,
	PLUTOVG_PAINT_TYPE_GRADIENT,
	PLUTOVG_PAINT_TYPE_TEXTURE
} plutovg_paint_type_t;

struct plutovg_paint
{
	plutovg_int_t ref_count;
	plutovg_paint_type_t type;
};

typedef struct
{
	plutovg_paint_t base;
	plutovg_color_t color;
} plutovg_solid_paint_t;

typedef enum
{
	PLUTOVG_GRADIENT_TYPE_LINEAR,
	PLUTOVG_GRADIENT_TYPE_RADIAL
} plutovg_gradient_type_t;

typedef struct
{
	plutovg_paint_t base;
	plutovg_gradient_type_t type;
	plutovg_spread_method_t spread;
	plutovg_matrix_t matrix;
	plutovg_gradient_stop_t *stops;
	plutovg_int_t nstops;
	float values[6];
} plutovg_gradient_paint_t;

typedef struct
{
	plutovg_paint_t base;
	plutovg_texture_type_t type;
	float opacity;
	plutovg_matrix_t matrix;
	plutovg_surface_t *surface;
} plutovg_texture_paint_t;

typedef struct
{
	plutovg_int_t x;
	plutovg_int_t len;
	plutovg_int_t y;
	unsigned char coverage;
} plutovg_span_t;

typedef struct
{
	struct
	{
		plutovg_span_t *data;
		plutovg_int_t size;
		plutovg_int_t capacity;
	} spans;

	plutovg_int_t x;
	plutovg_int_t y;
	plutovg_int_t w;
	plutovg_int_t h;
} plutovg_span_buffer_t;

typedef struct
{
	float offset;
	struct
	{
		float *data;
		plutovg_int_t size;
		plutovg_int_t capacity;
	} array;
} plutovg_stroke_dash_t;

typedef struct
{
	float width;
	plutovg_line_cap_t cap;
	plutovg_line_join_t join;
	float miter_limit;
} plutovg_stroke_style_t;

typedef struct
{
	plutovg_stroke_style_t style;
	plutovg_stroke_dash_t dash;
} plutovg_stroke_data_t;

typedef struct plutovg_state
{
	plutovg_paint_t *paint;
	plutovg_color_t color;
	plutovg_matrix_t matrix;
	plutovg_stroke_data_t stroke;
	plutovg_operator_t op;
	plutovg_fill_rule_t winding;
	plutovg_span_buffer_t clip_spans;
	plutovg_font_face_t *font_face;
	float font_size;
	float opacity;
	bool clipping;
	struct plutovg_state *next;
} plutovg_state_t;

struct plutovg_canvas
{
	plutovg_int_t ref_count;
	plutovg_surface_t *surface;
	plutovg_path_t *path;
	plutovg_state_t *state;
	plutovg_state_t *freed_state;
	plutovg_rect_t clip_rect;
	plutovg_span_buffer_t clip_spans;
	plutovg_span_buffer_t fill_spans;
};

void plutovg_span_buffer_init(plutovg_span_buffer_t *span_buffer);
void plutovg_span_buffer_init_rect(plutovg_span_buffer_t *span_buffer, plutovg_int_t x, plutovg_int_t y, plutovg_int_t width, plutovg_int_t height);
void plutovg_span_buffer_reset(plutovg_span_buffer_t *span_buffer);
void plutovg_span_buffer_destroy(plutovg_span_buffer_t *span_buffer);
void plutovg_span_buffer_copy(plutovg_span_buffer_t *span_buffer, const plutovg_span_buffer_t *source);
void plutovg_span_buffer_extents(plutovg_span_buffer_t *span_buffer, plutovg_rect_t *extents);
void plutovg_span_buffer_intersect(plutovg_span_buffer_t *span_buffer, const plutovg_span_buffer_t *a, const plutovg_span_buffer_t *b);

void plutovg_rasterize(plutovg_span_buffer_t *span_buffer, const plutovg_path_t *path,
	const plutovg_matrix_t *matrix, const plutovg_rect_t *clip_rect,
	const plutovg_stroke_data_t *stroke_data, plutovg_fill_rule_t winding);
void plutovg_blend(plutovg_canvas_t *canvas, const plutovg_span_buffer_t *span_buffer);
void plutovg_memfill32(uint32_t *dest, plutovg_int_t length, uint32_t value);

#ifdef __PUREC__
#include <math.h>
/* Pure-C is lacking the float functions */
#define cosf cos
#define sinf sin
#define tanf tan
#define sqrtf sqrt
#define roundf round
#define lroundf(x) (long)round(x)
#define ceilf ceil
#define fabsf fabs
#define fmodf fmod
#define acosf acos
#define floorf floor
#define atan2f atan2
#endif

#endif /* PLUTOVG_PRIVATE_H */

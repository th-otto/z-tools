#include "private.h"
#include "utils.h"

#if 'A' == 0x41 && 'a' == 0x61
#undef isalpha
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#undef isxdigit
#define isxdigit(c) (((c) >= '0' && (c) <= '9') || ((c) >= 'a' && (c) <= 'f') || ((c) >= 'A' && (c) <= 'F'))
#undef tolower
#define tolower(c) ((c) | ('a' - 'A'))
#endif

void plutovg_color_init_rgb(plutovg_color_t *color, float r, float g, float b)
{
	plutovg_color_init_rgba(color, r, g, b, 1.f);
}

void plutovg_color_init_rgba(plutovg_color_t *color, float r, float g, float b, float a)
{
	color->r = plutovg_clamp(r, 0.f, 1.f);
	color->g = plutovg_clamp(g, 0.f, 1.f);
	color->b = plutovg_clamp(b, 0.f, 1.f);
	color->a = plutovg_clamp(a, 0.f, 1.f);
}

void plutovg_color_init_rgb8(plutovg_color_t *color, int r, int g, int b)
{
	plutovg_color_init_rgba8(color, r, g, b, 255);
}

void plutovg_color_init_rgba8(plutovg_color_t *color, int r, int g, int b, int a)
{
	plutovg_color_init_rgba(color, r / 255.f, g / 255.f, b / 255.f, a / 255.f);
}

void plutovg_color_init_rgba32(plutovg_color_t *color, uint32_t value)
{
	uint8_t r = (value >> 24) & 0xFF;
	uint8_t g = (value >> 16) & 0xFF;
	uint8_t b = (value >> 8) & 0xFF;
	uint8_t a = (value >> 0) & 0xFF;

	plutovg_color_init_rgba8(color, r, g, b, a);
}

void plutovg_color_init_argb32(plutovg_color_t *color, uint32_t value)
{
	uint8_t a = (value >> 24) & 0xFF;
	uint8_t r = (value >> 16) & 0xFF;
	uint8_t g = (value >> 8) & 0xFF;
	uint8_t b = (value >> 0) & 0xFF;

	plutovg_color_init_rgba8(color, r, g, b, a);
}

uint32_t plutovg_color_to_rgba32(const plutovg_color_t *color)
{
	uint32_t r = lroundf(color->r * 255);
	uint32_t g = lroundf(color->g * 255);
	uint32_t b = lroundf(color->b * 255);
	uint32_t a = lroundf(color->a * 255);

	return (r << 24) | (g << 16) | (b << 8) | (a);
}

uint32_t plutovg_color_to_argb32(const plutovg_color_t *color)
{
	uint32_t a = lroundf(color->a * 255);
	uint32_t r = lroundf(color->r * 255);
	uint32_t g = lroundf(color->g * 255);
	uint32_t b = lroundf(color->b * 255);

	return (a << 24) | (r << 16) | (g << 8) | (b);
}

static inline uint8_t hex_digit(uint8_t c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return 10 + c - 'a';
	if (c >= 'A' && c <= 'F')
		return 10 + c - 'A';
	return 0;
}

static inline uint8_t hex_expand(uint8_t c)
{
	uint8_t h = hex_digit(c);

	return (h << 4) | h;
}

static inline uint8_t hex_combine(uint8_t c1, uint8_t c2)
{
	uint8_t h1 = hex_digit(c1);
	uint8_t h2 = hex_digit(c2);

	return (h1 << 4) | h2;
}

#define MAX_NAME 24
typedef struct
{
	char name[MAX_NAME];
	uint32_t value;
} color_entry_t;

static int color_entry_compare(const void *a, const void *b)
{
	const char *name = a;
	const color_entry_t *entry = b;

	return strcmp(name, entry->name);
}

static bool parse_rgb_component(const char **begin, const char *end, int *component)
{
	float value = 0;

	if (!plutovg_parse_number(begin, end, &value))
		return false;
	if (plutovg_skip_delim(begin, end, '%'))
		value *= 2.55f;
	value = plutovg_clamp(value, 0.f, 255.f);
	*component = (int)lroundf(value);
	return true;
}

static bool parse_alpha_component(const char **begin, const char *end, int *component)
{
	float value = 0;

	if (!plutovg_parse_number(begin, end, &value))
		return false;
	if (plutovg_skip_delim(begin, end, '%'))
		value /= 100.f;
	value = plutovg_clamp(value, 0.f, 1.f);
	*component = (int)lroundf(value * 255.f);
	return true;
}

plutovg_int_t plutovg_color_parse(plutovg_color_t *color, const char *data, plutovg_int_t length)
{
	const char *it;
	const char *end;

	if (length == -1)
		length = strlen(data);
	it = data;
	end = it + length;

	plutovg_skip_ws(&it, end);
	if (plutovg_skip_delim(&it, end, '#'))
	{
		int r, g, b;
		int a = 255;
		const char *begin = it;
		plutovg_int_t count;
		
		while (it < end && isxdigit(*it))
			++it;
		count = (plutovg_int_t)(it - begin);

		if (count == 3 || count == 4)
		{
			r = hex_expand(begin[0]);
			g = hex_expand(begin[1]);
			b = hex_expand(begin[2]);
			if (count == 4)
			{
				a = hex_expand(begin[3]);
			}
		} else if (count == 6 || count == 8)
		{
			r = hex_combine(begin[0], begin[1]);
			g = hex_combine(begin[2], begin[3]);
			b = hex_combine(begin[4], begin[5]);
			if (count == 8)
			{
				a = hex_combine(begin[6], begin[7]);
			}
		} else
		{
			return 0;
		}

		plutovg_color_init_rgba8(color, r, g, b, a);
	} else
	{
		plutovg_int_t name_length = 0;
		char name[MAX_NAME + 1];

		while (it < end && name_length < MAX_NAME && isalpha(*it))
			name[name_length++] = tolower(*it++);
		name[name_length] = '\0';

		if (strcmp(name, "transparent") == 0)
		{
			plutovg_color_init_rgba(color, 0, 0, 0, 0);
		} else if (strcmp(name, "rgb") == 0 || strcmp(name, "rgba") == 0)
		{
			int r, g, b;
			int a = 255;

			if (!plutovg_skip_ws_and_delim(&it, end, '('))
				return 0;
			if (!parse_rgb_component(&it, end, &r)
				|| !plutovg_skip_ws_and_comma(&it, end)
				|| !parse_rgb_component(&it, end, &g)
				|| !plutovg_skip_ws_and_comma(&it, end) || !parse_rgb_component(&it, end, &b))
			{
				return 0;
			}

			if (plutovg_skip_ws_and_comma(&it, end) && !parse_alpha_component(&it, end, &a))
			{
				return 0;
			}

			plutovg_skip_ws(&it, end);
			if (!plutovg_skip_delim(&it, end, ')'))
				return 0;
			plutovg_color_init_rgba8(color, r, g, b, a);
		} else
		{
			static const color_entry_t colormap[] = {
				{ "aliceblue", 0xF0F8FFUL },
				{ "antiquewhite", 0xFAEBD7UL },
				{ "aqua", 0x00FFFFUL },
				{ "aquamarine", 0x7FFFD4UL },
				{ "azure", 0xF0FFFFUL },
				{ "beige", 0xF5F5DCUL },
				{ "bisque", 0xFFE4C4UL },
				{ "black", 0x000000UL },
				{ "blanchedalmond", 0xFFEBCDUL },
				{ "blue", 0x0000FFUL },
				{ "blueviolet", 0x8A2BE2UL },
				{ "brown", 0xA52A2AUL },
				{ "burlywood", 0xDEB887UL },
				{ "cadetblue", 0x5F9EA0UL },
				{ "chartreuse", 0x7FFF00UL },
				{ "chocolate", 0xD2691EUL },
				{ "coral", 0xFF7F50UL },
				{ "cornflowerblue", 0x6495EDUL },
				{ "cornsilk", 0xFFF8DCUL },
				{ "crimson", 0xDC143CUL },
				{ "cyan", 0x00FFFFUL },
				{ "darkblue", 0x00008BUL },
				{ "darkcyan", 0x008B8BUL },
				{ "darkgoldenrod", 0xB8860BUL },
				{ "darkgray", 0xA9A9A9UL },
				{ "darkgreen", 0x006400UL },
				{ "darkgrey", 0xA9A9A9UL },
				{ "darkkhaki", 0xBDB76BUL },
				{ "darkmagenta", 0x8B008BUL },
				{ "darkolivegreen", 0x556B2FUL },
				{ "darkorange", 0xFF8C00UL },
				{ "darkorchid", 0x9932CCUL },
				{ "darkred", 0x8B0000UL },
				{ "darksalmon", 0xE9967AUL },
				{ "darkseagreen", 0x8FBC8FUL },
				{ "darkslateblue", 0x483D8BUL },
				{ "darkslategray", 0x2F4F4FUL },
				{ "darkslategrey", 0x2F4F4FUL },
				{ "darkturquoise", 0x00CED1UL },
				{ "darkviolet", 0x9400D3UL },
				{ "deeppink", 0xFF1493UL },
				{ "deepskyblue", 0x00BFFFUL },
				{ "dimgray", 0x696969UL },
				{ "dimgrey", 0x696969UL },
				{ "dodgerblue", 0x1E90FFUL },
				{ "firebrick", 0xB22222UL },
				{ "floralwhite", 0xFFFAF0UL },
				{ "forestgreen", 0x228B22UL },
				{ "fuchsia", 0xFF00FFUL },
				{ "gainsboro", 0xDCDCDCUL },
				{ "ghostwhite", 0xF8F8FFUL },
				{ "gold", 0xFFD700UL },
				{ "goldenrod", 0xDAA520UL },
				{ "gray", 0x808080UL },
				{ "green", 0x008000UL },
				{ "greenyellow", 0xADFF2FUL },
				{ "grey", 0x808080UL },
				{ "honeydew", 0xF0FFF0UL },
				{ "hotpink", 0xFF69B4UL },
				{ "indianred", 0xCD5C5CUL },
				{ "indigo", 0x4B0082UL },
				{ "ivory", 0xFFFFF0UL },
				{ "khaki", 0xF0E68CUL },
				{ "lavender", 0xE6E6FAUL },
				{ "lavenderblush", 0xFFF0F5UL },
				{ "lawngreen", 0x7CFC00UL },
				{ "lemonchiffon", 0xFFFACDUL },
				{ "lightblue", 0xADD8E6UL },
				{ "lightcoral", 0xF08080UL },
				{ "lightcyan", 0xE0FFFFUL },
				{ "lightgoldenrodyellow", 0xFAFAD2UL },
				{ "lightgray", 0xD3D3D3UL },
				{ "lightgreen", 0x90EE90UL },
				{ "lightgrey", 0xD3D3D3UL },
				{ "lightpink", 0xFFB6C1UL },
				{ "lightsalmon", 0xFFA07AUL },
				{ "lightseagreen", 0x20B2AAUL },
				{ "lightskyblue", 0x87CEFAUL },
				{ "lightslategray", 0x778899UL },
				{ "lightslategrey", 0x778899UL },
				{ "lightsteelblue", 0xB0C4DEUL },
				{ "lightyellow", 0xFFFFE0UL },
				{ "lime", 0x00FF00UL },
				{ "limegreen", 0x32CD32UL },
				{ "linen", 0xFAF0E6UL },
				{ "magenta", 0xFF00FFUL },
				{ "maroon", 0x800000UL },
				{ "mediumaquamarine", 0x66CDAAUL },
				{ "mediumblue", 0x0000CDUL },
				{ "mediumorchid", 0xBA55D3UL },
				{ "mediumpurple", 0x9370DBUL },
				{ "mediumseagreen", 0x3CB371UL },
				{ "mediumslateblue", 0x7B68EEUL },
				{ "mediumspringgreen", 0x00FA9AUL },
				{ "mediumturquoise", 0x48D1CCUL },
				{ "mediumvioletred", 0xC71585UL },
				{ "midnightblue", 0x191970UL },
				{ "mintcream", 0xF5FFFAUL },
				{ "mistyrose", 0xFFE4E1UL },
				{ "moccasin", 0xFFE4B5UL },
				{ "navajowhite", 0xFFDEADUL },
				{ "navy", 0x000080UL },
				{ "oldlace", 0xFDF5E6UL },
				{ "olive", 0x808000UL },
				{ "olivedrab", 0x6B8E23UL },
				{ "orange", 0xFFA500UL },
				{ "orangered", 0xFF4500UL },
				{ "orchid", 0xDA70D6UL },
				{ "palegoldenrod", 0xEEE8AAUL },
				{ "palegreen", 0x98FB98UL },
				{ "paleturquoise", 0xAFEEEEUL },
				{ "palevioletred", 0xDB7093UL },
				{ "papayawhip", 0xFFEFD5UL },
				{ "peachpuff", 0xFFDAB9UL },
				{ "peru", 0xCD853FUL },
				{ "pink", 0xFFC0CBUL },
				{ "plum", 0xDDA0DDUL },
				{ "powderblue", 0xB0E0E6UL },
				{ "purple", 0x800080UL },
				{ "rebeccapurple", 0x663399UL },
				{ "red", 0xFF0000UL },
				{ "rosybrown", 0xBC8F8FUL },
				{ "royalblue", 0x4169E1UL },
				{ "saddlebrown", 0x8B4513UL },
				{ "salmon", 0xFA8072UL },
				{ "sandybrown", 0xF4A460UL },
				{ "seagreen", 0x2E8B57UL },
				{ "seashell", 0xFFF5EEUL },
				{ "sienna", 0xA0522DUL },
				{ "silver", 0xC0C0C0UL },
				{ "skyblue", 0x87CEEBUL },
				{ "slateblue", 0x6A5ACDUL },
				{ "slategray", 0x708090UL },
				{ "slategrey", 0x708090UL },
				{ "snow", 0xFFFAFAUL },
				{ "springgreen", 0x00FF7FUL },
				{ "steelblue", 0x4682B4UL },
				{ "tan", 0xD2B48CUL },
				{ "teal", 0x008080UL },
				{ "thistle", 0xD8BFD8UL },
				{ "tomato", 0xFF6347UL },
				{ "turquoise", 0x40E0D0UL },
				{ "violet", 0xEE82EEUL },
				{ "wheat", 0xF5DEB3UL },
				{ "white", 0xFFFFFFUL },
				{ "whitesmoke", 0xF5F5F5UL },
				{ "yellow", 0xFFFF00UL },
				{ "yellowgreen", 0x9ACD32UL }
			};

			const color_entry_t *entry =
				bsearch(name, colormap, sizeof(colormap) / sizeof(color_entry_t), sizeof(color_entry_t), color_entry_compare);
			if (entry == NULL)
				return 0;
			plutovg_color_init_argb32(color, 0xFF000000UL | entry->value);
		}
	}

	plutovg_skip_ws(&it, end);
	return it - data;
}

static void *plutovg_paint_create(plutovg_paint_type_t type, size_t size)
{
	plutovg_paint_t *paint = malloc(size);

	paint->ref_count = 1;
	paint->type = type;
	return paint;
}

plutovg_paint_t *plutovg_paint_create_rgb(float r, float g, float b)
{
	return plutovg_paint_create_rgba(r, g, b, 1.f);
}

plutovg_paint_t *plutovg_paint_create_rgba(float r, float g, float b, float a)
{
	plutovg_solid_paint_t *solid = plutovg_paint_create(PLUTOVG_PAINT_TYPE_COLOR, sizeof(plutovg_solid_paint_t));

	solid->color.r = plutovg_clamp(r, 0.f, 1.f);
	solid->color.g = plutovg_clamp(g, 0.f, 1.f);
	solid->color.b = plutovg_clamp(b, 0.f, 1.f);
	solid->color.a = plutovg_clamp(a, 0.f, 1.f);
	return &solid->base;
}

plutovg_paint_t *plutovg_paint_create_color(const plutovg_color_t *color)
{
	return plutovg_paint_create_rgba(color->r, color->g, color->b, color->a);
}

static plutovg_gradient_paint_t *plutovg_gradient_create(plutovg_gradient_type_t type, plutovg_spread_method_t spread,
	const plutovg_gradient_stop_t *stops, plutovg_int_t nstops, const plutovg_matrix_t *matrix)
{
	plutovg_gradient_paint_t *gradient =
		plutovg_paint_create(PLUTOVG_PAINT_TYPE_GRADIENT, sizeof(plutovg_gradient_paint_t) + nstops * sizeof(plutovg_gradient_stop_t));
	float prev_offset;
	plutovg_int_t i;

	gradient->type = type;
	gradient->spread = spread;
	if (matrix)
		gradient->matrix = *matrix;
	else
		PLUTOVG_IDENTITY_MATRIX(&gradient->matrix);
	gradient->stops = (plutovg_gradient_stop_t *) (gradient + 1);
	gradient->nstops = nstops;

	prev_offset = 0.f;

	for (i = 0; i < nstops; ++i)
	{
		const plutovg_gradient_stop_t *stop = stops + i;

		gradient->stops[i].offset = plutovg_max(prev_offset, plutovg_clamp(stop->offset, 0.f, 1.f));
		gradient->stops[i].color.r = plutovg_clamp(stop->color.r, 0.f, 1.f);
		gradient->stops[i].color.g = plutovg_clamp(stop->color.g, 0.f, 1.f);
		gradient->stops[i].color.b = plutovg_clamp(stop->color.b, 0.f, 1.f);
		gradient->stops[i].color.a = plutovg_clamp(stop->color.a, 0.f, 1.f);
		prev_offset = gradient->stops[i].offset;
	}

	return gradient;
}

plutovg_paint_t *plutovg_paint_create_linear_gradient(float x1, float y1, float x2, float y2,
	plutovg_spread_method_t spread,
	const plutovg_gradient_stop_t *stops, plutovg_int_t nstops,
	const plutovg_matrix_t *matrix)
{
	plutovg_gradient_paint_t *gradient = plutovg_gradient_create(PLUTOVG_GRADIENT_TYPE_LINEAR, spread, stops, nstops, matrix);

	gradient->values[0] = x1;
	gradient->values[1] = y1;
	gradient->values[2] = x2;
	gradient->values[3] = y2;
	return &gradient->base;
}

plutovg_paint_t *plutovg_paint_create_radial_gradient(float cx, float cy, float cr, float fx, float fy, float fr,
	plutovg_spread_method_t spread,
	const plutovg_gradient_stop_t *stops, plutovg_int_t nstops,
	const plutovg_matrix_t *matrix)
{
	plutovg_gradient_paint_t *gradient = plutovg_gradient_create(PLUTOVG_GRADIENT_TYPE_RADIAL, spread, stops, nstops, matrix);

	gradient->values[0] = cx;
	gradient->values[1] = cy;
	gradient->values[2] = cr;
	gradient->values[3] = fx;
	gradient->values[4] = fy;
	gradient->values[5] = fr;
	return &gradient->base;
}

plutovg_paint_t *plutovg_paint_create_texture(plutovg_surface_t *surface, plutovg_texture_type_t type, float opacity,
	const plutovg_matrix_t *matrix)
{
	plutovg_texture_paint_t *texture = plutovg_paint_create(PLUTOVG_PAINT_TYPE_TEXTURE, sizeof(plutovg_texture_paint_t));

	texture->type = type;
	texture->opacity = plutovg_clamp(opacity, 0.f, 1.f);
	if (matrix)
		texture->matrix = *matrix;
	else
		PLUTOVG_IDENTITY_MATRIX(&texture->matrix);
	texture->surface = plutovg_surface_reference(surface);
	return &texture->base;
}

plutovg_paint_t *plutovg_paint_reference(plutovg_paint_t *paint)
{
	if (paint == NULL)
		return NULL;
	++paint->ref_count;
	return paint;
}

void plutovg_paint_destroy(plutovg_paint_t *paint)
{
	if (paint == NULL)
		return;
	if (--paint->ref_count == 0)
	{
		if (paint->type == PLUTOVG_PAINT_TYPE_TEXTURE)
		{
			plutovg_texture_paint_t *texture = (plutovg_texture_paint_t *) (paint);

			plutovg_surface_destroy(texture->surface);
		}

		free(paint);
	}
}

plutovg_int_t plutovg_paint_get_reference_count(const plutovg_paint_t *paint)
{
	if (paint)
		return paint->ref_count;
	return 0;
}

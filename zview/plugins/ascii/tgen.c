#include <stdio.h>

static unsigned char const weight_table[128] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x14, 0x18, 0x38, 0x30, 0x20, 0x3c, 0x0c,
	0x1a, 0x1a, 0x30, 0x18, 0x0b, 0x0c, 0x08, 0x18,
	0x36, 0x22, 0x2a, 0x2a, 0x2a, 0x2f, 0x2f, 0x20,
	0x34, 0x2d, 0x10, 0x13, 0x1b, 0x18, 0x1b, 0x20,
	0x34, 0x34, 0x39, 0x2c, 0x34, 0x2e, 0x26, 0x33,
	0x34, 0x28, 0x22, 0x30, 0x20, 0x39, 0x38, 0x34,
	0x2e, 0x33, 0x36, 0x2a, 0x20, 0x32, 0x2c, 0x38,
	0x2c, 0x24, 0x28, 0x20, 0x18, 0x20, 0x18, 0x0e,
	0x0f, 0x29, 0x30, 0x1e, 0x30, 0x27, 0x23, 0x33,
	0x2d, 0x1c, 0x1f, 0x2e, 0x1e, 0x2d, 0x26, 0x28,
	0x2e, 0x2e, 0x1b, 0x24, 0x21, 0x27, 0x20, 0x2c,
	0x22, 0x30, 0x22, 0x24, 0x1c, 0x24, 0x12, 0x1c
};
static unsigned char color_table[128];

int main(void)
{
	unsigned int maxc;
	unsigned int i;
	double scale;

	maxc = 0;
	for (i = 0; i < 128; i++)
	{
		maxc = weight_table[i] < maxc ? maxc : weight_table[i];
	}
	scale = (double)(255 / maxc);
	for (i = 0; i < 128; i++)
	{
		color_table[i] = weight_table[i] * scale;
	}
	
	for (i = 0; i < 128; i++)
	{
		printf("0x%02x, ", color_table[i]);
		if ((i % 8) == 7)
			printf("\n");
	}
	
	return 0;
}


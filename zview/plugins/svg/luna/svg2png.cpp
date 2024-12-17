#include <lunasvg.h>

#include <iostream>
#include <sstream>
#include <errno.h>
#include <string.h>
#include <zlib.h>

using namespace lunasvg;

int help()
{
    std::cout << "Usage: \n"
                 "   svg2png [filename] [resolution] [bgColor]\n\n"
                 "Examples: \n"
                 "    $ svg2png input.svg\n"
                 "    $ svg2png input.svg 512x512\n"
                 "    $ svg2png input.svg 512x512 0xff00ffff\n\n";
    return 1;
}

bool setup(int argc, char** argv, std::string& filename, std::uint32_t& width, std::uint32_t& height, std::uint32_t& bgColor)
{
    if(argc > 1) filename.assign(argv[1]);
    if(argc > 2) {
        std::stringstream ss;
        ss << argv[2];
        ss >> width;

        if(ss.fail() || ss.get() != 'x')
            return false;
        ss >> height;
    }

    if(argc > 3) {
        std::stringstream ss;
        ss << std::hex << argv[3];
        ss >> std::hex >> bgColor;
    }

    return argc > 1;
}

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
static uint32_t swap32(uint32_t l)
{
	return ((l >> 24) & 0xffL) | ((l << 8) & 0xff0000L) | ((l >> 8) & 0xff00L) | ((l << 24) & 0xff000000UL);
}
#endif

int main(int argc, char* argv[])
{
    std::string filename;
    std::uint32_t width = 0, height = 0;
    std::uint32_t bgColor = 0xffffffff;
    std::uint8_t *bmap;
    size_t image_size;
    FILE *fp;
    uint8_t magic[2];
    char *data;
	uint32_t file_size;
    
    if(!setup(argc, argv, filename, width, height, bgColor)) {
        return help();
    }

	fp = fopen(filename.c_str(), "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "%s: %s\n", filename.c_str(), strerror(errno));
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
		data = new char[file_size + 1];
		if (data == NULL)
		{
			fprintf(stderr, "%s: %s\n", filename.c_str(), strerror(errno));
			return 1;
		}
		
		fseek(fp, 0, SEEK_SET);
		gzfile = gzdopen(fileno(fp), "r");
		if (gzfile == NULL)
		{
			fprintf(stderr, "%s: %s\n", filename.c_str(), strerror(errno));
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
		data = new char[file_size + 1];
		if (fread(data, 1, file_size, fp) != file_size)
		{
			fprintf(stderr, "%s: %s\n", filename.c_str(), strerror(errno));
			return 1;
		}
		fclose(fp);
	}
	data[file_size] = '\0';
		
    auto document = Document::loadFromData(data, file_size);
    if(document == nullptr) {
        return help();
    }
	delete [] data;

	float xScale = 1.0f;
	float yScale = 1.0f;
	if (width == 0 || height == 0)
	{
		width = document->width();
		height = document->height();
	} else
	{
		xScale = (float)width / document->width();
		yScale = (float)height / document->height();
	}
	
	image_size = (size_t)width * height * 4;
	bmap = new std::uint8_t[image_size];
	
	Matrix matrix(xScale, 0, 0, yScale, 0, 0);

	Bitmap bitmap(bmap, width, height, (size_t)width * 4);
	if (bitmap.isNull())
        return help();
	bitmap.clear(bgColor);

	document->render(bitmap, matrix);

    auto lastSlashIndex = filename.find_last_of("/\\");
    auto basename = lastSlashIndex == std::string::npos ? filename : filename.substr(lastSlashIndex + 1);
    basename.append(".png");

    bitmap.writeToPng(basename);
    std::cout << "Generated PNG file: " << basename << std::endl;
    
    delete [] bmap;
    
    return 0;
}

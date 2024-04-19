//
// Created by Fedor Bondarev on 26.04.2023.
//

#include "png_reader.h"

#include "../deflate_decompressor/deflate_decompressor.h"
#include "../typedefs.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define IHDR_NAME 1229472850u
#define PLTE_NAME 1347179589u
#define IDAT_NAME 1229209940u
#define IEND_NAME 1229278788u

#define PNG_SIGNATURE 9894494448401390090ull

#define PNG_COLOR_TYPE_INDEXED 3
#define PNG_COLOR_TYPE_TRUECOLOR 2
#define PNG_COLOR_TYPE_GRAYSCALE 0

#define CHAR_ARRAY_TO_ULL_BE(x)                                                                                        \
	(((unsigned long long)(x)[0] << 56) + ((unsigned long long)(x)[1] << 48) + ((unsigned long long)(x)[2] << 40) +    \
	 ((unsigned long long)(x)[3] << 32) + ((unsigned long long)(x)[4] << 24) + ((unsigned long long)(x)[5] << 16) +    \
	 ((unsigned long long)(x)[6] << 8) + (unsigned long long)(x)[7])

#define CHAR_ARRAY_TO_UINT_BE(x)                                                                                       \
	(((unsigned int)(x)[0] << 24) + ((unsigned int)(x)[1] << 16) + ((unsigned int)(x)[2] << 8) + (unsigned int)(x)[3])

typedef struct
{
	uchar R, G, B;
} PaletteColor;

typedef struct
{
	bool is_IHDR_read;
	bool is_PLTE_read;
	bool is_IDAT_chunks_started;
	bool is_IEND_read;

	uchar png_color_type;

	PaletteColor palette[256];
	ushort palette_size;

	uchar *read_data;
	ull read_data_size;

	uchar *decompressed_data;
	ull decompressed_data_size;

	ull target_decompressed_data_size;

	Image image;
} Context;

static Context context;

int ReadChunkIHDR(FILE *in_file, uint chunk_size)
{
	if (chunk_size != 13)	 // IHDR Chunk size
	{
		return PNG_READER_ERROR_INVALID_DATA;
	}

	uchar next_4_bytes[4];
	uint read_bytes_count;

	read_bytes_count = fread(next_4_bytes, sizeof(*next_4_bytes), 4, in_file);

	if (read_bytes_count != 4 * sizeof(*next_4_bytes))
	{
		return PNG_READER_ERROR_INVALID_DATA;
	}

	context.image.width = CHAR_ARRAY_TO_UINT_BE(next_4_bytes);

	read_bytes_count = fread(next_4_bytes, sizeof(*next_4_bytes), 4, in_file);

	if (read_bytes_count != 4 * sizeof(*next_4_bytes))
	{
		return PNG_READER_ERROR_INVALID_DATA;
	}

	context.image.height = CHAR_ARRAY_TO_UINT_BE(next_4_bytes);

	uchar bit_depth;
	read_bytes_count = fread(&bit_depth, sizeof(bit_depth), 1, in_file);

	if (read_bytes_count != sizeof(bit_depth))
	{
		return PNG_READER_ERROR_INVALID_DATA;
	}

	if (bit_depth != 8)
	{
		return PNG_READER_ERROR_UNSUPPORTED;
	}

	read_bytes_count = fread(&(context.png_color_type), sizeof(context.png_color_type), 1, in_file);

	if (read_bytes_count != sizeof(*next_4_bytes))
	{
		return PNG_READER_ERROR_INVALID_DATA;
	}

	if (context.png_color_type != PNG_COLOR_TYPE_GRAYSCALE && context.png_color_type != PNG_COLOR_TYPE_TRUECOLOR &&
		context.png_color_type != PNG_COLOR_TYPE_INDEXED)
	{
		return PNG_READER_ERROR_UNSUPPORTED;
	}

	uchar compression_method;
	read_bytes_count = fread(&compression_method, sizeof(compression_method), 1, in_file);

	if (read_bytes_count != sizeof(compression_method))
	{
		return PNG_READER_ERROR_INVALID_DATA;
	}

	if (compression_method != 0)
	{
		return PNG_READER_ERROR_UNSUPPORTED;
	}

	uchar filter_method;
	read_bytes_count = fread(&filter_method, sizeof(filter_method), 1, in_file);

	if (read_bytes_count != sizeof(compression_method))
	{
		return PNG_READER_ERROR_INVALID_DATA;
	}

	if (filter_method != 0)
	{
		return PNG_READER_ERROR_UNSUPPORTED;
	}

	uchar interlace_method;
	read_bytes_count = fread(&interlace_method, sizeof(interlace_method), 1, in_file);

	if (read_bytes_count != sizeof(interlace_method))
	{
		return PNG_READER_ERROR_INVALID_DATA;
	}

	if (interlace_method != 0)
	{
		return PNG_READER_ERROR_UNSUPPORTED;
	}

	if (context.png_color_type == PNG_COLOR_TYPE_GRAYSCALE || context.png_color_type == PNG_COLOR_TYPE_INDEXED)
	{
		context.image.color_type = 0;
	}
	else
	{
		context.image.color_type = 1;
	}

	context.is_IHDR_read = true;

	uint pixel_size = context.png_color_type == PNG_COLOR_TYPE_TRUECOLOR ? 3 : 1;
	context.target_decompressed_data_size = (context.image.width * pixel_size + 1) * context.image.height;

	return PNG_READER_SUCCESS;
}

int ReadChunkPLTE(FILE *in_file, uint chunk_size)
{
	if (!context.is_IHDR_read || chunk_size % 3 != 0 || context.png_color_type == PNG_COLOR_TYPE_GRAYSCALE)
	{
		return PNG_READER_ERROR_INVALID_DATA;
	}

	if (context.png_color_type != PNG_COLOR_TYPE_INDEXED)
	{
		fseek(in_file, chunk_size, SEEK_CUR);
		goto end;
	}

	context.palette_size = chunk_size / 3;

	for (int i = 0; i < context.palette_size; ++i)
	{
		ull read_bytes_count;

		read_bytes_count = fread(&(context.palette)[i].R, sizeof((context.palette)[i].R), 1, in_file);
		if (read_bytes_count != sizeof((context.palette)[i].R))
		{
			return PNG_READER_ERROR_INVALID_DATA;
		}

		read_bytes_count = fread(&(context.palette)[i].G, sizeof((context.palette)[i].G), 1, in_file);
		if (read_bytes_count != sizeof((context.palette)[i].G))
		{
			return PNG_READER_ERROR_INVALID_DATA;
		}

		read_bytes_count = fread(&(context.palette)[i].B, sizeof((context.palette)[i].B), 1, in_file);
		if (read_bytes_count != sizeof((context.palette)[i].B))
		{
			return PNG_READER_ERROR_INVALID_DATA;
		}

		if (context.image.color_type == PNG_COLOR_TYPE_GRAYSCALE &&
			!((context.palette)[i].R == (context.palette)[i].G && (context.palette)[i].G == (context.palette)[i].B))
		{
			context.image.color_type = 1;
		}
	}

end:
	context.is_PLTE_read = true;
	return PNG_READER_SUCCESS;
}

int ReadChunkIDAT(FILE *in_file, uint chunk_size)
{
	if (!context.is_IHDR_read)
	{
		return PNG_READER_ERROR_INVALID_DATA;
	}

	if (context.png_color_type == PNG_COLOR_TYPE_INDEXED && !context.is_PLTE_read)
	{
		return PNG_READER_ERROR_INVALID_DATA;
	}

	if (!context.is_IDAT_chunks_started)
	{
		context.is_IDAT_chunks_started = true;

		context.read_data = malloc(chunk_size);

		if (context.read_data == NULL)
		{
			return PNG_READER_ERROR_OUT_OF_MEMORY;
		}
	}
	else
	{
		uchar *new_ptr = realloc(context.read_data, context.read_data_size + chunk_size);

		if (new_ptr == NULL)
		{
			return PNG_READER_ERROR_OUT_OF_MEMORY;
		}

		context.read_data = new_ptr;
	}

	fread(&context.read_data[context.read_data_size], 1, chunk_size, in_file);
	context.read_data_size += chunk_size;

	return PNG_READER_SUCCESS;
}

int ReadChunkIEND(FILE *in_file, uint chunk_size)
{
	fseek(in_file, chunk_size, SEEK_CUR);

	context.is_IEND_read = true;

	return PNG_READER_SUCCESS;
}

int ReadPngChunks(FILE *in_file)
{
	uchar next_8_bytes[8];
	uint chunk_size, chunk_type_name;
	ull read_bytes_count;

	while (!feof(in_file) && !context.is_IEND_read)
	{
		read_bytes_count = fread(next_8_bytes, sizeof(*next_8_bytes), 4, in_file);

		if (read_bytes_count != 4 * sizeof(*next_8_bytes))
		{
			return PNG_READER_ERROR_INVALID_DATA;
		}

		chunk_size = CHAR_ARRAY_TO_UINT_BE(next_8_bytes);

		read_bytes_count = fread(next_8_bytes, sizeof(*next_8_bytes), 4, in_file);

		if (read_bytes_count != 4 * sizeof(*next_8_bytes))
		{
			return PNG_READER_ERROR_INVALID_DATA;
		}

		chunk_type_name = CHAR_ARRAY_TO_UINT_BE(next_8_bytes);

		int read_chunk_code = PNG_READER_SUCCESS;

		switch (chunk_type_name)
		{
		case IHDR_NAME:
			read_chunk_code = ReadChunkIHDR(in_file, chunk_size);
			break;
		case IDAT_NAME:
			read_chunk_code = ReadChunkIDAT(in_file, chunk_size);
			break;
		case PLTE_NAME:
			read_chunk_code = ReadChunkPLTE(in_file, chunk_size);
			break;
		case IEND_NAME:
			read_chunk_code = ReadChunkIEND(in_file, chunk_size);
			break;
		default:
			fseek(in_file, chunk_size, SEEK_CUR);	 // Ignoring chunk
			break;
		}

		if (read_chunk_code != PNG_READER_SUCCESS)
		{
			return read_chunk_code;
		}

		fseek(in_file, 4, SEEK_CUR);
	}

	return PNG_READER_SUCCESS;
}

void ApplySubFilter(uint data_size, uchar const *data, int pixel_size, uchar *out)
{
	for (int i = 0; i < data_size; ++i)
	{
		uchar previous_pixel_byte = (i - pixel_size < 0) ? 0 : out[i - pixel_size];
		out[i] = data[i] + previous_pixel_byte;
	}
}

void ApplyUpFilter(uint data_size, uchar const *data, uchar const *unfiltered_previous_line, uchar *out)
{
	for (int i = 0; i < data_size; ++i)
	{
		out[i] = data[i] + unfiltered_previous_line[i];
	}
}

void ApplyAverageFilter(uint data_size, uchar const *data, int pixel_size, uchar const *unfiltered_previous_line, uchar *out)
{
	for (int i = 0; i < data_size; ++i)
	{
		uchar previous_pixel_byte = (i - pixel_size) < 0 ? 0 : out[i - pixel_size];
		out[i] = data[i] + ((unfiltered_previous_line[i] + previous_pixel_byte) / 2);
	}
}

uint PaethPredictor_(int a, int b, int c)
{
	int p = a + b - c;

	int pa = abs(p - a);
	int pb = abs(p - b);
	int pc = abs(p - c);

	if (pa <= pb && pa <= pc)
	{
		return a;
	}
	else if (pb <= pc)
	{
		return b;
	}

	return c;
}

void ApplyPaethFilter(uint data_size, uchar const *data, int pixel_size, uchar const *unfiltered_previous_line, uchar *out)
{
	for (int i = 0; i < data_size; ++i)
	{
		uchar previous_pixel_byte, previous_line_previous_pixel_byte;
		if ((i - pixel_size) < 0)
		{
			previous_pixel_byte = 0;
			previous_line_previous_pixel_byte = 0;
		}
		else
		{
			previous_pixel_byte = out[i - pixel_size];
			previous_line_previous_pixel_byte = unfiltered_previous_line[i - pixel_size];
		}

		out[i] = data[i] + (uchar)PaethPredictor_(previous_pixel_byte, unfiltered_previous_line[i], previous_line_previous_pixel_byte);
	}
}

int ProcessDecompressedPngData(void)
{
	int png_pixel_size = context.png_color_type == PNG_COLOR_TYPE_TRUECOLOR ? 3 : 1;
	uint scanline_size = png_pixel_size * context.image.width;

	uint out_image_pixel_size = context.image.color_type == 1 ? 3 : 1;
	uint out_image_line_size = out_image_pixel_size * context.image.width;

	context.image.data = malloc(out_image_pixel_size * context.image.height * context.image.width);
	if (context.image.data == NULL)
	{
		return PNG_READER_ERROR_OUT_OF_MEMORY;
	}

	uchar *unfiltered_data = malloc(scanline_size);
	if (unfiltered_data == NULL)
	{
		return PNG_READER_ERROR_OUT_OF_MEMORY;
	}

	uchar *unfiltered_previous_line = calloc(scanline_size, sizeof(uchar));
	if (unfiltered_previous_line == NULL)
	{
		free(unfiltered_data);
		return PNG_READER_ERROR_OUT_OF_MEMORY;
	}

	int result = PNG_READER_SUCCESS;

	for (int i = 0; i < context.image.height; ++i)
	{
		uchar filter_type = context.decompressed_data[i * (scanline_size + 1)];

		uchar *scanline = &context.decompressed_data[i * (scanline_size + 1) + 1];

		switch (filter_type)
		{
		case 0:
			for (int j = 0; j < scanline_size; ++j)
			{
				unfiltered_data[j] = scanline[j];
			}
			break;
		case 1:
			ApplySubFilter(scanline_size, scanline, png_pixel_size, unfiltered_data);
			break;
		case 2:
			ApplyUpFilter(scanline_size, scanline, unfiltered_previous_line, unfiltered_data);
			break;
		case 3:
			ApplyAverageFilter(scanline_size, scanline, png_pixel_size, unfiltered_previous_line, unfiltered_data);
			break;
		case 4:
			ApplyPaethFilter(scanline_size, scanline, png_pixel_size, unfiltered_previous_line, unfiltered_data);
			break;
		default:
			result = PNG_READER_ERROR_INVALID_DATA;
			goto end;
		}

		switch (context.png_color_type)
		{
		case PNG_COLOR_TYPE_GRAYSCALE:
		case PNG_COLOR_TYPE_TRUECOLOR:
			for (int j = 0; j < scanline_size; ++j)
			{
				unfiltered_previous_line[j] = unfiltered_data[j];

				context.image.data[i * out_image_line_size + j] = unfiltered_data[j];
			}
			break;
		case PNG_COLOR_TYPE_INDEXED:
			for (int j = 0; j < scanline_size; ++j)
			{
				unfiltered_previous_line[j] = unfiltered_data[j];

				if (unfiltered_data[j] > context.palette_size)
				{
					result = PNG_READER_ERROR_INVALID_DATA;
					goto end;
				}

				if (context.image.color_type == 0)
				{
					context.image.data[i * out_image_line_size + j] = context.palette[unfiltered_data[j]].R;
				}
				else
				{
					context.image.data[i * out_image_line_size + (j * 3)] = context.palette[unfiltered_data[j]].R;
					context.image.data[i * out_image_line_size + (j * 3) + 1] = context.palette[unfiltered_data[j]].G;
					context.image.data[i * out_image_line_size + (j * 3) + 2] = context.palette[unfiltered_data[j]].B;
				}
			}
			break;
		}
	}

	free(context.decompressed_data);
	context.decompressed_data = NULL;

end:
	if (unfiltered_previous_line)
	{
		free(unfiltered_previous_line);
	}

	if (unfiltered_data)
	{
		free(unfiltered_data);
	}

	return result;
}

int ReleaseContext(int code)
{
	if (context.read_data)
	{
		free(context.read_data);
		context.read_data = NULL;
	}

	if (context.decompressed_data)
	{
		free(context.decompressed_data);
		context.decompressed_data = NULL;
	}

	if (context.image.data && code != PNG_READER_SUCCESS)
	{
		free(context.image.data);
		context.image.data = NULL;
	}

	return code;
}

int ReadPng(FILE *in_file, Image *image)
{
	context = (Context){};

	int code;
	uchar next_8_bytes[8];

	fread(next_8_bytes, sizeof(*next_8_bytes), 8, in_file);

	if (PNG_SIGNATURE != CHAR_ARRAY_TO_ULL_BE(next_8_bytes))
	{
		return ReleaseContext(PNG_READER_ERROR_INVALID_DATA);
	}

	code = ReadPngChunks(in_file);
	if (code != PNG_READER_SUCCESS)
	{
		return ReleaseContext(code);
	}

	context.decompressed_data = malloc(context.target_decompressed_data_size);
	if (context.decompressed_data == NULL)
	{
		return ReleaseContext(PNG_READER_ERROR_OUT_OF_MEMORY);
	}

	int decompress_data_code =
		DecompressData(context.read_data, context.read_data_size, context.decompressed_data, context.target_decompressed_data_size);

	free(context.read_data);
	context.read_data = NULL;
	context.read_data_size = 0;

	if (decompress_data_code != DEFLATE_DECOMPRESSOR_SUCCESS)
	{
		switch (decompress_data_code)
		{
		case DEFLATE_DECOMPRESSOR_ERROR_DATA_INVALID:
			return ReleaseContext(PNG_READER_ERROR_INVALID_DATA);
		case DEFLATE_DECOMPRESSOR_ERROR_OUT_OF_MEMORY:
			return ReleaseContext(PNG_READER_ERROR_OUT_OF_MEMORY);
		default:
			return ReleaseContext(PNG_READER_ERROR_UNKNOWN);
		}
	}

	code = ProcessDecompressedPngData();
	if (code != PNG_READER_SUCCESS)
	{
		return ReleaseContext(code);
	}

	*image = context.image;

	return ReleaseContext(PNG_READER_SUCCESS);
}

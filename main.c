//
// Created by Fedor Bondarev on 24.04.2023.
//

#include "lib/png_reader/png_reader.h"
#include "lib/pnm_writer/pnm_writer.h"
#include "return_codes.h"

#include <stdio.h>
#include <stdlib.h>

char const *error_message_table[] = {
	[SUCCESS] = "",
	[ERROR_CANNOT_OPEN_FILE] = "File cannot be opened",
	[ERROR_OUT_OF_MEMORY] = "Not enough memory, memory allocation failed",
	[ERROR_DATA_INVALID] = "The data is invalid",
	[ERROR_PARAMETER_INVALID] = "The parameter or number of parameters is incorrect",
	[ERROR_UNSUPPORTED] = "Unsupported functionality",
};

int const png_reader_error_codes_table[] = {
	[PNG_READER_SUCCESS] = SUCCESS,
	[PNG_READER_ERROR_INVALID_DATA] = ERROR_DATA_INVALID,
	[PNG_READER_ERROR_UNSUPPORTED] = ERROR_UNSUPPORTED,
	[PNG_READER_ERROR_OUT_OF_MEMORY] = ERROR_OUT_OF_MEMORY,
	[PNG_READER_ERROR_UNKNOWN] = ERROR_UNKNOWN,
};

int ReleaseEndCode(int code)
{
	if (code < 0)
	{
		return code;
	}

	if (code != SUCCESS)
	{
		fprintf(stderr, "%s\n", error_message_table[code]);
	}

	return code;
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		return ReleaseEndCode(ERROR_PARAMETER_INVALID);
	}

	FILE *in_file = fopen(argv[1], "rb");

	if (in_file == NULL)
	{
		return ReleaseEndCode(ERROR_CANNOT_OPEN_FILE);
	}

	Image image;
	int png_read_code = ReadPng(in_file, &image);

	fclose(in_file);

	if (png_read_code != PNG_READER_SUCCESS)
	{
		return ReleaseEndCode(png_reader_error_codes_table[png_read_code]);
	}

	FILE *out_file = fopen(argv[2], "wb");

	if (out_file == NULL)
	{
		free(image.data);
		return ReleaseEndCode(ERROR_CANNOT_OPEN_FILE);
	}

	WritePnm(out_file, image);

	free(image.data);
	fclose(out_file);

	return ReleaseEndCode(SUCCESS);
}
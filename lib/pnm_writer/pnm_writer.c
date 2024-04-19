//
// Created by Fedor Bondarev on 27.04.2023.
//

#include "pnm_writer.h"

#include "../typedefs.h"

#include <stdio.h>

void WritePnm(FILE *out_file, Image image)
{
	uint pnm_type = image.color_type ? 6 : 5;

	char out[29];
	uint size = snprintf(out, 29, "P%u\n%u %u\n255\n", pnm_type, image.width, image.height);
	fwrite(out, 1, size, out_file);

	uint pixel_size = image.color_type ? 3 : 1;
	fwrite(image.data, 1, pixel_size * image.width * image.height, out_file);
}

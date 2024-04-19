//
// Created by Fedor Bondarev on 26.04.2023.
//

#ifndef ITMO_C_PNG_FEDORBONDAREV__IMAGE_DATA_H_
#define ITMO_C_PNG_FEDORBONDAREV__IMAGE_DATA_H_

typedef struct
{
	unsigned int height;
	unsigned int width;

	/*
	 *
	 * 0 - gray images
	 * 1 - color images
	 * */
	unsigned char color_type;

	unsigned char *data;
} Image;

#endif	  // ITMO_C_PNG_FEDORBONDAREV__IMAGE_DATA_H_

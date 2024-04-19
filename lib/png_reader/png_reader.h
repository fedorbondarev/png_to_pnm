//
// Created by Fedor Bondarev on 26.04.2023.
//

#ifndef ITMO_C_PNG_FEDORBONDAREV_PNG_READER_PNG_READER_H_
#define ITMO_C_PNG_FEDORBONDAREV_PNG_READER_PNG_READER_H_

#include "../image.h"

#include <stdio.h>

#define PNG_READER_SUCCESS 0
#define PNG_READER_ERROR_INVALID_DATA 1
#define PNG_READER_ERROR_UNSUPPORTED 2
#define PNG_READER_ERROR_OUT_OF_MEMORY 3
#define PNG_READER_ERROR_UNKNOWN 4

/*
 *
 * Return value:
 * 0 - Success (PNG_READER_SUCCESS)
 * 1 - Error, png data invalid (PNG_READER_ERROR_INVALID_DATA)
 * 2 - Error, unsupported png type (PNG_READER_ERROR_UNSUPPORTED)
 * */
int ReadPng(FILE *in_file, Image *image);

#endif	  // ITMO_C_PNG_FEDORBONDAREV_PNG_READER_PNG_READER_H_

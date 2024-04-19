//
// Created by Fedor Bondarev on 26.04.2023.
//

#include "deflate_decompressor.h"

#include "../typedefs.h"

#include <zlib.h>

int DecompressDataZlib_(uchar *in, uint in_size, uchar *out, ull target_out_size)
{
	int ret;
	z_stream strm;

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = in_size;
	strm.next_in = in;
	strm.avail_out = target_out_size;
	strm.next_out = out;

	ret = inflateInit(&strm);

	if (ret != Z_OK)
	{
		switch (ret)
		{
		case Z_DATA_ERROR:
			return DEFLATE_DECOMPRESSOR_ERROR_DATA_INVALID;

		case Z_MEM_ERROR:
			return DEFLATE_DECOMPRESSOR_ERROR_OUT_OF_MEMORY;

		default:
			return DEFLATE_DECOMPRESSOR_ERROR_UNKNOWN;
		}
	}

	ret = inflate(&strm, Z_NO_FLUSH);

	switch (ret)
	{
	case Z_STREAM_ERROR:
		return DEFLATE_DECOMPRESSOR_ERROR_DATA_INVALID;

	case Z_DATA_ERROR:
		inflateEnd(&strm);
		return DEFLATE_DECOMPRESSOR_ERROR_DATA_INVALID;

	case Z_MEM_ERROR:
		inflateEnd(&strm);
		return DEFLATE_DECOMPRESSOR_ERROR_OUT_OF_MEMORY;

	default:
		break;
	}

	inflateEnd(&strm);

	if (ret != Z_STREAM_END)
	{
		return DEFLATE_DECOMPRESSOR_ERROR_DATA_INVALID;
	}

	return DEFLATE_DECOMPRESSOR_SUCCESS;
}

int DecompressData(uchar *in, uint in_size, uchar *out, ull target_out_size)
{
	if (in == NULL || out == NULL || in_size == 0 || target_out_size == 0)
	{
		return DEFLATE_DECOMPRESSOR_ERROR_DATA_INVALID;
	}
	return DecompressDataZlib_(in, in_size, out, target_out_size);
}

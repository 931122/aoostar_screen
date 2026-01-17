/********************************************************************
* file: image.c                       date: Mon 2025-07-14 13:15:38 *
*                                                                   *
* Description:                                                      *
*                                                                   *
*                                                                   *
* Maintainer:  (yinxianglu)  <yinxianglu1993@gmail.com>             *
*                                                                   *
* This file is free software;                                       *
*   you are free to modify and/or redistribute it                   *
*   under the terms of the GNU General Public Licence (GPL).        *
*                                                                   *
* Last modified:                                                    *
*                                                                   *
* No warranty, no liability, use this at your own risk!             *
********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif
#include "stdint.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "draw.h"

int load_image_to_565(const char *path, unsigned char *img, int *width, int *height)
{
	int channels;
	int i;
	short *p = (short *)img;

	uint8_t *image = stbi_load(path, width, height, &channels, 3);
	if (image == NULL)
	{
		fprintf(stderr, "加载图像失败: %s\n", stbi_failure_reason());
		return -1;
	}

	for (i = 0; i < *width * *height; ++i)
	{
		uint8_t r = image[i * 3 + 0];
		uint8_t g = image[i * 3 + 1];
		uint8_t b = image[i * 3 + 2];
		*p++ = rgb565(r, g, b);
	}
	free(image);
	return 0;
}



#ifdef __cplusplus
};
#endif
/********************* End Of File: image.c *********************/

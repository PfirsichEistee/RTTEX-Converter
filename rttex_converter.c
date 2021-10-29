#include "rttex_converter.h"
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <png.h>


/*
RTTEX stores 2 kinds of sizes:
1. reserved image size: bigger or equal to the actual image size (sizes 2^n ?)
2. actual image size: basically the rectangle in which the stored pixels actually matter

Memory visualization:
XXXXXXXXX???
XXXXXXXXX???
XXXXXXXXX???
XXXXXXXXX???
????????????
????????????

X are important pixels
? are unnecessary values aka a waste of space
*/


int
main(int argc, char** arg)
{
	if (argc != 3)
	{
		printf("Usage: rttex_converter <PATH_TO_INPUT_RTTEX> <PATH_TO_OUTPUT_PNG>\n");
		return 1;
	}
	
	RT_FILE_DATA fdata = rt_read_file(arg[1]);
	
	if (rt_is_rtpack(fdata))
	{
		char status = rt_rtpack_unpack(&fdata);
		if (!status)
		{
			printf("Could not unpack RTPACK!\n");
			return 1;
		}
	}
	
	if (!rt_is_rttex(fdata))
	{
		printf("File is not a RTTEX!\n");
		return 1;
	}
	
	
	printf("Size (saved): %dx%d\n", rt_get_width(fdata, 1), rt_get_height(fdata, 1));
	printf("Size: %dx%d\n", rt_get_width(fdata, 0), rt_get_height(fdata, 0));
	
	char status = rt_make_png_cropped(	fdata,
										arg[2],
										rt_get_width(fdata, 0),
										rt_get_height(fdata, 0));
	if (!status)
	{
		printf("Failed to create PNG!\n");
		return 1;
	}
	
	
	// Print header for debug
	/*for (unsigned int y = 0; y < 5; y++)
	{
		for (unsigned int x = 0; x < 16; x++)
		{
			unsigned int i = y * 16 + x;
			
			if (i < fdata.size)
			{
				if (fdata.data[i] > 0xF)
					printf("%x  ", fdata.data[i]);
				else
					printf("0%x  ", fdata.data[i]);
			}
		}
		
		printf("\n");
	}*/
	
	free(fdata.data);
	
	return 0;
}


RT_FILE_DATA
rt_read_file(char* file_path)
{
	// Open file
	FILE* file = fopen(file_path, "r"); // read-mode
	
	if (file == NULL)
	{
		printf("Could not read file.\n");
		exit(1);
	}
	
	
	// Get file size
	fseek(file, 0, SEEK_END); // go to the end of the file
	int file_size = ftell(file); // get current position (-> end)
	fseek(file, 0, SEEK_SET); // move to beginning
	
	unsigned char* data = malloc(file_size); // note: sizeof(char) == 1. Always.
	
	// Read data
	for (int i = 0; i < file_size; i++)
	{
		data[i] = fgetc(file);
	}
	
	// Close file
	fclose(file);
	
	// Return
	//printf("Read file: %d Bytes\n", file_size);
	
	RT_FILE_DATA fd = {
		.data = data,
		.size = file_size
	};
	return fd;
}




unsigned char
rt_is_rtpack(RT_FILE_DATA fdata)
{
	static unsigned char* pack_str = "RTPACK";
	
	for (int i = 0; i < 6; i++)
	{
		if (pack_str[i] != fdata.data[i])
			return 0;
	}
	
	return 1;
}

unsigned char
rt_is_rttex(RT_FILE_DATA fdata)
{
	static unsigned char* pack_str = "RTTXTR";
	
	for (int i = 0; i < 6; i++)
	{
		if (pack_str[i] != fdata.data[i])
			return 0;
	}
	
	return 1;
}




unsigned char
rt_rtpack_unpack(RT_FILE_DATA* fdata)
{
	int status = 0;
	
	z_stream strm = {
		.zalloc = Z_NULL,
		.zfree = Z_NULL,
		.opaque = Z_NULL,
		.avail_in = 0,
		.next_in = Z_NULL
	};
	
	// Initialize
	status = inflateInit(&strm);
	if (status != Z_OK)
		return 0;
	
	// Prepare data
	unsigned int input_size = rt_read_number(*fdata, 0xB, 0x8);
	unsigned int output_size = rt_read_number(*fdata, 0xF, 0xC);
	
	// ..data array without RTPACK header
	unsigned char input_data[input_size];
	for (unsigned int i = 32; i < fdata->size; i++)
	{
		input_data[i - 32] = fdata->data[i];
	}
	
	unsigned char output_data[output_size];
	
	
	// Prepare stream
	strm.avail_in = input_size;
	strm.next_in = input_data;
	
	strm.avail_out = output_size;
	strm.next_out = output_data;
	
	// Inflate (decrypt)
	status = inflate(&strm, Z_NO_FLUSH);
	
	// Check for fail
	switch (status)
	{
		case (Z_STREAM_ERROR):
		case (Z_NEED_DICT):
		case (Z_DATA_ERROR):
		case (Z_MEM_ERROR):
			inflateEnd(&strm);
			return 0;
	}
	
	// Did we get all of the data? (should always be the case)
	if (strm.avail_out != 0)
		return 0;
	
	// Finalize
	inflateEnd(&strm);
	
	
	// Replace data
	fdata->size = output_size;
	fdata->data = realloc(fdata->data, output_size);
	for (unsigned int i = 0; i < output_size; i++)
	{
		fdata->data[i] = output_data[i];
	}
	
	return 1;
}





unsigned int
rt_get_width(	RT_FILE_DATA	fdata,
				unsigned char	saved_size)
{
	if (saved_size)
		return rt_read_number(fdata, 0x0F, 0x0C);
	else
		return rt_read_number(fdata, 0x1B, 0x18);
}

unsigned int
rt_get_height(	RT_FILE_DATA	fdata,
				unsigned char	saved_size)
{
	if (saved_size)
		return rt_read_number(fdata, 0x0B, 0x08);
	else
		return rt_read_number(fdata, 0x17, 0x14);
}




unsigned int
rt_read_number(	RT_FILE_DATA	fdata,
				unsigned int	start_adr,
				unsigned int	end_adr)
{
	unsigned int num = 0;
	
	int dir = 1;
	if (end_adr < start_adr)
		dir = -1;
	
	unsigned int adr = start_adr;
	num = fdata.data[adr];
	while (adr != end_adr)
	{
		adr += dir;
		
		num = num << 8;
		num = num | fdata.data[adr];
	}
	
	return num;
}




void
rt_print_data(RT_FILE_DATA fdata)
{
	for (unsigned int y = 0; y < (fdata.size / 16); y++)
	{
		for (unsigned int x = 0; x < 16; x++)
		{
			unsigned int i = y * 16 + x;
			
			if (i < fdata.size)
			{
				if (fdata.data[i] > 0xF)
					printf("%x  ", fdata.data[i]);
				else
					printf("0%x  ", fdata.data[i]);
			}
		}
		
		printf("\n");
	}
}




void
rt_get_pixel(	RT_FILE_DATA	fdata,
				RT_COLOR*		clr,
				unsigned int	x,
				unsigned int	y)
{
	unsigned char has_alpha = rt_read_number(fdata, 0x1C, 0x1C);
	
	unsigned int p = fdata.size;
	if (has_alpha)
	{
		p -= (rt_get_width(fdata, 1) * rt_get_height(fdata, 1)) * 4;
		p += (y * rt_get_width(fdata, 1) + x) * 4;
	}
	else
	{
		p -= (rt_get_width(fdata, 1) * rt_get_height(fdata, 1)) * 3;
		p += (y * rt_get_width(fdata, 1) + x) * 3;
	}
	
	
	if (p >= fdata.size)
	{
		clr->r = 0;
		clr->g = 0;
		clr->b = 0;
		clr->a = 0;
	}
	else
	{
		clr->r = fdata.data[p];
		clr->g = fdata.data[p + 1];
		clr->b = fdata.data[p + 2];
		
		clr->a = 255;
		
		if (has_alpha)
			clr->a = fdata.data[p + 3];
	}
}




unsigned char
rt_make_png_cropped(	RT_FILE_DATA	fdata,
						char*			png_path,
						unsigned int	crop_width,
						unsigned int	crop_height)
{
	// w == writing; b avoids translations such as "\n" to "\r\n"
	FILE* png = fopen(png_path, "wb");
	if (png == NULL)
		return 0;
	
	// Initialize PNG writer & info
	png_structp write_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (write_ptr == NULL)
		return 0;
	
	png_infop info_ptr = png_create_info_struct(write_ptr);
	if (info_ptr == NULL)
	{
		png_destroy_write_struct(	&write_ptr,
									(png_infopp)NULL);
		return 0;
	}
	
	// Errors?
	// (libpng will jump in here if there were any. status will return != 0 in this case)
	// (setjmp && longjmp are like goto, but long)
	int status = setjmp(png_jmpbuf(write_ptr));
	if (status != 0)
	{
		png_destroy_write_struct(&write_ptr, &info_ptr);
		fclose(png);
		return 0;
	}
	
	// Prepare I/O
	png_init_io(write_ptr, png);
	
	//unsigned int width = rt_get_width(fdata, 1);
	//unsigned int height = rt_get_width(fdata, 1);
	
	png_set_IHDR(	write_ptr,
					info_ptr,
					crop_width,
					crop_height,
					8, // 8 bits per channel
					PNG_COLOR_TYPE_RGB_ALPHA,
					PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT,
					PNG_FILTER_TYPE_DEFAULT);
	
	png_write_info(write_ptr, info_ptr);
	
	// Get image as png_byte-array
	png_byte* row_pointers[crop_height];
	for (unsigned int y = 0; y < crop_height; y++)
	{
		png_byte* row = malloc(crop_width * 4);
		
		for (unsigned int x = 0; x < crop_width; x++)
		{
			RT_COLOR clr;
			
			rt_get_pixel(fdata, &clr, x, y);
			
			row[x * 4] = clr.r;
			row[x * 4 + 1] = clr.g;
			row[x * 4 + 2] = clr.b;
			row[x * 4 + 3] = clr.a;
		}
		
		row_pointers[crop_height - y - 1] = row;
	}
	
	png_write_image(write_ptr, row_pointers);
	png_write_end(write_ptr, info_ptr);
	
	
	png_destroy_write_struct(&write_ptr, &info_ptr);
	
	for (unsigned int y = 0; y < crop_height; y++)
		free(row_pointers[y]);
	
	return 1;
}

unsigned char
rt_make_png(	RT_FILE_DATA		fdata,
				char*			png_path)
{
	return rt_make_png_cropped(	fdata,
								png_path,
								rt_get_width(fdata, 1),
								rt_get_height(fdata, 1));
}




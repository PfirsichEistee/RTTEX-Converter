#ifndef RTTEX_CONVERTER_H
#define RTTEX_CONVERTER_H


typedef struct _file_data
{
	unsigned char* data;
	int size;
} RT_FILE_DATA;

typedef struct _rt_color
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
} RT_COLOR;



RT_FILE_DATA rt_read_file(char* file_path); // RT_FILE_DATA or NULL

unsigned char rt_is_rtpack(RT_FILE_DATA fdata); // 1 if success
unsigned char rt_is_rttex(RT_FILE_DATA fdata); // 1 if success

unsigned char rt_rtpack_unpack(RT_FILE_DATA* fdata); // 1 if success

unsigned int rt_get_width(	RT_FILE_DATA	fdata,
							unsigned char	saved_size); // saved_size == 1 -> use the size that was reserved in the file
unsigned int rt_get_height(	RT_FILE_DATA	fdata,
							unsigned char	saved_size);

unsigned int rt_read_number(	RT_FILE_DATA	fdata,
								unsigned int	start_adr,
								unsigned int	end_adr); // Reads bytes and turns them into an int. end_adr < start_adr is valid

void rt_print_data(RT_FILE_DATA fdata); // Prints the whole file in hex format: kind of useless

void rt_get_pixel(	RT_FILE_DATA	fdata,
					RT_COLOR*		clr,
					unsigned int	x,
					unsigned int	y); // Assigns color values to clr

unsigned char rt_make_png_cropped(	RT_FILE_DATA	fdata,
									char*			png_path,
									unsigned int	crop_width,
									unsigned int	crop_height); // 1 if success
unsigned char rt_make_png(	RT_FILE_DATA	fdata,
							char*			png_path); // 1 if success


#endif

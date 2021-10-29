# RTTEX-Converter

Read Header for API

Compile with `gcc $(pkg-config --cflags --libs zlib libpng) ./rttex_converter.c -o ./out`

Make sure to call `free(rt_file_data.data);` after you're done with it

`rt_read_number(...)` can read the bytes "backwards" [(Little Endian)](https://en.wikipedia.org/wiki/Endianness)

***

Terminal usage:

`rttex_converter <PATH_TO_INPUT_RTTEX> <PATH_TO_OUTPUT_PNG>`

***

Bugs are probably included

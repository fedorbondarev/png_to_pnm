cmake_minimum_required(VERSION 3.25)

project(png_to_pnm C)

set(CMAKE_C_STANDARD 99)

include_directories(.)

add_executable(png_to_pnm
        main.c
        return_codes.h
        lib/png_reader/png_reader.h
        lib/png_reader/png_reader.c
        lib/typedefs.h
        lib/image.h
        lib/deflate_decompressor/deflate_decompressor.c
        lib/deflate_decompressor/deflate_decompressor.h
        lib/pnm_writer/pnm_writer.c
        lib/pnm_writer/pnm_writer.h)

find_package(ZLIB REQUIRED)
target_link_libraries(png_to_pnm PRIVATE ZLIB::ZLIB)


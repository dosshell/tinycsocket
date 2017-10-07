cmake_minimum_required (VERSION 2.8)
project (tinycsocket)

include_directories(src
					)

add_library(tinycsocket
			src/tinycsocket.c
			src/tinycsocket.h
			)
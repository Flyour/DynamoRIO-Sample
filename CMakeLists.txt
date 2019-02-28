cmake_minimum_required(VERSION 2.6)


if (NOT "${DynamoRIO_DIR}" STREQUAL "")
	project(bbsize)

	set(output_dir "${PROJECT_BINARY_DIR}/bin")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${output_dir}")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${output_dir}")

	find_package(DynamoRIO)
	if (NOT DynamoRIO_FOUND)
		message(FATAL_ERROR  "DynamoRIO package required to build")
	endif (NOT DynamoRIO_FOUND)

	# add_library(bbsize SHARED  bbsize.c)
	# configure_DynamoRIO_client(bbsize)
	# use_DynamoRIO_extension(bbsize drmgr)

	add_library(bbsize SHARED bbsize.c)
	configure_DynamoRIO_client(bbsize)
	use_DynamoRIO_extension(bbsize drmgr)
endif ()


#pragma once

#include "defines.h"

/**
 * @brief reads a text file, populates out_file_size_bytes even if out_file_content is NULL.
 * This is so you can call this function once to get the size of the file,
 * and then a second time to read its content.
 * 
 * @param filepath the path and name of the file to be read
 * @param out_file_size_bytes a pointer to a place to put the size of the file
 * @param out_file_content a pointer to a buffer which can hold the data from the file
 * @return u64 the amount of characters read from the file (this could be less then the bytes if CRLF translation happens on your platform)
 */
API u64 file_read(const char* filepath, u64* out_file_size_bytes, void* out_file_content);

/**
 * @brief reads a binary file, populates out_file_size_bytes even if out_file_content is NULL.
 * This is so you can call this function once to get the size of the file,
 * and then a second time to read its content.
 * 
 * @param filepath the path and name of the file to be read
 * @param out_file_size_bytes a pointer to a place to put the size of the file
 * @param out_file_content a pointer to a buffer which can hold the data from the file
 * @return u64 the amount of characters read from the file
 */
API u64 file_read_binary(const char* filepath, u64* out_file_size_bytes, void* out_file_content);

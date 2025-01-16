#include "utilities/file_utilities.h"
#include <stdio.h>

u64 get_file_size_bytes(const char* filepath, const char* mode);
u64 read_file_internal(const char* filepath, u64* out_size, void* out_content, const char* mode);


u64 file_read(const char* filepath, u64* out_file_size_bytes, void* out_file_content)
{
    return read_file_internal(filepath, out_file_size_bytes, out_file_content, "r");
}

u64 file_read_binary(const char* filepath, u64* out_file_size_bytes, void* out_file_content)
{
    return read_file_internal(filepath, out_file_size_bytes, out_file_content, "rb");
}


u64 get_file_size_bytes(const char* filepath, const char* mode)
{
    FILE* file;
    int error = fopen_s(&file, filepath, mode);
    if (error)
    {
        printf("file open error code: %d", error);
        return 0;
    }

    fseek(file, 0L, SEEK_END);
    u64 size = ftell(file);
    fclose(file);

    return size;
}

u64 read_file_internal(const char* filepath, u64* out_file_size_bytes, void* out_file_content, const char* mode)
{
    *out_file_size_bytes = get_file_size_bytes(filepath, mode);
    if (!out_file_content)
        return 0;

    FILE* file;
    int error = fopen_s(&file, filepath, mode);
    if (error)
    {
        printf("file open error code: %d", error);
        return 0;
    }

    u64 bytes_read = fread_s(out_file_content, *out_file_size_bytes, 1, *out_file_size_bytes, file);
    fclose(file);

    return bytes_read;
}
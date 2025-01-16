#include "lexer/token.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

// WARN!: The ordering of the keywords needs to match that of the token_type enum!
const char* keywords[] = {
    "invalid~~ignored~~", // ignored to match the indicies with that of the token_type enum
    "for",
    "while",
    "do",
    "if",
    "else",
    "null",
    "call"
};

STATIC_ASSERT((sizeof(keywords) / sizeof(keywords[0])) < 32, "We have too many keywords!");

u64 keywords_array_length()
{
    return sizeof(keywords) / sizeof(keywords[0]);
}


void token_print(token t)
{
    printf("Token{\n");
    printf("\ttype:   %d\n", t.type);
    printf("\tlength: %lld\n", t.length);
    printf("\ttext:   '%.*s'\n", (int)t.length, t.text);
    printf("\tlocation: {\n");
    printf("\t\tfilename: %s\n", t.location.filename);
    printf("\t\trow:      %lld\n", t.location.row);
    printf("\t\tcolumn:   %lld\n", t.location.column);
    printf("\t}\n");

    printf("}\n");
}

char* token_printable_text(token t, void*(*allocator)(u64 count, u64 stride))
{
    char* text_buffer = allocator(t.length + 1, sizeof(char));
    i32 error_code = strncpy_s(text_buffer, t.length + 1, t.text, t.length);
    assert(error_code == 0); // Unable to copy token text to printable buffer

    return text_buffer;
}

char* location_printable_text(location loc, void*(*allocator)(u64 count, u64 stride))
{
    u64 filename_string_size = strlen(loc.filename);
    u64 row_string_length = snprintf(NULL, 0, "%llu", loc.row);
    u64 column_string_length = snprintf(NULL, 0, "%llu", loc.column);
    u64 total_string_size = filename_string_size + row_string_length + column_string_length + 2; // NOTE: +2 for the two ':' in the final string

    char* text_buffer = allocator(total_string_size + 1, sizeof(char)); //NOTE: +1 for the null terminator
    u64 characters_written = snprintf(text_buffer, total_string_size + 1, "%s:%llu:%llu", loc.filename, loc.row, loc.column);

    assert(characters_written == total_string_size); // Failed to make the location string buffer!

    return text_buffer;
}
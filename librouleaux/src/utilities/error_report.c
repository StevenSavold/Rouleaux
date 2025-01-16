#include "utilities/error_report.h"
#include "utilities/file_utilities.h"

#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

//
// TODO(Steven): Fix the allocator situation here, the help methods 
//               to not use the supplied allocator from 
//               error_report_printable_text()
//

// Gets the text on the whole line of a given file
char* get_file_line_content(const char* filename, u64 line_number);

// Produces the '^^^^' which will go under the contextual faulted line in the error message
char* make_error_identification_line(token t);


char* format_error_message(char* message, va_list params)
{
    // Make the first pass to know how much we need to allocate
    i32 characters_written = vsnprintf(NULL, 0, message, params) + 1; // NOTE: +1 is for the null byte
    
    // Now allocate that buffer and print the formatted message into it
    char* message_buffer = calloc(characters_written, sizeof(char));
    //va_copy(params, message);
    characters_written = vsnprintf(message_buffer, characters_written, message, params);
    //va_end(params);

    return message_buffer;
}


char* error_report_printable_text(error_report report, void*(allocator)(u64 count, u64 stride))
{
    const char* report_format = 
    "Error @ [%s]: %s\n" // The line for the location in the file and the error message
    "|\n"
    "|     %s\n" // The line for the line of text in the file
    "|_    %s\n" // For the '^^^^' where the invalid token is
    ;

    char* location = location_printable_text(report.faulted_token.location, allocator);
    char* context_line = get_file_line_content(report.faulted_token.location.filename, report.faulted_token.location.row);
    char* ident_line = make_error_identification_line(report.faulted_token);

    u64 total_message_length = snprintf(NULL, 0, report_format, location, report.message, context_line, ident_line);

    char* text_buffer = allocator(total_message_length + 1, sizeof(char)); // +1 for null terminator
    snprintf(text_buffer, total_message_length, report_format, location, report.message, context_line, ident_line);

    free(context_line);
    free(ident_line);

    return text_buffer;
}



char* get_file_line_content(const char* filename, u64 line_number)
{
    u64 file_size = 0;
    u64 characters_read = file_read(filename, &file_size, NULL);
    char* file_content = malloc(file_size);
    characters_read = file_read(filename, &file_size, file_content);

    if (characters_read == 0)
    {
        free(file_content);
        // If we could not read the file, just return the error as part of the error message
        return "<Unable to read file content, to generate error message>";
    }

    u64 current_row = 1;
    char* head = file_content;
    while ((head < (file_content + characters_read)) && (current_row < line_number))
    {
        if (*head == '\n') // when we see a newline advance our row
            current_row++;

        head++; // move forward in the file content
    }

    // head should now be at the start of the line
    char* tail = head;
    while((tail < (file_content + characters_read)) && (*tail != '\n'))
    {
        tail++;
    }

    u64 line_length = tail - head;
    char* text_buffer = calloc(line_length + 1, sizeof(char));
    i32 error_code = strncpy_s(text_buffer, line_length + 1, head, line_length);
    if (error_code)
    {
        free(file_content);
        free(text_buffer);
        return "<Unable to copy file content to the output string>";
    }

    free(file_content);
    return text_buffer;
}

char* make_error_identification_line(token t)
{
    u64 num_spaces = t.location.column - 1;
    u64 total_length = num_spaces + t.length;

    char* text_buffer = calloc(total_length + 1, sizeof(char)); // +1 for the null terminator
    memset(text_buffer, ' ', total_length); // Fill the string with spaces
    char* start_bad_token_string = text_buffer + num_spaces;

    // Draw the '^~~~~' under the faulted token
    text_buffer[num_spaces] = '^';
    for (u64 i = num_spaces + 1; i < total_length; ++i)
        text_buffer[i] = '~';
    
    return text_buffer;
}
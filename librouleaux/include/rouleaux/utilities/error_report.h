#pragma once

#include "defines.h"
#include "lexer/token.h"
#include <stdarg.h>

// Forward declare
struct token;

typedef struct error_report {
    /* A heap allocated buffer containing a formatted error message */
    char* message;

    /* A pointer to the token that the parser faulted on */
    token faulted_token;
} error_report;


/**
 * @brief returns an allocated buffer of text containing the formatted message
 * 
 * @param message the message format to expand
 * @param params the format specifiers values
 * @return char* the formatted message
 */
API char* format_error_message(char* message, va_list params);

/**
 * @brief This function allocates a text buffer and fills it with a formatted message for the error
 * 
 * @param report the error_report to be converted to a string
 * @param allocator the function pointer to a allocator which will provide a zero'ed out buffer (calloc is allowed)
 * @return const char* the null terminated string
 */
API char* error_report_printable_text(error_report report, void*(allocator)(u64 count, u64 stride));

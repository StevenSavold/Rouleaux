#pragma once

#include "defines.h"
#include "parser/abstract_syntax_tree.h"
#include "lexer/token.h"
#include "utilities/error_report.h"

typedef struct parse_result {
    /* A flag indicating if the result of the parse was a success or failure */
    b8 success;

    /* The pointer to the root of the tree that is the result of the parse */
    ast_node* resulting_tree;

    /* The error information, this is only populated if success is false */
    error_report error;
} parse_result;


/**
 * @brief produces a parse result symbolizing a successful parse
 * 
 * @param resulting_tree the tree which represents the successfully parsed tokens
 * @return parse_result 
 */
API parse_result parse_result_success(ast_node* resulting_tree);

/**
 * @brief produces a parse result symbolizing an error, with a given message
 * 
 * @note message can be a format string, and will be expanded accordingly
 * @note this function will copy the message to a allocated buffer, it needs to be free'd by the user
 * 
 * @param t the token that caused the error
 * @param message the message of the error
 * @param ... any format specifier values found in the message
 * @return parse_result 
 */
API parse_result parse_result_error(token t, char* message, ...);

/**
 * @brief releases the memory held by the parse_result
 * 
 * @param result the parse_result to destroy
 */
API void parse_result_destroy(parse_result* result);


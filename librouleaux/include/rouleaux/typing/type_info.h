#pragma once

#include "defines.h"
#include "lexer/token.h"
#include "utilities/error_report.h"

// Forward declare
struct symbol_table;

typedef enum type_info {
    TYPE_INFO_UNKNOWN = 0,

    TYPE_INFO_INTEGER,
    TYPE_INFO_FLOAT,
    TYPE_INFO_STRING,

    TYPE_INFO_FUNCTION,

    MAX_TYPE_INFOS
} type_info;

typedef struct typing_result {
    /* A flag to denote if the typing was successful */
    b8 success;

    /* The error_report if the typing failed */
    error_report error;

    /* The type_info of the node that was successfully typed */
    type_info type;
} typing_result;


// Forward declare
struct ast_node;

/**
 * @brief recursively descends a given AST and sets the token typing_information of each node, or returns an error message if it fails
 * 
 * @param ast the node of an abstract syntax tree to recursively perform typing on
 * @param sym_table a pointer to a symbol table which the function can add to and lookup existing variables types
 * @return typing_result the result of typing on the ast_node
 */
API typing_result resolve_types(struct ast_node* ast, struct symbol_table* sym_table);

/**
 * @brief returns a successful typing_result with the given type_info
 * 
 * @param tinfo the type info to add to the result
 * @return typing_result a successful typing_result
 */
API typing_result typing_result_success(type_info tinfo);

/**
 * @brief 
 * 
 * @param t the token that caused the error
 * @param message the message of the error
 * @param ... any format specifier values found in the message
 * @return typing_result a failed typing_result with an error_report containing the given message
 */
API typing_result typing_result_error(token t, char* message, ...);

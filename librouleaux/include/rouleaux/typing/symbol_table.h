#pragma once

#include "defines.h"
#include "lexer/token.h"
#include "typing/type_info.h"

// Forward declare
struct ast_node;

/**
 * @brief A struct of non-owning pointers to
 * 
 */
typedef struct symbol {
    /* The token of symbol (used for getting the name) */
    token t;

    /* The type of the symbol */
    type_info type;

    /* True if the symbol was declared as a constant, false otherwise */
    b8 is_constant;

    /* If the type of this symbol is a function. This pointer will be set to the function declaration node that the declaration of this symbol had */
    struct ast_node* function_decl_node;
} symbol;


//
// TODO(Steven): This should be a hash table... but for simplicity we made it a list that is iterated over
//
/**
 * @brief a dynamic array of symbols to be searched through
 * 
 */
typedef struct symbol_table {
    symbol* buffer;
    u64 size;
    u64 capacity;
} symbol_table;

/**
 * @brief creates a symbol_table
 * 
 * @return symbol_table the created table
 */
API symbol_table symbol_table_create();

/**
 * @brief destroys a symbol_table
 * 
 * @param table the table to destroy
 */
API void symbol_table_destroy(symbol_table* table);

/**
 * @brief Adds a symbol to the table with the given token and type_info
 * 
 * @param table the table to add to
 * @param t the token to set the new symbol with
 * @param type the type info to set the new symbol with
 * @param is_constant the flag which denotes if the symbol is constant
 * @return b8 true of the symbol was added, false otherwise
 */
API b8 symbol_table_add(symbol_table* table, token t, type_info type, b8 is_constant);

/**
 * @brief finds a symbol in the table
 * 
 * @param table the table to search in
 * @param t the token to search for
 * @return symbol* the non_owning pointer to the symbol in the table, NULL if it could not be found
 */
API symbol* symbol_table_find(symbol_table* table, token t);

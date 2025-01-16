#include "typing/symbol_table.h"

#include <malloc.h>
#include <string.h>

#define DEFAULT_SYMBOL_TABLE_CAPACITY 1
#define DEFAULT_SYMBOL_TABLE_RESIZE_FACTOR 2

static b8 reallocate_buffer(symbol_table* table, u64 resize_factor);
static void populate_builtin_types(symbol_table* table);
static token create_base_type_token(const char* text, token_type ttype, type_info tinfo);

symbol_table symbol_table_create()
{
    symbol_table table = {};
    table.buffer = malloc(DEFAULT_SYMBOL_TABLE_CAPACITY * sizeof(symbol));
    table.capacity = DEFAULT_SYMBOL_TABLE_CAPACITY;

    populate_builtin_types(&table);
    
    return table;
}

void symbol_table_destroy(symbol_table* table)
{
    free(table->buffer);
    table->capacity = 0;
    table->size = 0;
}

b8 symbol_table_add(symbol_table* table, token t, type_info type, b8 is_constant)
{
    if (table->size + 1 >= table->capacity)
    {
        if (!reallocate_buffer(table, DEFAULT_SYMBOL_TABLE_RESIZE_FACTOR))
            return false; // Failed to allocate table
    }

    // If the symbol is already in the table, we should not add it
    if (symbol_table_find(table, t) != NULL)
        return false;

    // Set the element
    table->buffer[table->size].t = t;
    table->buffer[table->size].type = type;
    table->buffer[table->size].is_constant = is_constant;
    table->size++;

    return true;
}

symbol* symbol_table_find(symbol_table* table, token t)
{
    for (u64 i = 0; i < table->size; ++i)
    {
        if (table->buffer[i].t.length != t.length)
            continue; // If the lengths are not he same, they are clearly not equal

        if (strncmp(table->buffer[i].t.text, t.text, t.length) == 0)
            return &(table->buffer[i]);
    }

    return NULL;
}


static b8 reallocate_buffer(symbol_table* table, u64 resize_factor)
{
    u64 new_capacity = table->capacity * resize_factor;
    symbol* new_buffer = malloc(new_capacity * sizeof(symbol));

    int error_code = memcpy_s(new_buffer, new_capacity * sizeof(symbol), table->buffer, table->capacity * sizeof(symbol));
    if (error_code)
        return false; // we failed to copy the memory

    free(table->buffer);
    table->buffer = new_buffer;
    table->capacity = new_capacity;

    return true; // We successfully reallocated the list buffer
}

static void populate_builtin_types(symbol_table* table)
{
    // TODO(Steven): This is all wrong! update me!
    token float_type = create_base_type_token("float", TOKEN_FLOAT_LITERAL, TYPE_INFO_FLOAT);
    symbol_table_add(table, float_type, TYPE_INFO_FLOAT, true);

    token int_type = create_base_type_token("int", TOKEN_INTEGER_LITERAL, TYPE_INFO_INTEGER);
    symbol_table_add(table, int_type, TYPE_INFO_INTEGER, true);
}

static token create_base_type_token(const char* text, token_type ttype, type_info tinfo)
{
    token type_token = {};
    type_token.text = text;
    type_token.length = strlen(text);
    type_token.type = ttype;
    type_token.typing_information = tinfo;
    
    return type_token;
}
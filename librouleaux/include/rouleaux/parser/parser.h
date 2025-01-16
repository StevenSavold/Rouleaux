#pragma once

#include "defines.h"
#include "lexer/lexer.h"
#include "parser/abstract_syntax_tree.h"
#include "parser/parse_result.h"

//
// TODO(Steven): Since ast_nodes should be all the same size, this feels like a great opportunity for a arena allocator
//


typedef void* (*node_allocator_fptr)   (u64 size_in_bytes);
typedef void  (*node_deallocator_fptr) (void* block, u64 size_in_bytes);


/**
 * @brief The parser for the rouleaux language
 * 
 */
typedef struct rouleaux_parser {
    /* A lexer for the file we are parsing */
    rouleaux_lexer lexer;

    /* A heap allocated ast_node which points to other heap allocated ast_nodes */
    ast_node* ast_head;

    /* If the parser encounters an error it will set this flag */
    b8 has_error;

    /* True when the parse is done parsing the file */
    b8 done;

    /* A function pointer to a function which will allocate memory for the ast_nodes */
    node_allocator_fptr node_allocator;
    /* A function pointer to a function which will deallocate memory that was created by the allocator function */
    node_deallocator_fptr node_deallocator;
} rouleaux_parser;


/**
 * @brief Creates a rouleaux_parser for the given file
 * 
 * @note the parser's has_error feild will be set to true if creation fails
 * 
 * @param filename the name of the file to parse
 * @param allocator a pointer to a function which will allocate memory (this is used for allocating the ast_nodes)
 * @param deallocator a pointer to a function which will deallocate memory created by the allocator function (this is used for deallocating the ast_nodes)
 * @return rouleaux_parser the parser for the given file
 */
API rouleaux_parser parser_create(const char* filename, node_allocator_fptr allocator, node_deallocator_fptr deallocator);

/**
 * @brief recursively deallocates the memory held by the parser
 * 
 * @param parser the parser to release the resources of
 */
API void parser_destroy(rouleaux_parser* parser);

/**
 * @brief parses the whole file and gives the result
 * 
 * @param parser the parse to operate on
 * @return parse_result the result of the parse
 */
API parse_result parser_parse_file(rouleaux_parser* parser);

/**
 * @brief parses the next few tokens as it the next token will start a statement
 * 
 * @param parser the parser to operate on
 * @return parse_result the result of the parse
 */
API parse_result parser_parse_statement(rouleaux_parser* parser);

/**
 * @brief parses the next few tokens as if they represent a variable (or constant) declaration, or assignment
 * 
 * @param parser the parser to operate on
 * @return parse_result the result of the parse
 */
API parse_result parser_parse_declaration_or_assignment(rouleaux_parser* parser);

/**
 * @brief parses the next few tokens as if they represent a function declaration or expression
 * 
 * @param parser the parser to operate on
 * @return parse_result the result of the parse
 */
API parse_result parser_parse_function_or_expression(rouleaux_parser* parser);

/**
 * @brief parses the next few tokens as if they represent a function declaration
 * 
 * @param parser the parser to operate on
 * @return parse_result the result of the parse
 */
API parse_result parser_parse_function_declaration(rouleaux_parser* parser);

/**
 * @brief parses the next few tokens as if they represent a function declaration parameter list
 * 
 * @param parser the parser to operate on
 * @return parse_result the result of the parse
 */
API parse_result parser_parse_parameter_list(rouleaux_parser* parser);

/**
 * @brief parses the next few tokens as if they represent a function call parameter list
 * 
 * @param parser the parser to operate on
 * @return parse_result the result of the parse
 */
API parse_result parser_parse_function_call_list(rouleaux_parser* parser);

/**
 * @brief parses the next few tokens as if they represent a function declaration parameter
 * 
 * @param parser the parser to operate on
 * @return parse_result the result of the parse
 */
API parse_result parser_parse_function_declaration_parameter(rouleaux_parser* parser);

/**
 * @brief parses the next few tokens as if they represent a function return type
 * 
 * @param parser the parser to operate on
 * @return parse_result the result of the parse
 */
API parse_result parser_parse_return_type(rouleaux_parser* parser);

/**
 * @brief parses the next token as if it was an if keyword
 * 
 * @param parser the parser to operate on
 * @return parse_result the result of the parse
 */
API parse_result parser_parse_keyword_if(rouleaux_parser* parser);

/**
 * @brief parses the next token as if it was a while keyword
 * 
 * @param parser the parser to operate on
 * @return parse_result the result of the parse
 */
API parse_result parser_parse_keyword_while(rouleaux_parser* parser);

/**
 * @brief parses the next few tokens as if the next token will start an expression
 * 
 * @param parser the parser to operate on
 * @return parse_result the resulting state of the parse
 */
API parse_result parser_parse_expression_beginning(rouleaux_parser* parser);

/**
 * @brief parses the next few tokens as if the next token will be in the middle of an expression
 * @note parser_parse_expression_beginning is the function to use to parse a whole expression, this function only returns the right hand side
 * 
 * @param parser the parser to operate on
 * @return parse_result the resulting state of the parse
 */
API parse_result parser_parse_expression(rouleaux_parser* parser);

/**
 * @brief parses the next token as if it was a identifier
 * 
 * @param parser the parser to operate on
 * @return parse_result the result state of the parse
 */
API parse_result parser_parse_identifier(rouleaux_parser* parser);

/**
 * @brief parses the next token as if it was a integer literal
 * 
 * @param parser the parser to operate on
 * @return parse_result the resulting state of the parse
 */
API parse_result parser_parse_integer_literal(rouleaux_parser* parser);

/**
 * @brief parses the next token as if it was a float literal
 * 
 * @param parser the parser to operate on
 * @return parse_result the resulting state of the parse
 */
API parse_result parser_parse_float_literal(rouleaux_parser* parser);

/**
 * @brief parses the next token as if it was a string literal
 * 
 * @param parser the parser to operate on
 * @return parse_result the resulting state of the parse
 */
API parse_result parser_parse_string_literal(rouleaux_parser* parser);

/**
 * @brief parses the next token as if it was a line comment or a block comment
 * 
 * @param parser the parser to operate on
 * @return parse_result the resulting state of the parse
 */
API parse_result parser_parse_comment(rouleaux_parser* parser);

/**
 * @brief parses the next token as if it were a value assignment operator
 * 
 * @param parser the parser to operate on
 * @return parse_result the resulting state of the parse
 */
API parse_result parser_parse_value_assignment_operator(rouleaux_parser* parser);

/**
 * @brief parses the next token as if it were a value assignment operator
 * 
 * @param parser the parser to operate on
 * @return parse_result the resulting state of the parse
 */
API parse_result parser_parse_constant_assignment_operator(rouleaux_parser* parser);

/**
 * @brief parses the next token as if it was a type assignment operator
 * 
 * @param parser the parser to operate on
 * @return parse_result the resulting state of the parse
 */
API parse_result parser_parse_type_assignment_operator(rouleaux_parser* parser);

/**
 * @brief parses the next token as if it was a statement end operator
 * 
 * @param parser the parser to operate on
 * @return parse_result the resulting state of the parse
 */
API parse_result parser_parse_statement_end_operator(rouleaux_parser* parser);

/**
 * @brief a helper function which allocates using the parser's node_allocator and populates the type of a ast_node then returns it
 *
 * @note This does not set any children of the node
 * 
 * @param parser the parser, who is responsible for allocating the node
 * @param type the type of the node to create
 * @return ast_node* a pointer to the allocated ast_node
 */
API ast_node* parser_create_ast_node(rouleaux_parser* parser, ast_node_type type);

/**
 * @brief a helper function which deallocates using the parser's node_deallocator
 * 
 * @param parser the parser, who created the node
 * @param node the node to be deallocated
 */
API void parser_destroy_ast_node(rouleaux_parser* parser, ast_node* node);

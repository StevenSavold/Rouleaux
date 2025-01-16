#pragma once

#include "defines.h"
#include "lexer/token.h"
#include "parser/node_list.h"


typedef enum ast_node_type {
    AST_INVALID = 0,

    AST_IDENTIFIER,

    AST_BINARY_OPERATOR_PLUS,
    AST_BINARY_OPERATOR_MINUS,
    AST_BINARY_OPERATOR_MULTIPLY,
    AST_BINARY_OPERATOR_DIVIDE,
    AST_BINARY_OPERATOR_MODULUS,
    AST_BINARY_OPERATOR_GREATER_THAN,
    AST_BINARY_OPERATOR_LESS_THAN,

    AST_VALUE_ASSIGNMENT,
    AST_CONST_ASSIGNMENT,
    AST_TYPE_ASSIGNMENT,

    AST_INTEGER_LITERAL,
    AST_FLOAT_LITERAL,
    AST_STRING_LITERAL,

    AST_FUNCTION_DECLARATION,
    AST_FUNCTION_CALL,
    AST_PARAMETER_LIST,
    AST_CALL_OPERATOR,

    AST_IF_STATEMENT,
    AST_WHILE_STATEMENT,

    AST_STATEMENT_END,

    AST_COMMENT,

    AST_SCOPE,

    AST_EOF,
    AST_MAX_TYPES
} ast_node_type;

typedef enum ast_node_child_strategy {
    CHILD_STRATEGY_NONE = 0,    // A node with no children
    CHILD_STRATEGY_UNARY,       // A node with a single child
    CHILD_STRATEGY_BINARY,      // A node with 2 children
    CHILD_STRATEGY_TERNARY,     // A node with 3 children

    CHILD_STRATEGY_MANY,        // A node with potentially limitless children
} ast_node_child_strategy;


struct ast_node; // forward declare

typedef struct ast_leaf_node {
    token t;
} ast_leaf_node;

typedef struct ast_unary_node {
    token t;
    struct ast_node* child;
} ast_unary_node;

typedef struct ast_binary_node {
    token t;
    struct ast_node* left_child;
    struct ast_node* right_child;
} ast_binary_node;

typedef struct ast_ternary_node {
    token t;
    struct ast_node* left_child;
    struct ast_node* center_child;
    struct ast_node* right_child;
} ast_ternary_node;

typedef struct ast_many_node {
    token t;
    node_list children;
} ast_many_node;

typedef union ast_generic_node {
    ast_leaf_node leaf;
    ast_unary_node unary;
    ast_binary_node binary;
    ast_ternary_node ternary;
    ast_many_node many;
} ast_generic_node;

/**
 * @brief A node in the Rouleaux abstract syntax tree (ast)
 */
typedef struct ast_node {
    /* The type of node this represents */
    ast_node_type type;

    /* The node data */
    ast_generic_node node;

    /* A flag indicating if this node represents the top most node in a subtree that was enclosed in parenthesis */
    b8 enclosed_in_parens;
} ast_node;


/**
 * @brief Construct a new ast node of the given type
 * 
 * @param type the type of node you wish to create
 */
API ast_node ast_node_create(ast_node_type type);

/**
 * @brief recursively release the allocated resources of the ast_node and its children
 * 
 * @param node the node to be deallocated
 * @param deallocator the function that should be used to deallocate the children of the node
 */
API void ast_node_destroy(ast_node* node, void(*deallocator)(void* block, u64 size));

/**
 * @brief Given a ast_node_type, this function returns the child_strategy that is associated with that type
 * @example a AST_BINARY_OPERATOR_PLUS is a CHILD_STRATEGY_BINARY because the '+' operator has a left child and a right child
 * 
 * @param type the node type
 * @return ast_node_child_strategy the child_strategy enum value
 */
API ast_node_child_strategy ast_node_child_strategy_from_node_type(ast_node_type type);

/**
 * @brief returns the precedence value of the given node_type
 * 
 * @param node_type the node_type to get the precedence of
 * @return i32 returns the precedence value of the given node_type, -1 if no value is defined
 */
API i32 precedence_from_node_type(ast_node_type node_type);

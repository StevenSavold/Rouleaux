#include "parser/abstract_syntax_tree.h"
#include <assert.h>

ast_node ast_node_create(ast_node_type type)
{
    ast_node node = {};
    node.type = type;

    return node;
}

void ast_node_destroy(ast_node* node, void(*deallocator)(void* block, u64 size))
{
    if (!node)
    {
        // If the node is null, there is nothing to do
        return;
    }

    ast_node_child_strategy number_of_children = ast_node_child_strategy_from_node_type(node->type);
    switch(number_of_children)
    {
        case CHILD_STRATEGY_NONE:
        {
            // There is no children to deallocate
            break;
        }
        case CHILD_STRATEGY_UNARY:
        {
            // Deallocate the only child
            ast_node_destroy(node->node.unary.child, deallocator);
            node->node.unary.child = NULL;

            break;
        }
        case CHILD_STRATEGY_BINARY:
        {
            // Deallocate the left and right children
            ast_node_destroy(node->node.binary.left_child, deallocator);
            ast_node_destroy(node->node.binary.right_child, deallocator);

            node->node.binary.left_child = NULL;
            node->node.binary.right_child = NULL;

            break;
        }
        case CHILD_STRATEGY_TERNARY:
        {
            // Deallocate the left, center, and right children
            ast_node_destroy(node->node.ternary.left_child, deallocator);
            ast_node_destroy(node->node.ternary.center_child, deallocator);
            ast_node_destroy(node->node.ternary.right_child, deallocator);

            node->node.ternary.left_child = NULL;
            node->node.ternary.center_child = NULL;
            node->node.ternary.right_child = NULL;

            break;
        }
        case CHILD_STRATEGY_MANY:
        {
            // Loop over the list of children and deallocate them
            node_list_destroy(&(node->node.many.children), deallocator);

            break;
        }
    };
}

ast_node_child_strategy ast_node_child_strategy_from_node_type(ast_node_type type)
{
    switch(type)
    {
        case AST_COMMENT:
        case AST_IDENTIFIER:
        case AST_INTEGER_LITERAL:
        case AST_FLOAT_LITERAL:
        case AST_STRING_LITERAL:
        case AST_STATEMENT_END:
        case AST_EOF:
        case AST_INVALID:
        case AST_MAX_TYPES:
        {
            return CHILD_STRATEGY_NONE;
        }
        case AST_CALL_OPERATOR:
        {
            return CHILD_STRATEGY_UNARY;
        }
        case AST_BINARY_OPERATOR_PLUS:
        case AST_BINARY_OPERATOR_MINUS:
        case AST_BINARY_OPERATOR_MULTIPLY:
        case AST_BINARY_OPERATOR_DIVIDE:
        case AST_BINARY_OPERATOR_MODULUS:
        case AST_BINARY_OPERATOR_GREATER_THAN:
        case AST_BINARY_OPERATOR_LESS_THAN:
        case AST_VALUE_ASSIGNMENT:
        case AST_TYPE_ASSIGNMENT:
        case AST_CONST_ASSIGNMENT:
        case AST_WHILE_STATEMENT:
        case AST_FUNCTION_CALL:
        {
            return CHILD_STRATEGY_BINARY;
        }
        case AST_IF_STATEMENT:
        case AST_FUNCTION_DECLARATION:
        {
            return CHILD_STRATEGY_TERNARY;
        }
        case AST_SCOPE:
        case AST_PARAMETER_LIST:
        {
            return CHILD_STRATEGY_MANY;
        }
    };
}

i32 precedence_from_node_type(ast_node_type node_type)
{
    // NOTE(Steven): Higher precedence number means that operation will happen first (lower in the tree)
    switch (node_type)
    {
        case AST_BINARY_OPERATOR_GREATER_THAN:
        case AST_BINARY_OPERATOR_LESS_THAN:
        {
            return 1;
        }
        case AST_BINARY_OPERATOR_PLUS:
        case AST_BINARY_OPERATOR_MINUS:
        {
            return 2;
        }
        case AST_BINARY_OPERATOR_MULTIPLY:
        case AST_BINARY_OPERATOR_DIVIDE:
        case AST_BINARY_OPERATOR_MODULUS:
        {
            return 3;
        }
        default:
        {
            return -1;
        }
    }
}
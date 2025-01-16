#include "typing/type_info.h"

#include "lexer/token.h"
#include "parser/abstract_syntax_tree.h"
#include "typing/symbol_table.h"
#include "utilities/error_report.h"

#include <stdarg.h>
#include <malloc.h>


typing_result resolve_types(ast_node* ast, symbol_table* sym_table)
{
    switch(ast->type)
    {
        case AST_INTEGER_LITERAL:
        {
            ast->node.leaf.t.typing_information = TYPE_INFO_INTEGER;
            return typing_result_success(ast->node.leaf.t.typing_information);
        }
        case AST_FLOAT_LITERAL:
        {
            ast->node.leaf.t.typing_information = TYPE_INFO_FLOAT;
            return typing_result_success(ast->node.leaf.t.typing_information);
        }
        case AST_STRING_LITERAL:
        {
            ast->node.leaf.t.typing_information = TYPE_INFO_STRING;
            return typing_result_success(ast->node.leaf.t.typing_information);
        }
        case AST_BINARY_OPERATOR_PLUS:
        case AST_BINARY_OPERATOR_MINUS:
        case AST_BINARY_OPERATOR_MULTIPLY:
        case AST_BINARY_OPERATOR_DIVIDE:
        case AST_BINARY_OPERATOR_MODULUS:
        case AST_BINARY_OPERATOR_GREATER_THAN:
        case AST_BINARY_OPERATOR_LESS_THAN:
        {
            typing_result left_result = resolve_types(ast->node.binary.left_child, sym_table);
            if (!left_result.success) // If we failed to type the left node, bubble up the error
                return left_result;

            typing_result right_result = resolve_types(ast->node.binary.right_child, sym_table);
            if (!right_result.success) // If we failed to type the right node, bubble up the error
                return right_result;

            if (left_result.type == right_result.type) // If the types match, this operator is fine to perform its action
                return typing_result_success(left_result.type);

            // TODO(Steven): Handle mismatching types, we want to auto cast (or similar) for some types
            return typing_result_error(ast->node.binary.t, "Left and right operand types do not match!");
        }
        case AST_TYPE_ASSIGNMENT:
        {
            // type assignment operator could have a null right child (in which case typing needs to be automatically assigned)
            if (ast->node.binary.right_child == NULL)
                return typing_result_success(TYPE_INFO_UNKNOWN); // Return unknown and let it be handled higher in the tree

            // If there is a right child, get its type from the symbol table and assign the left child to it
            symbol* sym = symbol_table_find(sym_table, ast->node.binary.right_child->node.leaf.t);
            if (sym == NULL)
            {
                token* t = &(ast->node.binary.right_child->node.leaf.t);
                return typing_result_error(*t, "Unknown type '%.*s' being used in variable declaration", t->length, t->text);
            }

            // We need to check if the variable being assigned this type already exists!
            token* identifier_token = &(ast->node.binary.left_child->node.leaf.t);
            symbol* identifier_symbol = symbol_table_find(sym_table, *identifier_token);
            if (identifier_symbol != NULL)
            {
                // We are re-declaring this variable!
                // @BumpAllocator
                char* original_declaration_location_text = location_printable_text(identifier_symbol->t.location, calloc);
                typing_result result = typing_result_error(*identifier_token, "A variable with the name '%.*s' already exists! It was declared here [%s]", identifier_token->length, identifier_token->text, original_declaration_location_text);
                free(original_declaration_location_text);

                return result;
            }

            // We could not find a symbol with that name, so we need to add it
            // NOTE(Steven): This will add the variable to the symbol table when 
            //               the symbol is being declared with a type specified 
            //               (i.e. my_int: int = 0) but NOT when its auto deduced
            // TODO(Steven): @CompilerBug what is the parent of this node? We cant say for sure if this is not a constant assignment operation!!
            symbol_table_add(sym_table, *identifier_token, sym->type, false);
            
            ast->node.binary.left_child->node.leaf.t.typing_information = sym->type;
            ast->node.binary.t.typing_information = sym->type;
            return typing_result_success(ast->node.binary.left_child->node.leaf.t.typing_information);
        }
        case AST_VALUE_ASSIGNMENT:
        {
            typing_result right_result = resolve_types(ast->node.binary.right_child, sym_table);
            if (!right_result.success)
                return right_result;

            typing_result left_result = resolve_types(ast->node.binary.left_child, sym_table);
            if (!left_result.success)
                return left_result;


            // If we are assigning to an already declared variable
            if (ast->node.binary.left_child->type == AST_IDENTIFIER)
            {
                // If our left child is an identifier, this is a previously declared variable
                // and we need to look at the symbol table to resolve its type...
                symbol* sym = symbol_table_find(sym_table, ast->node.binary.left_child->node.leaf.t);
                // If the symbol could not be found, the variable has not been declared yet
                if (sym == NULL)
                {
                    token* t = &(ast->node.binary.left_child->node.leaf.t);
                    return typing_result_error(*t, "Undeclared variable '%.*s'", t->length, t->text);
                }

                if (sym->is_constant)
                {
                    // @BumpAllocator
                    token* t = &(ast->node.binary.left_child->node.leaf.t);
                    char* orig_location = location_printable_text(sym->t.location, calloc);
                    typing_result result = typing_result_error(*t, "Cannot assign to variable '%.*s' because it was defined as a constant. Original declaration was made here [%s]", t->length, t->text, orig_location);
                    free(orig_location);

                    return result;
                }

                // If the types dont match!
                if (sym->type != right_result.type)
                {
                    // Grab a reference to the actual token that caused the error
                    token* t = &(ast->node.binary.left_child->node.leaf.t);
                    return typing_result_error(*t, "Type mismatch: the type of '%.*s' does not match that of the assigned expression.", t->length, t->text);
                }

                // The types matched! everything checks out, we can move back up the tree
                ast->node.binary.t.typing_information = sym->type;
                return typing_result_success(sym->type);
            }

            // If we are creating a new variable
            if (ast->node.binary.left_child->type == AST_TYPE_ASSIGNMENT)
            {
                // If the type info could not tell us the type, it means we need to automatically deduce the type for the variable via the right side
                if (left_result.type == TYPE_INFO_UNKNOWN)
                {
                    token* identifier_token = &(ast->node.binary.left_child->node.binary.left_child->node.leaf.t);
                    symbol* sym = symbol_table_find(sym_table, *identifier_token);
                    if (sym != NULL)
                    {
                        // The variable already exists!
                        // @BumpAllocator
                        char* original_symbol_location_text = location_printable_text(sym->t.location, calloc);
                        typing_result result = typing_result_error(*identifier_token, "A variable named '%.*s' already exists! The original was declared here [%s]", identifier_token->length, identifier_token->text, original_symbol_location_text);
                        free(original_symbol_location_text);

                        return result;
                    }
                    // Get a reference to the identifiers token
                    // Set both the type assignment operator and that identifier to the rvalue's type
                    ast->node.binary.left_child->node.binary.t.typing_information = right_result.type;
                    identifier_token->typing_information = right_result.type;

                    // Add this variable to the symbol_table
                    // NOTE(Steven): This will add the symbol to the table when it's type was auto
                    //               deduced. The symbol would have been added already if it had
                    //               its type manually specified.
                    if (!symbol_table_add(sym_table, *identifier_token, right_result.type, false))
                    {
                        // If we failed to add to the symbol table, we must have failed an allocation?
                        return typing_result_error(*identifier_token, "Unable to allocate memory for the symbol table! *This is a compiler bug*");
                    }

                    // If that added symbol was a function declaration, we need to add the parameter list to the table
                    if (identifier_token->typing_information == TYPE_INFO_FUNCTION)
                    {
                        // @Performance: We just set this, and now we are linearly searching for it?
                        symbol* added_symbol = symbol_table_find(sym_table, *identifier_token);
                        if (added_symbol == NULL)
                            return typing_result_error(*identifier_token, "Unable to find added token in symbol table! *Compiler Bug*");
                    
                        // Set the function declaration node
                        added_symbol->function_decl_node = ast->node.binary.right_child;
                    }

                    // The variable has been added to the symbol table and been given a type. We are all set
                    ast->node.binary.t.typing_information = right_result.type;
                    return typing_result_success(right_result.type);
                }

                if (left_result.type == right_result.type)
                    return typing_result_success(right_result.type);
                
                // TODO(Steven): Handle mismatching types, we want to auto cast (or similar) for some types
                token* t = &(ast->node.binary.left_child->node.binary.left_child->node.leaf.t);
                return typing_result_error(ast->node.binary.t, "Attempting to assign incorrect type to variable '%.*s'", t->length, t->text);
            }

            return typing_result_error(ast->node.binary.t, "Unimplemented typing event for assignment operator! *aka. Compiler Bug*");
        }
        case AST_CONST_ASSIGNMENT:
        {
            typing_result right_result = resolve_types(ast->node.binary.right_child, sym_table);
            if (!right_result.success)
                return right_result;

            typing_result left_result = resolve_types(ast->node.binary.left_child, sym_table);
            if (!left_result.success)
                return left_result;

            // A type assignment node should be the only possible thing here
            if (ast->node.binary.left_child->type != AST_TYPE_ASSIGNMENT)
            {
                return typing_result_error(ast->node.binary.left_child->node.leaf.t, "Unexpected token to the left of const-assignment operator!");
            }

            // If the type assignment node does not know the type, we must automatically deduce its type based on the right hand expression
            if (left_result.type == TYPE_INFO_UNKNOWN)
            {
                // Make sure the symbol is not being re-defined
                token* identifier_token = &(ast->node.binary.left_child->node.binary.left_child->node.leaf.t);
                symbol* sym = symbol_table_find(sym_table, *identifier_token);
                if (sym != NULL)
                {
                    // The variable already exists!
                    // @BumpAllocator
                    char* original_symbol_location_text = location_printable_text(sym->t.location, calloc);
                    typing_result result = typing_result_error(*identifier_token, "A variable named '%.*s' already exists! The original was declared here [%s]", identifier_token->length, identifier_token->text, original_symbol_location_text);
                    free(original_symbol_location_text);

                    return result;
                }

                // Set both the const assignment operator and that identifier to the rvalue's type
                ast->node.binary.left_child->node.binary.t.typing_information = right_result.type;
                identifier_token->typing_information = right_result.type;

                // Add this constant to the symbol_table
                if (!symbol_table_add(sym_table, *identifier_token, right_result.type, true))
                {
                    // If we failed to add to the symbol table, we must have failed an allocation?
                    return typing_result_error(*identifier_token, "Unable to allocate memory for the symbol table! *This is a compiler bug*");
                }

                // If that added symbol was a function declaration, we need to add the parameter list to the table
                if (identifier_token->typing_information == TYPE_INFO_FUNCTION)
                {
                    // @Performance: We just set this, and now we are linearly searching for it?
                    symbol* added_symbol = symbol_table_find(sym_table, *identifier_token);
                    if (added_symbol == NULL)
                        return typing_result_error(*identifier_token, "Unable to find added token in symbol table! *Compiler Bug*");
                
                    // Set the declaration parameter list
                    added_symbol->function_decl_node = ast->node.binary.right_child;
                }

                // The variable has been added to the symbol table and been given a type. We are all set
                ast->node.binary.t.typing_information = right_result.type;
                return typing_result_success(right_result.type);
            }

        }
        case AST_IDENTIFIER:
        {
            symbol* sym = symbol_table_find(sym_table, ast->node.leaf.t);
            // If we could not find the symbol throw an error
            if (sym == NULL)
                return typing_result_error(ast->node.leaf.t, "Undeclared symbol '%.*s'", ast->node.leaf.t.length, ast->node.leaf.t.text);

            ast->node.leaf.t.typing_information = sym->type;
            return typing_result_success(sym->type);
        }
        case AST_FUNCTION_DECLARATION:
        {
            typing_result params_result = resolve_types(ast->node.ternary.left_child, sym_table);
            if (!params_result.success)
            {
                return params_result;
            }

            typing_result return_type_result = resolve_types(ast->node.ternary.center_child, sym_table);
            if (!return_type_result.success)
            {
                return params_result;
            }

            // Do block typing after parameter typing to make sure symbols are defined
            typing_result block_result = resolve_types(ast->node.ternary.right_child, sym_table);
            if (!block_result.success)
            {
                return block_result;
            }

            ast->node.ternary.t.typing_information = TYPE_INFO_FUNCTION;
            return typing_result_success(TYPE_INFO_FUNCTION);
        }
        case AST_FUNCTION_CALL:
        {
            // This will check if the function name already exists
            typing_result function_name_result = resolve_types(ast->node.binary.left_child, sym_table);
            if (!function_name_result.success)
                return function_name_result;

            // Now we need to iterate over both this list (the right child) and the function declarations param list to match the types
            // Go find the function symbol to get the function declarations
            symbol* function_symbol = symbol_table_find(sym_table, ast->node.binary.left_child->node.leaf.t);
            // No need to check if this exists, we know it does or the function_name_result would have failed!
            if (function_symbol->type != TYPE_INFO_FUNCTION)
                return typing_result_error(ast->node.binary.left_child->node.leaf.t, "Cannot call something that is not a function");

            if (function_symbol->function_decl_node == NULL)
                return typing_result_error(ast->node.binary.left_child->node.leaf.t, "No parameter list found for this symbol! *Compiler Bug*");
        
            i32 param_length_difference = function_symbol->function_decl_node->node.ternary.left_child->node.many.children.number_of_nodes - ast->node.binary.right_child->node.many.children.number_of_nodes;
            if (param_length_difference != 0)
            {
                const char* param_diff_text = (param_length_difference > 0) ? "Too few" : "Too many";
                return typing_result_error(ast->node.binary.left_child->node.leaf.t, "%s parameters for function call, got %llu, but expected %llu", param_diff_text, ast->node.binary.right_child->node.many.children.number_of_nodes, function_symbol->function_decl_node->node.ternary.left_child->node.many.children.number_of_nodes);
            }

            // We know the function call has the same amount as the function declaration
            for (u64 i = 0; i < function_symbol->function_decl_node->node.ternary.left_child->node.many.children.number_of_nodes; ++i)
            {
                ast_node* function_decl_param = function_symbol->function_decl_node->node.ternary.left_child->node.many.children.nodes[i];
                ast_node* function_call_param = ast->node.binary.right_child->node.many.children.nodes[i];
                
                typing_result function_call_param_result = resolve_types(function_call_param, sym_table);
                if (!function_call_param_result.success)
                {
                    return function_call_param_result;
                }

                if (function_decl_param->node.binary.t.typing_information != function_call_param->node.leaf.t.typing_information)
                {
                    return typing_result_error(function_call_param->node.leaf.t, "Parameter's type does not match that of function declaration");
                }
            }

            // Return with the function return type
            return typing_result_success(function_symbol->function_decl_node->node.ternary.center_child->node.leaf.t.typing_information);
        }
        case AST_PARAMETER_LIST:
        {
            for (u64 i = 0; i < ast->node.many.children.number_of_nodes; ++i)
            {
                typing_result param_result = resolve_types(ast->node.many.children.nodes[i], sym_table);
                if (!param_result.success)
                {
                    return param_result;
                }
            }

            return typing_result_success(TYPE_INFO_UNKNOWN);
        }
        case AST_CALL_OPERATOR:
        {
            typing_result function_call_result = resolve_types(ast->node.unary.child, sym_table);
            if (!function_call_result.success)
                return function_call_result;
            
            ast->node.unary.t.typing_information = function_call_result.type;
            return function_call_result;
        }
        case AST_COMMENT:
        case AST_STATEMENT_END:
        case AST_EOF:
        case AST_INVALID:
        case AST_MAX_TYPES:
        {
            return typing_result_success(TYPE_INFO_UNKNOWN);
        }
        case AST_IF_STATEMENT:
        {
            // We want to make sure the children of this node get type checked, but this node is just an unknown type
            // The left needs to go first, because the block might define a symbol we are using...
            typing_result expr_result = resolve_types(ast->node.ternary.left_child, sym_table);
            if (!expr_result.success)
                return expr_result;

            typing_result block_result = resolve_types(ast->node.ternary.center_child, sym_table);
            if (!block_result.success)
                return block_result;

            // TODO(Steven): This will allow the else block to use symbols defined in the if block... probably not okay...
            if (ast->node.ternary.right_child != NULL)
            {
                typing_result else_result = resolve_types(ast->node.ternary.right_child, sym_table);
                if (!else_result.success)
                    return else_result;
            }

            return typing_result_success(TYPE_INFO_UNKNOWN);
        }
        case AST_WHILE_STATEMENT:
        {
            // We want to make sure the children of this node get type checked, but this node is just an unknown type
            // The left needs to go first, because the block might define a symbol we are using...
            typing_result expr_result = resolve_types(ast->node.binary.left_child, sym_table);
            if (!expr_result.success)
                return expr_result;

            typing_result block_result = resolve_types(ast->node.binary.right_child, sym_table);
            if (!block_result.success)
                return block_result;

            return typing_result_success(TYPE_INFO_UNKNOWN);
        }
        case AST_SCOPE:
        {
            for (u64 i = 0; i < ast->node.many.children.number_of_nodes; ++i)
            {
                typing_result result = resolve_types(ast->node.many.children.nodes[i], sym_table);
                if (!result.success)
                {
                    // We got an error, bubble that up
                    return result;
                }
            }
            // We got all the way through this scope without throwing an error! return successfully
            return typing_result_success(TYPE_INFO_UNKNOWN);
        }
    };
}

typing_result typing_result_success(type_info tinfo)
{
    typing_result result = {};
    result.success = true;
    result.type = tinfo;

    return result;
}

typing_result typing_result_error(token t, char* message, ...)
{
    typing_result result = {};
    result.success = false;
    result.error.faulted_token = t;

    va_list params;
    va_start(params, message);
    result.error.message = format_error_message(message, params);
    va_end(params);

    return result;
}
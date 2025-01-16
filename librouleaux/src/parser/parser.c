#include "parser/parser.h"
#include "parser/abstract_syntax_tree.h"
#include "parser/node_list.h"

#include <assert.h>
#include <malloc.h>
#include <stdio.h>

/*
 * @BumpAllocator
 *     Currently using calloc/free, we might want to make a bump allocator
 *     for temporary allocations and use that instead.
 *          - Steven (March 11th, 2023)
 */

// A helper method for terminal parser nodes
parse_result parse_terminal(rouleaux_parser* parser, token_type t_type, ast_node_type node_type, const char* token_type_string);

// Will rearrange the tree such that it obeys operator precedence, returns true if nodes were rearranged
b8 fix_precedence(ast_node** original_root);

// Consumes the statement end operator if present and returns true, does nothing and returns false otherwise
b8 check_statement_end(rouleaux_parser* parser);

// parses a binary operator starting from the operator token, if successful returns the ast with the right child filled. The left child must be set by the caller
parse_result parse_binary_operator(rouleaux_parser* parser, ast_node_type type);



rouleaux_parser parser_create(const char* filename, node_allocator_fptr allocator, node_deallocator_fptr deallocator)
{
    rouleaux_parser parser = {};
    if (!allocator || !deallocator)
    {
        parser.has_error = true;
        printf("A rouleaux_parser MUST have both an allocator and a deallocator!");
        return parser;
    }

    parser.node_allocator = allocator;
    parser.node_deallocator = deallocator;

    parser.lexer = lexer_create(filename);

    parser.ast_head = parser.node_allocator(sizeof(ast_node));
    *parser.ast_head = ast_node_create(AST_INVALID);

    parser.has_error = false;

    return parser;
}

void parser_destroy(rouleaux_parser* parser)
{
    if (parser->ast_head)
    {
        ast_node_destroy(parser->ast_head, parser->node_deallocator);
        free(parser->ast_head);
        parser->ast_head = NULL;
    }

    lexer_destroy(&parser->lexer);
}

parse_result parser_parse_file(rouleaux_parser* parser)
{
    ast_node* file_node = parser_create_ast_node(parser, AST_SCOPE);
    file_node->node.many.children = node_list_create();

    parse_result result;
    do {
        result = parser_parse_statement(parser);

        if (result.success) // If we got a valid ast, we add it to our file_node
            node_list_push_back(&(file_node->node.many.children), result.resulting_tree);

    } while (result.success && !parser->done);

    if (!result.success)
    {
        // We failed to parse the file, destroy what we built so far and return the error
        parser_destroy_ast_node(parser, file_node);
        return result;
    }

    return parse_result_success(file_node);
}

parse_result parser_parse_statement(rouleaux_parser* parser)
{
    token t = lexer_peek_token(&parser->lexer);
    switch (t.type)
    {
        case TOKEN_IDENTIFIER: // If the line starts with an identifier, its an assignment node of some kind
        {
            return parser_parse_declaration_or_assignment(parser);
        }
        case TOKEN_KEYWORD_CALL:
        {
            ast_node* call_node = parser_create_ast_node(parser, AST_CALL_OPERATOR);
            call_node->node.unary.t = lexer_next_token(&parser->lexer);

            parse_result function_name_result = parser_parse_identifier(parser);
            if (!function_name_result.success)
            {
                parser_destroy_ast_node(parser, call_node);
                return function_name_result;
            }

            parse_result function_call_list_result = parser_parse_function_call_list(parser);
            if (!function_call_list_result.success)
            {
                parser_destroy_ast_node(parser, function_name_result.resulting_tree);
                parser_destroy_ast_node(parser, call_node);
                return function_call_list_result;
            }

            if (!check_statement_end(parser))
            {
                parser_destroy_ast_node(parser, function_name_result.resulting_tree);
                parser_destroy_ast_node(parser, call_node);
                return parse_result_error(lexer_peek_token(&parser->lexer), "Expected end of statement ';'");
            }

            ast_node* function_call_node = parser_create_ast_node(parser, AST_FUNCTION_CALL);
            function_call_node->node.binary.left_child = function_name_result.resulting_tree;
            function_call_node->node.binary.right_child = function_call_list_result.resulting_tree;

            call_node->node.unary.child = function_call_node;
            return parse_result_success(call_node);
        }
        case TOKEN_KEYWORD_IF:
        {
            parse_result if_result = parser_parse_keyword_if(parser);
            if (!if_result.success)
            {
                // This should be impossible!
                return if_result;
            }

            parse_result expression_result = parser_parse_expression_beginning(parser);
            if (!expression_result.success)
            {
                // There needs to be an expression after the if
                parser_destroy_ast_node(parser, if_result.resulting_tree);
                return expression_result;
            }

            parse_result statement_result = parser_parse_statement(parser);
            if (!statement_result.success)
            {
                // We need to have a statement after the if's expression
                parser_destroy_ast_node(parser, expression_result.resulting_tree);
                parser_destroy_ast_node(parser, if_result.resulting_tree);
                return statement_result;
            }

            // We have all the pieces, the left child of an if is its expression, and the center child is the statement
            // The right node will be optionally the else block
            if_result.resulting_tree->node.binary.left_child = expression_result.resulting_tree;
            if_result.resulting_tree->node.ternary.center_child = statement_result.resulting_tree;

            token else_token = lexer_peek_token(&parser->lexer);
            if (else_token.type != TOKEN_KEYWORD_ELSE)
            {
                // If the next token is not an else statement, that is all there is to the if block
                return if_result;
            }

            // if there was an else block, we need to grab that token and grab the else's block
            // make sure to take the else token
            else_token = lexer_next_token(&parser->lexer);
            parse_result else_block_result = parser_parse_statement(parser);
            if (!else_block_result.success)
            {
                // If the else block failed, destroy the if block and return the error
                parser_destroy_ast_node(parser, if_result.resulting_tree);
                return else_block_result;
            }

            // We successfully got the else block, assign that to the if nodes optional third child
            if_result.resulting_tree->node.ternary.right_child = else_block_result.resulting_tree;

            return if_result;
        }
        case TOKEN_KEYWORD_WHILE:
        {
            parse_result while_result = parser_parse_keyword_while(parser);
            if (!while_result.success)
            {
                // Impossible! Unreachable code
                return while_result;
            }

            parse_result expr_result = parser_parse_expression_beginning(parser);
            if (!expr_result.success)
            {
                // Destroy the while node and return
                parser_destroy_ast_node(parser, while_result.resulting_tree);
                return expr_result;
            }

            parse_result statement_result = parser_parse_statement(parser);
            if (!statement_result.success)
            {
                // Destroy the expr and while nodes and return the error
                parser_destroy_ast_node(parser, expr_result.resulting_tree);
                parser_destroy_ast_node(parser, while_result.resulting_tree);
                return statement_result;
            }

            // If we have all the pieces correctly, put them together
            // The left node is the expression and the right node is the block
            while_result.resulting_tree->node.binary.left_child = expr_result.resulting_tree;
            while_result.resulting_tree->node.binary.right_child = expr_result.resulting_tree;
            return while_result;
        }
        case TOKEN_LEFT_CURLY:
        {
            token open_curly_token = lexer_next_token(&parser->lexer);
            if (open_curly_token.type != TOKEN_LEFT_CURLY)
            {
                // This is impossible!
                return parse_result_error(open_curly_token, "*Compiler Bug* Impossible wrong token at start of scope!");
            }

            ast_node* scope_node = parser_create_ast_node(parser, AST_SCOPE);
            scope_node->node.many.children = node_list_create();

            // While the scope is not closing...
            token peeked_token = lexer_peek_token(&parser->lexer);
            while (peeked_token.type != TOKEN_RIGHT_CURLY && peeked_token.type != TOKEN_EOF)
            {
                parse_result statement_result = parser_parse_statement(parser);
                if (!statement_result.success)
                {
                    // Destroy what we worked on so far and return the error
                    parser_destroy_ast_node(parser, scope_node);
                    return statement_result;
                }

                node_list_push_back(&(scope_node->node.many.children), statement_result.resulting_tree);

                peeked_token = lexer_peek_token(&parser->lexer);
            }
            if (peeked_token.type != TOKEN_RIGHT_CURLY)
            {
                return parse_result_error(peeked_token, "Expected end of scope '}'");
            }

            // We need to grab this token before we leave
            token close_curly = lexer_next_token(&parser->lexer);
            if (close_curly.type != TOKEN_RIGHT_CURLY)
            {
                parser_destroy_ast_node(parser, scope_node);
                return parse_result_error(close_curly, "Expected a  closing curly bracket '}'");
            }

            return parse_result_success(scope_node);
        }
        case TOKEN_LINE_COMMENT:
        case TOKEN_BLOCK_COMMENT:
        {
            // We know the next token is a comment, so lets just parse it and return the result
            return parser_parse_comment(parser);
        }
        case TOKEN_EOF:
        {
            // The lexer has no more tokens in this file
            parser->done = true;
            return parse_result_success(parser_create_ast_node(parser, AST_EOF));
        }
        case TOKEN_INVALID:
        {
            return parse_result_error(t, "Invalid token found");
        }
        default:
        {
            return parse_result_error(t, "Expected the start of a statement, but instead got '%.*s'", t.length, t.text);
        }
    };
}

parse_result parser_parse_declaration_or_assignment(rouleaux_parser* parser)
{
    token identifier = lexer_next_token(&parser->lexer);
    parse_result assignment_result;

    token token_after_identifier = lexer_peek_token(&parser->lexer);
    switch (token_after_identifier.type)
    {
        case TOKEN_EQUALS:
        {
            // This is an assignment node
            assignment_result = parser_parse_value_assignment_operator(parser);
            break;
        }
        case TOKEN_COLON:
        {
            // This is either must be the type assignment node, because even for
            // a const-assignment there must be a type assign before it
            assignment_result = parser_parse_type_assignment_operator(parser);
            break;
        }
        default:
        {
            // anything else is a parse error
            return parse_result_error(token_after_identifier, "Invalid Statement, a identifier must be followed by either a value assignment ('=') or type assignment (':')");
        }
    };

    // No matter what we got, the identifier will be our left child
    assignment_result.resulting_tree->node.binary.left_child = parser_create_ast_node(parser, AST_IDENTIFIER);
    assignment_result.resulting_tree->node.binary.left_child->node.leaf.t = identifier;

    if (assignment_result.resulting_tree->type == AST_VALUE_ASSIGNMENT)
    {
        // If we got a value assignment, try to grab the expression
        parse_result expr_result = parser_parse_expression_beginning(parser);
        if (!expr_result.success)
        {
            // If we could not get the expression, destroy the nodes we made and return the error
            parser_destroy_ast_node(parser, assignment_result.resulting_tree);
            return expr_result;
        }

        // We got the expression, now set the right child of the value_assignment_node and return
        assignment_result.resulting_tree->node.binary.right_child = expr_result.resulting_tree;

        if (!check_statement_end(parser))
        {
            // If we could not grab the end of statement token, destroy what we built and return the error
            parser_destroy_ast_node(parser, assignment_result.resulting_tree);
            token not_end_token = lexer_peek_token(&parser->lexer);
            return parse_result_error(not_end_token, "Expected end of statement, but got ('%.*s')", not_end_token.length, not_end_token.text);
        }
        return assignment_result;
    }

    parse_result identifier_result = {};
    parse_result declaration_result = {};
    b8 found_declaration = false;
    do {
        token current_token = lexer_peek_token(&parser->lexer);
        switch (current_token.type)
        {
            case TOKEN_IDENTIFIER:
            {
                // This is the type identifier for our type assignment node
                identifier_result = parser_parse_identifier(parser);
                break;
            }
            case TOKEN_COLON:
            {
                // This is a const-assignment operator
                declaration_result = parser_parse_constant_assignment_operator(parser);
                found_declaration = true;
                break;
            }
            case TOKEN_EQUALS:
            {
                // This is a assignment operator
                declaration_result = parser_parse_value_assignment_operator(parser);
                found_declaration = true;
                break;
            }
            default:
            {
                return parse_result_error(current_token, "Invalid variable declaration, expected a const assignment (':') or a value assignment ('=')");
            }
        };
    } while (!found_declaration);

    if (identifier_result.success)
    {
        // If we found an identifier, we need to set it as the right child of our type_assignment
        assignment_result.resulting_tree->node.binary.right_child = identifier_result.resulting_tree;
    }
    // else // No need for else as the parse_result will be default constructed with null if no identifier was found
    declaration_result.resulting_tree->node.binary.left_child = assignment_result.resulting_tree;

    // We have the declaration node, now just grab the right side expression and make sure it ends with a semicolon
    parse_result expr_result = parser_parse_function_or_expression(parser);
    if (!expr_result.success)
    {
        parser_destroy_ast_node(parser, declaration_result.resulting_tree);
        return expr_result;
    }
    declaration_result.resulting_tree->node.binary.right_child = expr_result.resulting_tree;

    if (!check_statement_end(parser))
    {
        // If we could not grab the end of statement token, destroy what we built and return the error
        parser_destroy_ast_node(parser, declaration_result.resulting_tree);
        token not_end_token = lexer_peek_token(&parser->lexer);
        return parse_result_error(not_end_token, "Expected end of statement, but got ('%.*s')", not_end_token.length, not_end_token.text);
    }

    return declaration_result;
}

parse_result parser_parse_function_or_expression(rouleaux_parser* parser)
{
    // If we dont start with parens, we know its definitely not a function
    if (lexer_peek_token(&parser->lexer).type != TOKEN_LEFT_PAREN)
    {
        // We know this is NOT a function. So treat it like a expression and return
        return parser_parse_expression_beginning(parser);
    }

    // We are still not sure if its a function...
    token open_paren_token = lexer_next_token(&parser->lexer);
    token after_paren = lexer_peek_token(&parser->lexer);
    if (after_paren.type == TOKEN_RIGHT_PAREN)
    {
        // We know its a function with no params... Put the open paren back and call it as a function
        lexer_put_back_token(&parser->lexer, open_paren_token);
        return parser_parse_function_declaration(parser);
    }
    else if (after_paren.type != TOKEN_IDENTIFIER)
    {
        // We know functions need to have an identifier at this point. So treat this like an expression and return
        // Dont forget to put back that paren
        lexer_put_back_token(&parser->lexer, open_paren_token);
        return parser_parse_expression_beginning(parser);
    }

    // We are still not sure if its a function...
    token identifier_token = lexer_next_token(&parser->lexer);
    if (lexer_peek_token(&parser->lexer).type != TOKEN_COLON)
    {
        // We know functions need to have an type assignment at this point. So treat this like an expression and return
        // Dont forget to put back that paren and identifier
        lexer_put_back_token(&parser->lexer, identifier_token);
        lexer_put_back_token(&parser->lexer, open_paren_token);
        return parser_parse_expression_beginning(parser);
    }

    // Now we know its a function, So put the tokens back and treat it like one
    lexer_put_back_token(&parser->lexer, identifier_token);
    lexer_put_back_token(&parser->lexer, open_paren_token);
    return parser_parse_function_declaration(parser);
}

parse_result parser_parse_function_declaration(rouleaux_parser* parser)
{
    parse_result parameter_list_result = parser_parse_parameter_list(parser);
    if (!parameter_list_result.success)
    {
        return parameter_list_result;
    }

    parse_result return_type_result = parser_parse_return_type(parser);
    if (!return_type_result.success)
    {
        parser_destroy_ast_node(parser, parameter_list_result.resulting_tree);
        return return_type_result;
    }

    parse_result function_block_result = parser_parse_statement(parser);
    if (!function_block_result.success)
    {
        parser_destroy_ast_node(parser, return_type_result.resulting_tree);
        parser_destroy_ast_node(parser, parameter_list_result.resulting_tree);
        return return_type_result;
    }

    ast_node* function_node = parser_create_ast_node(parser, AST_FUNCTION_DECLARATION);
    function_node->node.ternary.left_child = parameter_list_result.resulting_tree;
    function_node->node.ternary.center_child = return_type_result.resulting_tree;
    function_node->node.ternary.right_child = function_block_result.resulting_tree;

    return parse_result_success(function_node);
}

parse_result parser_parse_parameter_list(rouleaux_parser* parser)
{
    token open_paren = lexer_next_token(&parser->lexer);
    if (open_paren.type != TOKEN_LEFT_PAREN)
    {
        return parse_result_error(open_paren, "Expected start of function parameter list ('(')");
    }

    ast_node* param_list_node = parser_create_ast_node(parser, AST_PARAMETER_LIST);
    param_list_node->node.many.children = node_list_create();

    token t = lexer_peek_token(&parser->lexer);
    while (t.type != TOKEN_RIGHT_PAREN && t.type != TOKEN_EOF)
    {
        parse_result type_assign_result = parser_parse_function_declaration_parameter(parser);
        if (!type_assign_result.success)
        {
            parser_destroy_ast_node(parser, param_list_node);
            return type_assign_result;
        }

        node_list_push_back(&(param_list_node->node.many.children), type_assign_result.resulting_tree);

        t = lexer_next_token(&parser->lexer);
        if (t.type != TOKEN_COMMA && t.type != TOKEN_RIGHT_PAREN)
        {
            parser_destroy_ast_node(parser, param_list_node);
            return parse_result_error(t, "Expected comma separated parameters in function or parameter list end");
        }
        if (t.type == TOKEN_RIGHT_PAREN)
            lexer_put_back_token(&parser->lexer, t);
        
        t = lexer_peek_token(&parser->lexer);
    }

    if (t.type == TOKEN_EOF)
    {
        parser_destroy_ast_node(parser, param_list_node);

        char* open_paren_location_text = location_printable_text(open_paren.location, calloc);
        parse_result result = parse_result_error(t, "Reached end of file before finishing function parameter list. Did you forget a closing parenthesis around [%s]?", open_paren_location_text);
        free(open_paren_location_text);

        return result;
    }

    token close_paren = lexer_next_token(&parser->lexer);
    if (close_paren.type != TOKEN_RIGHT_PAREN)
    {
        parser_destroy_ast_node(parser, param_list_node);
        return parse_result_error(close_paren, "Expected closing of function parameter list");
    }
    
    return parse_result_success(param_list_node);
}

parse_result parser_parse_function_call_list(rouleaux_parser* parser)
{
    token open_paren = lexer_next_token(&parser->lexer);
    if (open_paren.type != TOKEN_LEFT_PAREN)
    {
        return parse_result_error(open_paren, "Expected start of function call list");
    }

    ast_node* param_list_node = parser_create_ast_node(parser, AST_PARAMETER_LIST);
    param_list_node->node.many.children = node_list_create();

    token t = lexer_peek_token(&parser->lexer);
    while (t.type != TOKEN_RIGHT_PAREN)
    {
        parse_result expr_result = parser_parse_expression_beginning(parser);
        if (!expr_result.success)
        {
            parser_destroy_ast_node(parser, param_list_node);
            return expr_result;
        }
        node_list_push_back(&(param_list_node->node.many.children), expr_result.resulting_tree);

        token comma_or_paren_token = lexer_peek_token(&parser->lexer);
        // TODO(Steven): This is messy and can probably be done in a better way...
        if (comma_or_paren_token.type == TOKEN_RIGHT_PAREN)
            break;
        else if (comma_or_paren_token.type == TOKEN_EOF)
            return parse_result_error(comma_or_paren_token, "Reached end of file before completing the function call list");
        else if (comma_or_paren_token.type == TOKEN_COMMA)
        {
            // Grab the comma before looping again
            t = lexer_next_token(&parser->lexer);
        }
        else
            return parse_result_error(comma_or_paren_token, "Unexpected token in function call list");
    }

    // If we got here its because the loop ended with a close paren, we need to take that off the lexer and return
    lexer_next_token(&parser->lexer);

    return parse_result_success(param_list_node);
}

parse_result parser_parse_function_declaration_parameter(rouleaux_parser* parser)
{
    parse_result name_result = parser_parse_identifier(parser);
    if (!name_result.success)
    {
        return name_result;
    }

    parse_result type_assign_result = parser_parse_type_assignment_operator(parser);
    if (!type_assign_result.success)
    {
        parser_destroy_ast_node(parser, name_result.resulting_tree);
        return type_assign_result;
    }

    parse_result type_result = parser_parse_identifier(parser);
    if (!type_result.success)
    {
        parser_destroy_ast_node(parser, type_assign_result.resulting_tree);
        parser_destroy_ast_node(parser, name_result.resulting_tree);
        return type_result;
    }

    type_assign_result.resulting_tree->node.binary.left_child = name_result.resulting_tree;
    type_assign_result.resulting_tree->node.binary.right_child = type_result.resulting_tree;
    return type_assign_result;
}

parse_result parser_parse_return_type(rouleaux_parser* parser)
{
    token arrow = lexer_next_token(&parser->lexer);
    if (arrow.type != TOKEN_ARROW)
    {
        return parse_result_error(arrow, "Expected start of function return type ('->'), but got '%.*s'", arrow.length, arrow.text);
    }

    token identifier = lexer_next_token(&parser->lexer);
    if (identifier.type != TOKEN_IDENTIFIER)
    {
        return parse_result_error(identifier, "Expected a function return type, but got '%.*s'", identifier.length, identifier.text);
    }

    ast_node* return_type_node = parser_create_ast_node(parser, AST_IDENTIFIER);
    return_type_node->node.leaf.t = identifier;
    return parse_result_success(return_type_node);
}

parse_result parser_parse_expression_beginning(rouleaux_parser* parser)
{
    token t = lexer_peek_token(&parser->lexer);
    switch (t.type)
    {
        case TOKEN_LEFT_PAREN:
        {
            token open_paren = lexer_next_token(&parser->lexer);

            // Parse the expression inside the parens
            parse_result result = parser_parse_expression_beginning(parser);
            if (!result.success) // If we failed bubble the error result back up
                return result;
            
            token maybe_close_paren = lexer_peek_token(&parser->lexer);
            if (maybe_close_paren.type != TOKEN_RIGHT_PAREN)
            {
                // There is no closing paren!
                // @BumpAllocator
                char* location_text = location_printable_text(open_paren.location, calloc);
                parse_result result = parse_result_error(maybe_close_paren, "Expected a closing parenthesis, but got '%.*s'. Expecting a closing parenthesis for opening found here [%s]", maybe_close_paren.length, maybe_close_paren.text, location_text);
                free(location_text);

                return result;
            }

            // We got the closing paren! we succeeded, mark that this expression is in parens!
            result.resulting_tree->enclosed_in_parens = true;
            lexer_next_token(&parser->lexer); // We also need to grab that close paren because its a part of this node!

            // There could be more to this expression!
            parse_result after_paren_result = parser_parse_expression(parser);
            if (!after_paren_result.success)
            {
                // We can get rid of the error and return the result from above
                parse_result_destroy(&after_paren_result);
                return result;
            }

            // We successfully got a expression, so we are that new expressions left child
            after_paren_result.resulting_tree->node.binary.left_child = result.resulting_tree;
            fix_precedence(&after_paren_result.resulting_tree);
            return after_paren_result;
        }
        case TOKEN_IDENTIFIER:
        {
            token identifier = lexer_next_token(&parser->lexer);
            ast_node* expression = parser_create_ast_node(parser, AST_IDENTIFIER);
            expression->node.leaf.t = identifier;

            // Make sure its not a function call!
            if (lexer_peek_token(&parser->lexer).type == TOKEN_LEFT_PAREN)
            {
                parse_result parameters_result = parser_parse_function_call_list(parser);
                if (!parameters_result.success)
                {
                    return parameters_result;
                }

                // It was a function call and we need to pack on some parameters
                ast_node* identifier_node = expression;
                expression = parser_create_ast_node(parser, AST_FUNCTION_CALL);
                expression->node.binary.left_child = identifier_node;
                expression->node.binary.right_child = parameters_result.resulting_tree;
            }

            // Attempt to parse the next in the chain
            parse_result result = parser_parse_expression(parser);
            if (!result.success)
            {
                // If we could not get a valid expression out of the right hand side,
                // free that error and just return this identifier
                parse_result_destroy(&result);

                return parse_result_success(expression);
            }

            // If we could parse more of the expression, set the above expression to the left child
            result.resulting_tree->node.binary.left_child = expression;
            fix_precedence(&result.resulting_tree);
            return result;
        }
        case TOKEN_INTEGER_LITERAL:
        {
            token int_literal = lexer_next_token(&parser->lexer);
            ast_node* expression = parser_create_ast_node(parser, AST_INTEGER_LITERAL);
            expression->node.leaf.t = int_literal;

            // Attempt to parse the next in the chain
            parse_result result = parser_parse_expression(parser);
            if (!result.success)
            {
                // If we could not get a valid expression out of the right hand side,
                // free that error and just return this identifier
                parse_result_destroy(&result);

                return parse_result_success(expression);
            }

            // If we could parse more of the expression, set the above expression to the left child
            result.resulting_tree->node.binary.left_child = expression;
            fix_precedence(&result.resulting_tree);
            return result;
        }
        case TOKEN_FLOAT_LITERAL:
        {
            token float_literal = lexer_next_token(&parser->lexer);
            ast_node* expression = parser_create_ast_node(parser, AST_FLOAT_LITERAL);
            expression->node.leaf.t = float_literal;

            // Attempt to parse the next in the chain
            parse_result result = parser_parse_expression(parser);
            if (!result.success)
            {
                // If we could not get a valid expression out of the right hand side,
                // free that error and just return this identifier
                parse_result_destroy(&result);

                return parse_result_success(expression);
            }

            // If we could parse more of the expression, set the above expression to the left child
            result.resulting_tree->node.binary.left_child = expression;
            fix_precedence(&result.resulting_tree);
            return result;
        }
        case TOKEN_STRING_LITERAL:
        {
            token string_literal_token = lexer_next_token(&parser->lexer);
            ast_node* expression = parser_create_ast_node(parser, AST_STRING_LITERAL);
            expression->node.leaf.t = string_literal_token;

            parse_result result = parser_parse_expression(parser);
            if (!result.success)
            {
                // If we could not get a valid expression out of the right hand side,
                // free that error and just return this identifier
                parse_result_destroy(&result);

                return parse_result_success(expression);
            }

            // If we could parse more of the expression, set the above expression to the left child
            result.resulting_tree->node.binary.left_child = expression;
            fix_precedence(&result.resulting_tree);
            return result;
        }
        default:
        {
            return parse_result_error(t, "Expected the start of an expression, but instead got '%.*s'", t.length, t.text);
        }
    };
}

parse_result parser_parse_expression(rouleaux_parser* parser)
{
    token t = lexer_peek_token(&parser->lexer);
    switch (t.type)
    {
        case TOKEN_PLUS:
        {
            return parse_binary_operator(parser, AST_BINARY_OPERATOR_PLUS);
        }
        case TOKEN_MINUS:
        {
            return parse_binary_operator(parser, AST_BINARY_OPERATOR_MINUS);
        }
        case TOKEN_ASTERISK:
        {
            return parse_binary_operator(parser, AST_BINARY_OPERATOR_MULTIPLY);
        }
        case TOKEN_FORWARD_SLASH:
        {
            return parse_binary_operator(parser, AST_BINARY_OPERATOR_DIVIDE);
        }
        case TOKEN_PERCENT:
        {
            return parse_binary_operator(parser, AST_BINARY_OPERATOR_MODULUS);
        }
        case TOKEN_GREATER_THAN:
        {
            return parse_binary_operator(parser, AST_BINARY_OPERATOR_GREATER_THAN);
        }
        case TOKEN_LESS_THAN:
        {
            return parse_binary_operator(parser, AST_BINARY_OPERATOR_LESS_THAN);
        }
        default:
        {
            return parse_result_error(t, "Unexpected token '%.*s', in expression", t.length, t.text);
        }
    };
}

parse_result parser_parse_identifier(rouleaux_parser* parser)
{
    return parse_terminal(parser, TOKEN_IDENTIFIER, AST_IDENTIFIER, "identifier");
}

parse_result parser_parse_integer_literal(rouleaux_parser* parser)
{
    return parse_terminal(parser, TOKEN_INTEGER_LITERAL, AST_INTEGER_LITERAL, "integer literal");
}

parse_result parser_parse_float_literal(rouleaux_parser* parser)
{
    return parse_terminal(parser, TOKEN_FLOAT_LITERAL, AST_FLOAT_LITERAL, "float literal");
}

parse_result parser_parse_string_literal(rouleaux_parser* parser)
{
    return parse_terminal(parser, TOKEN_STRING_LITERAL, AST_STRING_LITERAL, "string literal");
}

parse_result parser_parse_comment(rouleaux_parser* parser)
{
    parse_result line_comment_result = parse_terminal(parser, TOKEN_LINE_COMMENT, AST_COMMENT, "comment");
    if (line_comment_result.success)
        return line_comment_result;
    
    // We did not find a line comment, but could still find a block comment.
    // So lets get rid of this error message...
    parse_result_destroy(&line_comment_result);

    parse_result block_comment_result = parse_terminal(parser, TOKEN_BLOCK_COMMENT, AST_COMMENT, "comment");

    // We can return the block comment result wether it succeeded or failed
    return block_comment_result;
}

parse_result parser_parse_value_assignment_operator(rouleaux_parser* parser)
{
    return parse_terminal(parser, TOKEN_EQUALS, AST_VALUE_ASSIGNMENT, "value assignment operator('=')");
}

parse_result parser_parse_constant_assignment_operator(rouleaux_parser* parser)
{
    return parse_terminal(parser, TOKEN_COLON, AST_CONST_ASSIGNMENT, "constant assignment operator(':')");
}

parse_result parser_parse_type_assignment_operator(rouleaux_parser* parser)
{
    return parse_terminal(parser, TOKEN_COLON, AST_TYPE_ASSIGNMENT, "type assignment operator(':')");
}

parse_result parser_parse_statement_end_operator(rouleaux_parser* parser)
{
    return parse_terminal(parser, TOKEN_SEMICOLON, AST_STATEMENT_END, "semicolon (';')");
}

parse_result parser_parse_keyword_if(rouleaux_parser* parser)
{
    return parse_terminal(parser, TOKEN_KEYWORD_IF, AST_IF_STATEMENT, "if statement");
}

parse_result parser_parse_keyword_while(rouleaux_parser* parser)
{
    return parse_terminal(parser, TOKEN_KEYWORD_WHILE, AST_WHILE_STATEMENT, "while statement");
}

ast_node* parser_create_ast_node(rouleaux_parser* parser, ast_node_type type)
{
    ast_node* out_node;
    out_node = parser->node_allocator(sizeof(ast_node));
    *out_node = ast_node_create(type);

    return out_node;
}

void parser_destroy_ast_node(rouleaux_parser* parser, ast_node* node)
{
    ast_node_destroy(node, parser->node_deallocator);
}



parse_result parse_terminal(rouleaux_parser* parser, token_type t_type, ast_node_type node_type, const char* token_type_string)
{
    token t = lexer_peek_token(&parser->lexer);

    if (t.type == t_type)
    {
        ast_node* node = parser_create_ast_node(parser, node_type);
        // Now we actually want to grab that token
        node->node.leaf.t = lexer_next_token(&parser->lexer);
        return parse_result_success(node);
    }

    return parse_result_error(t, "Expected a %s, but got '%.*s'", token_type_string, t.length, t.text);
}

b8 fix_precedence(ast_node** original_root)
{
    // Get a reference to the original
    ast_node* original = *original_root;
    // Get a reference to the original right child
    ast_node* right_child = original->node.binary.right_child;

    i32 original_root_precedence = precedence_from_node_type(original->type);
    i32 original_right_child_precedence = precedence_from_node_type(right_child->type);

    if (original_right_child_precedence < 0)
    {
        // The right child is not an operator, no need for reordering
        return false;
    }

    if (original->enclosed_in_parens || ((original_root_precedence > original_right_child_precedence) && !right_child->enclosed_in_parens))
    {
        // Reference to the original left child
        ast_node* left_child = right_child->node.binary.left_child;

        // The left child of the lower precedence node becomes the right child of the higher
        original->node.binary.right_child = left_child;

        // The higher precedence node becomes the right child of the lower
        right_child->node.binary.left_child = original;

        // the lower precedence becomes the root
        *original_root = right_child;

        return true;
    }

    return false;
}

b8 check_statement_end(rouleaux_parser* parser)
{
    token end_token = lexer_peek_token(&parser->lexer);
    if (end_token.type == TOKEN_SEMICOLON)
    {
        lexer_next_token(&parser->lexer);
        return true;
    }

    return false;
}

parse_result parse_binary_operator(rouleaux_parser* parser, ast_node_type type)
{
    // Grab the token so we can parse the expression to the right
    token operator_token = lexer_next_token(&parser->lexer);

    // Now parse the right hand to expression to get the right_child
    parse_result expr_result = parser_parse_expression_beginning(parser);
    if (!expr_result.success)
    {
        // We need to put back the plus operator because we failed to use it meaningfully
        lexer_put_back_token(&parser->lexer, operator_token);
        
        // If we failed, we have an invalid right child and should return the error
        return expr_result;
    }

    ast_node* binary_operator = parser_create_ast_node(parser, type);
    binary_operator->node.binary.t = operator_token;

    // We successfully got the right hand side expression, set that to our right child
    binary_operator->node.binary.right_child = expr_result.resulting_tree;

    return parse_result_success(binary_operator);
}
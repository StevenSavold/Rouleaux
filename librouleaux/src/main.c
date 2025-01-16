#include "parser/parser.h"
#include "parser/parser_allocators.h"
#include "utilities/error_report.h"

#include <stdio.h>
#include <malloc.h>

int print_usage(const char* program_name);
void lexer_test(const char* filename);
void parser_test(const char* filename);


int main(int argc, char** argv)
{
    if (argc < 2)
        return print_usage(argv[0]);

    // lexer_test(argv[1]);
    parser_test(argv[1]);

    return 0;
}


int print_usage(const char* program_name)
{
    printf("%s rouleaux_file", program_name);
    return 1;
}

void lexer_test(const char* filename)
{
    rouleaux_lexer lexer = lexer_create(filename);
    if (lexer.has_error)
    {
        printf("FATAL: unable to create lexer! exiting...");
        return;
    }

    token tokens[128] = {};
    for (i32 i = 0; i < 128; ++i)
    {
        tokens[i] = lexer_next_token(&lexer);
        if (lexer.has_error)
        {
            printf("Lexer encountered an error at [%s:%lld:%lld]\n", tokens[i].location.filename, tokens[i].location.row, tokens[i].location.column);
            break;
        }
        token_print(tokens[i]);
        printf("\n");

        if (tokens[i].type == TOKEN_EOF)
        {
            printf("Reached EOF before end of buffer! At token #%d\n", i + 1);
            break;
        }
    }
}

void parser_test(const char* filename)
{
    rouleaux_parser parser = parser_create(filename, default_node_allocator, default_node_deallocator);

    do {
        parse_result result = parser_parse_statement(&parser);
        if (!result.success)
        {
            // Report the error!
            char* error_text = error_report_printable_text(result.error, calloc);
            printf("%s", error_text);
            free(error_text);
            __debugbreak();
        }

        if (result.resulting_tree->type != AST_COMMENT)
        {
            __debugbreak();
        }

        parser_destroy_ast_node(&parser, result.resulting_tree);
    } while(!parser.done);

    printf("successfully parsed the whole file!");

    return;
}

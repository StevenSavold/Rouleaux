#include <rouleaux/rouleaux.h>
#include <stdio.h>
#include <malloc.h>

int print_usage(const char* program_name);

int main(int argc, char** argv)
{
    if (argc < 2)
        return print_usage(argv[0]);

    int return_code = 0;

    rouleaux_parser parser = parser_create(argv[1], default_node_allocator, default_node_deallocator);
    parse_result ast = parser_parse_file(&parser);
    if (!ast.success)
    {
        char* error_text = error_report_printable_text(ast.error, calloc);
        printf("%s", error_text);
        free(error_text);

        return_code = 1;
        goto cleanup_parser;
    }

    // Make the type table
    symbol_table sym_table = symbol_table_create();
    typing_result result = resolve_types(ast.resulting_tree, &sym_table);
    if (!result.success)
    {
        char* error_text = error_report_printable_text(result.error, calloc);
        printf("%s", error_text);
        free(error_text);

        return_code = 1;
        goto cleanup_symbol_table;
    }

    printf("Symbol Table:\n");
    for (u64 i = 0; i < sym_table.size; ++i)
    {
        printf("\t%.*s = ", (int)sym_table.buffer[i].t.length, sym_table.buffer[i].t.text);
        if (sym_table.buffer[i].type == TYPE_INFO_INTEGER)
        {
            printf("%lld\n", sym_table.buffer[i].t.value.unsigned64);
        }
        else
        {
            printf("%lf\n", sym_table.buffer[i].t.value.float64);
        }
    }

    printf("Success!\n");

cleanup_symbol_table:
    parser_destroy_ast_node(&parser, ast.resulting_tree);
    symbol_table_destroy(&sym_table);
cleanup_parser:
    parser_destroy(&parser);

    return return_code;
}

int print_usage(const char* program_name)
{
    printf("%s rouleaux_file", program_name);
    return 1;
}

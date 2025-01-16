#include "parser/parse_result.h"
#include "utilities/error_report.h"

#include <stdarg.h>
#include <malloc.h>

parse_result parse_result_success(ast_node* resulting_tree)
{
    parse_result result = {};
    result.success = true;
    result.resulting_tree = resulting_tree;

    return result;
}


parse_result parse_result_error(token t, char* message, ...)
{
    parse_result result = {};
    result.success = false;
    result.error.faulted_token = t;

    va_list params;
    va_start(params, message);
    result.error.message = format_error_message(message, params);
    va_end(params);

    return result;
}

void parse_result_destroy(parse_result* result)
{
    if (!result->success)
    {
        free(result->error.message);
        result->error.message = NULL;
    }
}

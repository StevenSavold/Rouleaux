#include "parser/parser_allocators.h"

#include <malloc.h>


void* default_node_allocator(u64 size)
{
    return malloc(size);
}

void default_node_deallocator(void* block, u64 size)
{
    (void)size;
    free(block);
}

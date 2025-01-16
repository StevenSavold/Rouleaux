#include "parser/node_list.h"
#include "parser/abstract_syntax_tree.h"
#include <malloc.h>
#include <string.h>

#define DEFAULT_NODE_LIST_CAPACITY        1
#define DEFAULT_NODE_LIST_RESIZE_FACTOR   2


static b8 reallocate_buffer(node_list* list, u32 resize_factor);


node_list node_list_create()
{
    node_list list = {};
    list.nodes = malloc(DEFAULT_NODE_LIST_CAPACITY * sizeof(ast_node*));
    list.capacity = DEFAULT_NODE_LIST_CAPACITY;

    return list;
}

void node_list_destroy(node_list* list, void(*node_deallocator)(void* block, u64 size))
{
    for (u64 i = 0; i < list->number_of_nodes; ++i)
    {
        ast_node_destroy(list->nodes[i], node_deallocator);
    }

    free(list->nodes);
    list->nodes = NULL;
    list->number_of_nodes = 0;
    list->capacity = 0;
}

b8 node_list_push_back(node_list* list, ast_node* node)
{
    if (list->number_of_nodes + 1 >= list->capacity)
    {
        if (!reallocate_buffer(list, DEFAULT_NODE_LIST_RESIZE_FACTOR))
            return false;
    }

    list->nodes[list->number_of_nodes] = node; // insert at the back of the list
    list->number_of_nodes++;

    return true;
}

b8 node_list_pop_back(node_list* list, ast_node** out_node)
{
    if (list->number_of_nodes == 0)
        return false;

    *out_node = list->nodes[list->number_of_nodes - 1];
    list->number_of_nodes--;

    return true;
}


b8 reallocate_buffer(node_list* list, u32 resize_factor)
{
    u64 new_capacity = list->capacity * resize_factor;
    ast_node** new_buffer = malloc(new_capacity * sizeof(ast_node*));

    int error_code = memcpy_s(new_buffer, new_capacity * sizeof(ast_node*), list->nodes, list->capacity * sizeof(ast_node*));
    if (error_code)
        return false; // we failed to copy the memory

    free(list->nodes);
    list->nodes = new_buffer;
    list->capacity = new_capacity;

    return true; // We successfully reallocated the list buffer
}
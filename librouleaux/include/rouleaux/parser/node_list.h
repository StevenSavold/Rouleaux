#pragma once

#include "defines.h"

#define DEFAULT_NODE_LIST_CAPACITY 1
#define DEFAULT_NODE_LIST_

// Forward declare ast_node
struct ast_node;

/**
 * @brief A node list is a dynamic array which holds nodes of the abstract syntax tree
 * @note This is used by nodes with a child strategy of MANY to hold as many nodes as they need
 */
typedef struct node_list {
    /* The list of node pointers the list owns all the nodes in this list */
    struct ast_node** nodes;

    /* The number of nodes in the list */
    u64 number_of_nodes;

    /* The number of ast_node pointer's the list can currently hold */
    u64 capacity;
} node_list;

/**
 * @brief creates a node_list object with a default amount of capacity and 0 elements
 * 
 * @return node_list the list which was allocated
 */
API node_list node_list_create();

/**
 * @brief deallocates the memory for all nodes in the list then 
 * 
 * @param list the list to destroy
 * @param deallocator the function to use when deallocating the nodes in the list
 */
API void node_list_destroy(node_list* list, void(*node_deallocator)(void* block, u64 size));

/**
 * @brief pushs a node to the back of the list
 * 
 * @param list the list to operate on
 * @param node the pointer to the node which the list will take ownership of
 * @return b8 true if we successfully pushed the node into the list, false otherwise
 */
API b8 node_list_push_back(node_list* list, struct ast_node* node);

/**
 * @brief removes a node pointer from the list
 * @note the caller now owns the node found at the pointer and needs to free it
 * 
 * @param list the list to operate on
 * @param node a pointer to a place to set the node pointer being popped from the list
 * @return b8
 */
API b8 node_list_pop_back(node_list* list, struct ast_node** node);

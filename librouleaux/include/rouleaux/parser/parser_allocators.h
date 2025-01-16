#pragma once

#include "defines.h"

/**
 * @brief The default allocator for a rouleaux_parser. This is just a wrapper around malloc()
 * 
 * @param size the size in bytes
 * @return void* a pointer to the newly allocated block of memory
 */
API void* default_node_allocator(u64 size);

/**
 * @brief The default deallocator for a rouleaux_parser. This is just a wrapper around free()
 * 
 * @param block the pointer to the block to be free'd
 * @param size the size of the block in bytes
 */
API void default_node_deallocator(void* block, u64 size);

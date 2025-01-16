#include "lexer/peek_queue.h"

#include <malloc.h>
#include <string.h>

#define DEFAULT_PEEK_QUEUE_RESIZE_FACTOR 2

static b8 reallocate_buffer(peek_queue* queue, u32 resize_factor);

peek_queue peek_queue_create(u64 capacity)
{
    peek_queue queue = {};
    queue.capacity = capacity;
    queue.size = 0;
    queue.buffer = malloc(capacity * sizeof(token));

    return queue;
}

void peek_queue_destroy(peek_queue* queue)
{
    if (queue->buffer)
    {
        free(queue->buffer);
        queue->buffer = NULL;
    }
    queue->size = 0;
    queue->capacity = 0;
}

void peek_queue_empty(peek_queue* queue)
{
    queue->size = 0;
}

b8 peek_queue_push(peek_queue* queue, token t)
{
    // If we are about to overflow, realloc
    if (queue->size + 1 >= queue->capacity)
    {
        if (!reallocate_buffer(queue, DEFAULT_PEEK_QUEUE_RESIZE_FACTOR))
            return false; // We failed to add the token
    }

    // Loop backwards from the queue size, and copy all the elements up one index
    if (queue->size > 0)
        for (u64 i = queue->size; i >= 0; --i)
            queue->buffer[i + 1] = queue->buffer[i];

    queue->buffer[0] = t; // insert the token in the front of the buffer (which is the back of the queue)
    queue->size++;

    return true;
}

b8 peek_queue_pop(peek_queue* queue, token* out_token)
{
    b8 did_get_token = peek_queue_front(queue, out_token);
    if (did_get_token)
        queue->size--;
    
    return did_get_token;
}

b8 peek_queue_front(peek_queue* queue, token* out_token)
{
    if (queue->size == 0)
        return false;

    *out_token = queue->buffer[queue->size - 1];

    return true;
}

b8 peek_queue_push_front(peek_queue* queue, token t)
{
    // If we are about to overflow, realloc
    if (queue->size + 1 >= queue->capacity)
    {
        if (!reallocate_buffer(queue, DEFAULT_PEEK_QUEUE_RESIZE_FACTOR))
            return false; // We failed to add the token
    }

    queue->buffer[queue->size] = t; // insert the token in the back of the buffer (which is the front of the queue)
    queue->size++;

    return true;
}


b8 reallocate_buffer(peek_queue* queue, u32 resize_factor)
{
    u64 new_capacity = queue->capacity * resize_factor;
    token* new_buffer = malloc(new_capacity * sizeof(token));

    int error_code = memcpy_s(new_buffer, new_capacity * sizeof(token), queue->buffer, queue->capacity * sizeof(token));
    if (error_code)
        return false; // we failed to copy the memory

    free(queue->buffer);
    queue->buffer = new_buffer;
    queue->capacity = new_capacity;

    return true; // We successfully reallocated the queue buffer
}
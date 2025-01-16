#pragma once

#include "defines.h"
#include "lexer/token.h"

//
// TODO(Steven): Interface improvement, add push_many and pop_many
//

/**
 * @brief A FIFO queue which holds tokens
 * Used by rouleaux_lexer to hold peeked tokens
 * 
 */
typedef struct peek_queue {
    /* The buffer holding the tokens */
    token* buffer;
    /* The amount of tokens currently held by the queue */
    u64 size;
    /* The amount of tokens the buffer can currently hold */
    u64 capacity;
} peek_queue;

/**
 * @brief Creates a peek_queue with a given initial capacity
 * 
 * @param capacity the amount of tokens the queue should be able to initially hold
 * @return peek_queue the create queue
 */
API peek_queue peek_queue_create(u64 capacity);

/**
 * @brief releases the internal memory of the given queue
 * 
 * @param queue the queue to be released
 */
API void peek_queue_destroy(peek_queue* queue);

/**
 * @brief Empties the queue of all elements
 * 
 * @param queue 
 */
API void peek_queue_empty(peek_queue* queue);

/**
 * @brief pushes a token onto the back of the queue
 * 
 * @param queue the queue to be pushed to
 * @param t the token to be copied into the queue
 * @return b8 true if the token was successfully added to the queue
 */
API b8 peek_queue_push(peek_queue* queue, token t);

/**
 * @brief removes the first element in the queue
 * 
 * @param queue the queue to be popped from
 * @param out_token the memory to copy the outgoing token to
 * @return b8 true, if the out_token was populated with a token, false otherwise
 */
API b8 peek_queue_pop(peek_queue* queue, token* out_token);

/**
 * @brief gets the first token in the peek_queue
 * 
 * @param queue the queue to operate on
 * @param out_token the memory to copy the token into
 * @return b8 true if a token was successfully gotten, false otherwise
 */
API b8 peek_queue_front(peek_queue* queue, token* out_token);

/**
 * @brief pushes a token to the front of the queue
 * 
 * @param queue the queue to push to
 * @param t the token to push
 * @return b8 true if operation succeed, false otherwise
 */
API b8 peek_queue_push_front(peek_queue* queue, token t);

#pragma once

#include "defines.h"
#include "lexer/token.h"
#include "lexer/peek_queue.h"


typedef struct rouleaux_lexer {
    /* The name of the file being lexed by this lexer */
    const char* filename;
    /* The buffer holding the content of the file being lexed by this lexer */
    char* file_content;
    /* The length of the file_content buffer in bytes */
    u64 file_content_length;
    /* The pointer to the end of the file_content buffer */
    char* file_end;

    /* The current point in the file_content the lexer is reading from */
    char* head;

    /* A FIFO queue of already lexed tokens */
    peek_queue peek_buffer;

    /* The current row the lexer is lexing from */
    u64 current_row;
    /* The current column the lexer is lexing from */
    u64 current_column;

    /* a boolean which is set to true when the lexer is in an invalid state */
    b8 has_error;
} rouleaux_lexer;

/**
 * @brief Creates a lexer from a given filename
 * 
 * @note the lexer's has_error feild will be set to true if creation fails
 * 
 * @param filename the name of the file to lex
 * @return rouleaux_lexer a lexer of the given file
 */
API rouleaux_lexer lexer_create(const char* filename);

/**
 * @brief frees any allocated memory a lexer is holding and zeros the struct
 * 
 * @param lexer the lexer to release the resources of
 */
API void lexer_destroy(rouleaux_lexer* lexer);

/**
 * @brief resets a lexer to the state it was in just after its initial creation
 * 
 * @param lexer the lexer to reset
 * @return b8 true if the lexer is in a valid state, false otherwise
 */
API b8 lexer_reset(rouleaux_lexer* lexer);

/**
 * @brief returns the next token in the file (will pull from the peek_queue first if a token is present there)
 * 
 * @param lexer the lexer to get the next token from
 * @return token the next token in the stream
 */
API token lexer_next_token(rouleaux_lexer* lexer);

/**
 * @brief parses the next token and returns it, but adds it to a buffer for the next time lexer_next_token() is called
 * 
 * @param lexer the lexer to get the token from
 * @return token the next token the lexer will return from lexer_next_token()
 */
API token lexer_peek_token(rouleaux_lexer* lexer);

/**
 * @brief puts the given token at the front of the peek_queue
 * 
 * @param lexer the lexer to put the token into
 * @param t the token to put back
 * @return b8 true if the operation was successful, false otherwise
 */
API b8 lexer_put_back_token(rouleaux_lexer* lexer, token t);

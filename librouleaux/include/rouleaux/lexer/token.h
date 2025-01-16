#pragma once

#include "defines.h"

typedef enum token_type {
    TOKEN_INVALID = 0,

    // Identifiers & Keywords
    // WARN!: The ordering of the keywords needs to match that of the global keywords array! and they need to start at 1
    TOKEN_KEYWORD_FOR = 1,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_DO,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_NULL,
    TOKEN_KEYWORD_CALL,

    TOKEN_IDENTIFIER,

    // Single character tokens (enum value is equal to the ascii value)
    TOKEN_EXCLAMATION = 33,    // '!'
    TOKEN_DOUBLE_QUOTE,        // '"'
    TOKEN_POUND,               // '#'
    TOKEN_DOLLAR_SIGN,         // '$'
    TOKEN_PERCENT,             // '%'
    TOKEN_AMPERSAND,           // '&'
    TOKEN_SINGLE_QUOTE,        // '''
    TOKEN_LEFT_PAREN,          // '('
    TOKEN_RIGHT_PAREN,         // ')'
    TOKEN_ASTERISK,            // '*'
    TOKEN_PLUS,                // '+'
    TOKEN_COMMA,               // ','
    TOKEN_MINUS,               // '-'
    TOKEN_PERIOD,              // '.'
    TOKEN_FORWARD_SLASH,       // '/'

    // ASCII [0-9]...

    TOKEN_COLON = 58,          // ':'
    TOKEN_SEMICOLON,           // ';'
    TOKEN_LESS_THAN,           // '<'
    TOKEN_EQUALS,              // '='
    TOKEN_GREATER_THAN,        // '>'
    TOKEN_QUESTION_MARK,       // '?'
    TOKEN_AT_SIGN,             // '@'

    // ASCII [A-Z]...

    TOKEN_LEFT_BRACKET = 91,   // '['
    TOKEN_BACK_SLASH,          // '\'
    TOKEN_RIGHT_BRACKET,       // ']'
    TOKEN_CARET,               // '^'
    TOKEN_UNDERSCORE,          // '_'
    TOKEN_GRAVE,               // '`'

    // ASCII [a-z]...

    TOKEN_LEFT_CURLY = 123,    // '{'
    TOKEN_VERTICAL_BAR,        // '|'
    TOKEN_RIGHT_CURLY,         // '}'
    TOKEN_TILDE,               // '~'

    // 2 char operators
    TOKEN_ARROW,               // '->'

    // Literals
    TOKEN_INTEGER_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_STRING_LITERAL,

    // Comments
    TOKEN_LINE_COMMENT,
    TOKEN_BLOCK_COMMENT,

    // EOF
    TOKEN_EOF
} token_type;


typedef struct location {
    /* The row the token text starts on */
    u64 row;
    /* The column the token text starts on */
    u64 column;
    /* The name of the file the token was lexed in */
    const char* filename;
} location;

typedef struct token {
    /* The type of the token */
    token_type type;

    /* The text of the token */
    const char* text;
    /* The length of the text */
    u64 length;

    /* The value that the token has, *if applicable* (i.e. a TOKEN_INTEGER_LITERAL will have its actual value populated here, as if by stoull)*/
    union {
        u64 unsigned64;
        i64 signed64;
        f64 float64;
    } value;

    /* The location the token was found in */
    location location;

    /* The type info of this node, (NOTE: this is only populated after the types are resolved using resolve_types() on an AST) */
    i32 typing_information; // TODO(Steven): I changed this to use i32 to try to break the cyclical return that is caused by making it a type_info
} token;

// WARN!: The ordering of the keywords needs to match that of the token_type enum!
extern const char* keywords[];
u64 keywords_array_length();

/**
 * @brief prints a token and all of its fields to stdout
 * 
 * @param t the token to print
 */
API void token_print(token t);

/**
 * @brief copies the token text to a null terminated buffer and returns it
 * 
 * @param t the token whose text should be copied
 * @param allocator the function pointer to a allocator which will provide a zero'ed out buffer (calloc is allowed)
 * @return const char* the buffer with a null terminated string for printing
 */
API char* token_printable_text(token t, void*(*allocator)(u64 count, u64 stride));

/**
 * @brief converts a location to a string to be printed
 * 
 * @param loc the location to be converted
 * @param allocator the function pointer to a allocator which will provide a zero'ed out buffer (calloc is allowed)
 * @return const char* the buffer with a null terminated string for printing
 */
API char* location_printable_text(location loc, void*(*allocator)(u64 count, u64 stride));

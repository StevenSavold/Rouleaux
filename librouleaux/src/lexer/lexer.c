#include "lexer/lexer.h"
#include "utilities/file_utilities.h"

#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief This is an internal function that performs the lexing of the next token without checking the peek_queue
 * 
 * @param lexer the lexer to be operated on
 * @return token the nex token this lexer found
 */
token lexer_next_token_internal(rouleaux_lexer* lexer);

location current_location(rouleaux_lexer* lexer);
b8 head_is_at_eof(rouleaux_lexer* lexer);
void skip_char(rouleaux_lexer* lexer, u64 n);
void trim_left(rouleaux_lexer* lexer);

b8 is_whitespace(char c);
b8 is_single_char_token(char c);
b8 is_identifier_character(char c, b8 include_numerics);
b8 is_numeric_character(char c);

b8 check_for_keyword(token* t);



rouleaux_lexer lexer_create(const char* filename)
{
    rouleaux_lexer lexer = {};
    lexer.filename = filename;
    lexer.current_row = 1;
    lexer.current_column = 1;

    u64 bytes_read = file_read(filename, &lexer.file_content_length, lexer.file_content);
    lexer.file_content = malloc(lexer.file_content_length);
    bytes_read = file_read(lexer.filename, &lexer.file_content_length, lexer.file_content);
    if (!bytes_read)
    {
        printf("lexer error: unable to read file '%s'", filename);
        free(lexer.file_content);
        lexer.file_content = NULL;
        lexer.has_error = true;
        return lexer;
    }

    // NOTE(Steven): Because it's a text file, the bytes_read could be smaller
    //               then the filesize because of CRLF translation
    lexer.file_content_length = bytes_read;
    lexer.file_end = lexer.file_content + lexer.file_content_length;

    lexer.head = lexer.file_content;

    // Initialize the peek_queue
    lexer.peek_buffer = peek_queue_create(32); //TODO(Steven): Reconsider the defaults... maybe we should expose this to the user?

    return lexer;
}

void lexer_destroy(rouleaux_lexer* lexer)
{
    if (lexer->file_content)
    {
        free(lexer->file_content);
        lexer->file_content = NULL;
    }

    peek_queue_destroy(&lexer->peek_buffer);

    memset(lexer, 0, sizeof(rouleaux_lexer));
}

b8 lexer_reset(rouleaux_lexer* lexer)
{
    lexer->head = lexer->file_content;
    lexer->current_column = 1;
    lexer->current_row = 1;
    lexer->has_error = false;

    peek_queue_empty(&lexer->peek_buffer);

    return true;
}

token lexer_next_token(rouleaux_lexer* lexer)
{
    // If the peek_buffer has tokens in it already we need to grab those first
    if (lexer->peek_buffer.size > 0)
    {
        token t;
        peek_queue_pop(&lexer->peek_buffer, &t);
        return t;
    }

    return lexer_next_token_internal(lexer);
}

token lexer_peek_token(rouleaux_lexer* lexer)
{
    token t;

    // If the peek_buffer has tokens in it already we need to grab those first
    if (lexer->peek_buffer.size > 0)
    {
        peek_queue_front(&lexer->peek_buffer, &t);
        return t;
    }

    // There were no cached tokens, so we need to go lex one
    t = lexer_next_token(lexer);

    // Push the token into the peek_queue
    peek_queue_push(&lexer->peek_buffer, t);

    return t;
}

b8 lexer_put_back_token(rouleaux_lexer* lexer, token t)
{
    return peek_queue_push_front(&lexer->peek_buffer, t);
}

token lexer_next_token_internal(rouleaux_lexer* lexer)
{
    // Get rid of the whitespace
    trim_left(lexer);

    token t = {};
    t.location = current_location(lexer);
    t.text = lexer->head;
    t.length = 0;
    
    if (head_is_at_eof(lexer))
    {
        t.type = TOKEN_EOF;
        return t;
    }

    // Identifiers & keywords
    if (is_identifier_character(*lexer->head, false))
    {
        do {
            skip_char(lexer, 1);
            t.length++;
        } while (is_identifier_character(*lexer->head, true));

        t.type = TOKEN_IDENTIFIER;
        check_for_keyword(&t); // If its a keyword token, change it to that token type
        return t; // return the identifier/keyword token
    }

    // Numbers that start with a numeric
    if (is_numeric_character(*lexer->head))
    {
        skip_char(lexer, 1);
        t.length++;
        t.type = TOKEN_INTEGER_LITERAL;
        while (is_numeric_character(*lexer->head))
        {
            skip_char(lexer, 1);
            t.length++;
        }

        // @BumpAllocator
        char* token_text = token_printable_text(t, calloc);
        char* ignored = NULL;
        t.value.unsigned64 = strtoull(token_text, &ignored, 10);
        free(token_text);

        if (*lexer->head == '.' && is_numeric_character(*(lexer->head + 1)))
        {
            // We found a decimal point followed by more numerics, this must be a float literal
            t.type = TOKEN_FLOAT_LITERAL;
            skip_char(lexer, 1);
            t.length++;
            while (is_numeric_character(*lexer->head))
            {
                skip_char(lexer, 1);
                t.length++;
            }

            // @BumpAllocator
            char* token_text = token_printable_text(t, calloc);
            char* ignored = NULL;
            t.value.float64 = strtold(token_text, &ignored);
            free(token_text);
        }
        return t; // return the 'number literal' token
    }

    // String Literals //
    if (*lexer->head == '"')
    {
        t.type = TOKEN_STRING_LITERAL;
        skip_char(lexer, 1);
        t.length++;
        
        while (!head_is_at_eof(lexer) && *lexer->head != '"')
        {
            skip_char(lexer, 1);
            t.length++;
        }

        if (head_is_at_eof(lexer))
        {
            // We could not finish parsing this token, so we flag the error and exit
            lexer->has_error = true;
            t.type = TOKEN_INVALID;
            return t;
        }

        skip_char(lexer, 1);
        t.length++;

        return t;
    }

    // Line Comments '//'
    if (*lexer->head == '/' && *(lexer->head + 1) == '/')
    {
        // This is a line comment and we can keep eating till the end of the line
        t.type = TOKEN_LINE_COMMENT;
        skip_char(lexer, 2);
        t.length += 2; // Skip the two slashes

        while (!head_is_at_eof(lexer) && *lexer->head != '\n')
        {
            skip_char(lexer, 1);
            t.length++;
        }

        return t;
    }

    // Block Comments '/**/'
    if (*lexer->head == '/' && *(lexer->head + 1) == '*')
    {
        // This is a block comment, keep eating characters until you see a
        // new opening block comment, or the close to this one
        t.type = TOKEN_BLOCK_COMMENT;
        skip_char(lexer, 2);
        t.length += 2;

        while (!head_is_at_eof(lexer) && !(*lexer->head == '*' && *(lexer->head + 1) == '/'))
        {
            skip_char(lexer, 1);
            t.length++;
        }

        if (head_is_at_eof(lexer))
        {
            // We could not finish parsing this token, so we flag the error and exit
            lexer->has_error = true;
            t.type = TOKEN_INVALID;
            return t;
        }
        // If we are not at the EOF we hit the closing symbol and can wrap up
        // This is the closing of the current block comment
        skip_char(lexer, 2);
        t.length += 2;

        return t;
    }


    // Punctuation & Single character operators
    if (is_single_char_token(*lexer->head))
    {
        t.type = *lexer->head; // The token_type will map directly from ASCII for single character tokens

        skip_char(lexer, 1);
        t.length++;

        // Don't return yet, we might want to augment the
        // token type based on the next character still
    }

    // TODO(Steven): Multi-Character operators?
    if (t.type == TOKEN_MINUS && *lexer->head == '>')
    {
        skip_char(lexer, 1);
        t.length++;
        t.type = TOKEN_ARROW;
    }
    
    return t;
}

location current_location(rouleaux_lexer* lexer)
{
    location loc = {};
    loc.row      = lexer->current_row;
    loc.column   = lexer->current_column;
    loc.filename = lexer->filename;

    return loc;
}

b8 head_is_at_eof(rouleaux_lexer* lexer)
{
    return lexer->head >= lexer->file_end;
}

void skip_char(rouleaux_lexer* lexer, u64 n)
{
    assert(!head_is_at_eof(lexer));
    for (u64 i = 0; i < n; ++i)
    {
        lexer->current_column++;
        if (*lexer->head == '\n')
        {
            lexer->current_row++;
            lexer->current_column = 1;
        }
        
        lexer->head++;
    }
}

void trim_left(rouleaux_lexer* lexer)
{
    while (!head_is_at_eof(lexer) && is_whitespace(*lexer->head))
    {
        skip_char(lexer, 1);
    }
}

b8 is_whitespace(char c)
{
    b8 is_space_or_tab = c == ' ' || c == '\t';
    b8 is_newline_or_carriage_return = c == '\n' || c == '\r';
    b8 is_vertical_tab_or_formfeed = c == '\v' || c == '\f';

    return is_space_or_tab || is_newline_or_carriage_return || is_vertical_tab_or_formfeed;
}

b8 is_single_char_token(char c)
{
    switch (c)
    {
        case TOKEN_EXCLAMATION:
        case TOKEN_DOUBLE_QUOTE:
        case TOKEN_POUND:
        case TOKEN_DOLLAR_SIGN:
        case TOKEN_PERCENT:
        case TOKEN_AMPERSAND:
        case TOKEN_SINGLE_QUOTE:
        case TOKEN_LEFT_PAREN:
        case TOKEN_RIGHT_PAREN:
        case TOKEN_ASTERISK:
        case TOKEN_PLUS:
        case TOKEN_COMMA:
        case TOKEN_MINUS:
        case TOKEN_PERIOD:
        case TOKEN_FORWARD_SLASH:
        case TOKEN_COLON:
        case TOKEN_SEMICOLON:
        case TOKEN_LESS_THAN:
        case TOKEN_EQUALS:
        case TOKEN_GREATER_THAN:
        case TOKEN_QUESTION_MARK:
        case TOKEN_AT_SIGN:
        case TOKEN_LEFT_BRACKET:
        case TOKEN_BACK_SLASH:
        case TOKEN_RIGHT_BRACKET:
        case TOKEN_CARET:
        case TOKEN_UNDERSCORE:
        case TOKEN_GRAVE:
        case TOKEN_LEFT_CURLY:
        case TOKEN_VERTICAL_BAR:
        case TOKEN_RIGHT_CURLY:
        case TOKEN_TILDE:
            return true;
    };

    return false;
}

b8 is_identifier_character(char c, b8 include_numerics)
{
    b8 is_uppercase = c >= 'A' && c <= 'Z';
    b8 is_lowercase = c >= 'a' && c <= 'z';
    b8 is_underscore = c == '_';
    b8 is_numeric = false;

    if (include_numerics)
        is_numeric = c >= '0' && c <= '9';

    return is_uppercase || is_lowercase || is_underscore || is_numeric;
}

b8 is_numeric_character(char c)
{
    b8 is_numeric = c >= '0' && c <= '9';

    return is_numeric;
}


b8 check_for_keyword(token* t)
{
    b8 matched = false;
    // NOTE: Starts at 1 to offset from the TOKEN_INVALID token_type, and goes till the end of the keywords array
    for (u64 i = 1; i < keywords_array_length(); ++i)
    {
        if (memcmp(t->text, keywords[i], t->length) == 0)
        {
            t->type = (token_type)i; // make the token the specific keyword type we matched with
            matched = true;
            break;
        }
    }
    return matched;
}
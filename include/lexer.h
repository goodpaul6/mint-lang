#pragma once

#define INIT_TOKEN_CAPACITY     128
#define MAX_LEXEME_LENGTH       1024
#define MAX_LEXER_ERROR_LENGTH  512

#include <stdbool.h>

typedef enum
{
    TOK_NULL,
    TOK_TRUE,
    TOK_FALSE,
    TOK_STRING,
    TOK_NUMBER,
    TOK_IDENT,

    TOK_VAR,
    TOK_FUNC,
    TOK_OPERATOR,
    TOK_IF,
    TOK_ELSE,
    TOK_RETURN,
    TOK_EXTERN,
    TOK_FOR,
    TOK_STRUCT,

    TOK_CONTINUE,
    TOK_BREAK,

    TOK_COMMA,
    TOK_COLON,
    TOK_SEMICOLON,

    TOK_OPENPAREN,
    TOK_CLOSEPAREN,
    TOK_OPENCURLY,
    TOK_CLOSECURLY,
    TOK_OPENSQUARE,
    TOK_CLOSESQUARE,

    TOK_EQUAL,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_MOD,
    TOK_XOR,
    TOK_BITOR,
    TOK_BITAND,
    TOK_LSHIFT,
    TOK_RSHIFT,

    TOK_LT,
    TOK_GT,
    TOK_LTE,
    TOK_GTE,
    TOK_AND,
    TOK_OR,
    TOK_EQUALS,
    TOK_NEQUALS,

    TOK_CAT,
    TOK_ELLIPSIS,

    NUM_TOKEN_TYPES
} TokenType;

typedef struct
{
    TokenType type;
    int line;

    union
    {
        char* ident;
        char* str;
        double num;
    };
} Token;

typedef struct _Tokenized
{
    // Number of tokens and the array
    int count;
    Token* tokens;

    // If this this is non-empty, then there was an error
    char error[MAX_LEXER_ERROR_LENGTH];

    // 'pos' is the last processed byte
    // 'line' is the last processed line
    int pos, line;
} Tokenized;

Tokenized Tokenize(const char* src);

const char* TokenTypeRepr(TokenType type);

// Gets printable representation of token (only valid until next call to TokenRepr)
// Asserts if it cannot get representation.
const char* TokenRepr(const Token* token);

// Frees all token data
void DestroyTokenized(Tokenized* tokenized);

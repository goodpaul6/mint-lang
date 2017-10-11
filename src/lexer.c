#include <assert.h>
#include <ctype.h>
#include <string.h>

#include "utils.h"
#include "lexer.h"

static char Lexeme[MAX_LEXEME_LENGTH];

static const char* TokenTypeNames[NUM_TOKEN_TYPES] = {
    "null",
    "true",
    "false",
    "string",
    "identifier",

    "var",
    "func",
    "operator",
    "if",
    "else",
    "return",
    "extern",
    "for",
    "struct",
    
    "continue",
    "break",

    ",",
    ":",
    ";",

    "(",
    ")",
    "{",
    "}",
    "[",
    "]",

    "=",
    "+",
    "-",
    "*",
    "/",
    "%",
    "^",
    "|",
    "&",
    "<<",
    ">>",
    
    "<",
    ">",
    "<=",
    ">=",
    "&&",
    "||",
    "==",
    "!=",

    "..",
    "..."
};

static const struct
{
    const char* name;
    TokenType tokenType;
} Keywords[] = {
    { "func", TOK_FUNC },
    { "operator", TOK_OPERATOR },
    { "return", TOK_RETURN },
    { "if", TOK_IF },
    { "else", TOK_ELSE },
    { "extern", TOK_EXTERN },
    { "for", TOK_FOR },
    { "continue", TOK_CONTINUE },
    { "break", TOK_BREAK },
    { "var", TOK_VAR },
    { "null", TOK_NULL },
    { "true", TOK_TRUE },
    { "false", TOK_FALSE },
    { "struct", TOK_STRUCT },
};

static const struct
{
    char seq[4];
    TokenType tokenType;
} Operators[] = {
    // These are ordered by number of characters because
    // we want to match the longest possible substring.
    { "...", TOK_ELLIPSIS },
    { "..", TOK_CAT },
    { "!=", TOK_NEQUALS },
    { "==", TOK_EQUALS },
    { "<=", TOK_LTE },
    { ">=", TOK_GTE },
    { "<<", TOK_LSHIFT },
    { ">>", TOK_RSHIFT },
    { "&&", TOK_AND },
    { "||", TOK_OR },

    { "<", TOK_LT },
    { ">", TOK_GT },
    { "+", TOK_PLUS },
    { "-", TOK_MINUS },
    { "*", TOK_MUL },
    { "/", TOK_DIV },
    { "%", TOK_MOD },
    { "^", TOK_XOR },
    { "|", TOK_BITOR },
    { "&", TOK_BITAND },

    { "(", TOK_OPENPAREN },
    { ")", TOK_CLOSEPAREN },
    { "{", TOK_OPENCURLY },
    { "}"' }, TOK_CLOSECURLY },
    { "[", TOK_OPENSQUARE },
    { "]", TOK_CLOSESQUARE },
    
    { ";", TOK_SEMICOLON },
    { ":", TOK_COLON },
    { ",", TOK_COMMA },
};

inline static bool TooLong(int i, char* error) {
    if(i >= MAX_LEXEME_LENGTH - 1) {
        strcpy(error, "Token exceeded maximum length of " #MAX_LEXEME_LENGTH " characters.");
        return false;
    }

    return true;
}

static bool NextToken(const char* src, int* pos, char* error, int* line, Token* token)
{
    while(isspace(src[*pos])) {
        if(src[*pos] == '\n') {
            *line += 1;
        }
        *pos += 1;
    }

    token->line = *line;

    if(isalpha(src[*pos]) || src[*pos] == '_') {
        int i = 0;
        while(isalnum(src[*pos]) || src[*pos] == '_') {
            if(TooLong(i, error)) {
                return false;
            }

            Lexeme[i++] = src[*pos];
            *pos += 1;
        }

        Lexeme[i] = '\0';

        for(int i = 0; i < sizeof(Keywords) / sizeof(Keywords[0]); ++i) {
            if(strcmp(Keywords[i].name, Lexeme) == 0) {
                token->type = Keywords[i].tokenType;
                return true;
            }
        }

        token->type = TOK_IDENT;
        token->ident = estrdup(Lexeme);
        
        return true;
    } 

    if(isdigit(src[*pos])) {
        int i = 0;
        bool hasRadix = false;

        while(isdigit(src[*pos]) || src[*pos] == '.') {
            if(src[*pos] == '.' && hasRadix) {
                strcpy(error, "Too many '.' in numeric literal.");
                return false;
            }

            if(TooLong(i, error)) {
                return false;
            }

            Lexeme[i++] = src[*pos];
            *pos += 1;
        }

        Lexeme[i] = '\0';

        token->type = TOK_NUMBER;
        token->num = strtod(Lexeme, NULL);
        
        return true;
    }

    if(src[*pos] == '"') {
        int i = 0;

        // Skip '"'
        *pos += 1;

        while(src[*pos] && src[*pos] != '"') {
            if(TooLong(i, error)) {
                return false;
            }

            char ch = src[*pos];

			if(ch == '\\')
			{
                *pos += 1;
				switch(src[*pos])
				{
					case 'n': ch  = '\n'; break;
					case 'r': ch = '\r'; break;
					case 't': ch = '\t'; break;
					case '"': ch = '"'; break;
					case '\'': ch = '\''; break;
					case '\\': ch = '\\'; break;
				}
			}

            Lexeme[i++] = ch;
            *pos += 1;
        }

        if(!src[*pos]) {
            strcpy(error, "Unexpected end of source inside string literal.");
            return false;
        }

        // Skip '"'
        *pos += 1;

        Lexeme[i] = '\0';

        token->type = TOK_STRING;
        token->str = estrdup(Lexeme);

        return true;
    }

    if(!src[*pos]) {
        // No error, just end of source
        return false;
    }
    
    // Try to match an "operator"
    for(int i = 0; i < sizeof(Operators) / sizeof(Operators[0]); ++i) {
        size_t len = strlen(Operators[i].seq);

        bool match = true;

        for(int j = 0; j < len; ++j) {
            if(src[*pos + j] && src[*pos + j] != Operators[i].seq[j]) {
                match = false;
                break;
            }
        }

        if(match) {
            token->type = Operators[i].tokenType;
            *pos += len;
            return true;
        }
    }
    
    sprintf(error, "Unexpected character '%c'.", src[*pos]);
    return false;
}

Tokenized Tokenize(const char* src)
{
    Tokenized res = { 0 };

    size_t capacity = INIT_TOKEN_CAPACITY;

    res.tokens = emalloc(sizeof(Token) * capacity);
    res.line = 1;

    while(true) {
        Token token;

        if(!NextToken(src, &res.pos, &res.line, res.error, &token)) {
            break;
        }

        // Grow the token buffer
        if(res.count + 1 >= capacity) {
            capacity *= 2;
            res.tokens = erealloc(res.tokens, sizeof(Token) * capacity);
        }

        // Copy token
        memcpy(&res.tokens[res.count], &token, sizeof(Token));
        res.count += 1;
    }

    return res;
}

const char* TokenTypeRepr(TokenType type)
{
    return TokenTypeNames[(int)type];
}

const char* TokenRepr(const Token* token)
{
    static char numBuf[64];

    switch(token->type) {
        case TOK_IDENT: return token->ident;
        case TOK_STRING: return token->str;
        case TOK_NUMBER: sprintf(numBuf, "%g", token->num); return numBuf;
        default: {
            for(int i = 0; i < sizeof(Keywords) / sizeof(Keywords[0]); ++i) {
                if(token->type == Keywords[i].tokenType) {
                    return Keywords[i].name;
                }
            }

            for(int i = 0; i < sizeof(Operators) / sizeof(Operators[0]); ++i) {
                if(token->type == Operators[i].tokenType) {
                    return Operators[i].seq;
                }
            }

            assert(false); // Invalid token type?
        } break;
    }
 
    assert(false);  // Unreachable code
    return NULL;
}

void DestroyTokenized(Tokenized* tokenized)
{
    for(int i = 0; i < tokenized->count; ++i) {
        Token* token = tokenized->tokens[i];

        switch(token->type) {
            case TOK_IDENT: free(token->ident); break;
            case TOK_STRING: free(token->str); break;
            default: break;
        }
    }

    free(tokenized->tokens);
}

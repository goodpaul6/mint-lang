#pragma once

#define MAX_PARSER_ERROR_LENGTH 2048

typedef struct _Tokenized Tokenized;

typedef struct
{
    // Global variables (VarDecl*), functions (FuncDecl*), and externs (FuncDecl*)
    List globals, functions, externs;

    // User-defined types (TypeHint*)
    List types;

    // Top level expressions (Expr*)
    List topLevel;

    // If this is non-empty, then an error occurred
    char error[MAX_PARSER_ERROR_LENGTH];
    
    // Last processed line (if an error occurs, its on this line)
    int line;
} Parsed;

// Parses all top level expressions
Parsed Parse(const Tokenized* tokenized);

// Deletes all symbols and expressions
void DestroyParsed(Parsed* parsed);

#pragma once

#include <stdbool.h>

#include "linkedlist.h"

typedef enum
{
    EXP_NULL,
    EXP_BOOL,
    EXP_NUMBER,
    EXP_STRING, 
    EXP_IDENT,          // name
    EXP_CALL,           // expr(expr{, expr}*);
    EXP_VAR,            // var ident : type;
    EXP_ASSIGN,         // var-or-ident = expr;
    EXP_UNARY,          // unary-op expr
    EXP_BIN,            // expr binary-op expr
    EXP_PAREN,          // ( expr )
    EXP_FOR,            // for expr '{' stmt* '}'
    EXP_FOR_IN,         // for name in expr '{' stmt* '}'
    EXP_IF,             // if expr '{' stmt* '}' {else stmt}
    EXP_EXTERN,         // extern ident(ident : type, ...) : type;
    EXP_FUNC,           // func ident(ident : type, ...) : type '{' stmt* '}'
    EXP_RETURN,         // return expr;
    EXP_ARRAY_LITERAL,  // '[' {expr,}* ']'
    EXP_DICT_LITERAL,   // '{' {assignment, }* '}'
    EXP_INDEX,          // expr '[' expr ']'
    EXP_DOT,            // expr '.' ident
    EXP_CONTINUE,       // 'continue'
    EXP_BREAK,          // 'break'
    EXP_STRUCT_DECL,    // struct '{' (ident : type;)* '}'
    EXP_CAST,           // 'cast' (type) expr

    // expr: ident | 

    // stmt: var | assignment | call | extern | if | 
    //       for | func | return | continue | break ';'

    NUM_EXPR_TYPES
} ExprType;

typedef struct _Expr
{
    ExprType type;

    int fileIndex;          // Index into string table for filename, -1 if none
    int line;               // Line number in code where this is located

    FuncDecl* curFunc;
    int varScope;

    // This is -1 until the expression is compiled
    int pc;

    union
    {
        bool boolean;
        int constIndex;     // Both EXP_NUMBER and EXP_STRING

        struct
        {
            VarDecl* decl;
            char* name;
        } varx;

        struct
        {
            struct _Expr* func;
            List args;
        } callx;
        
        struct
        {
            int op;
            struct _Expr* value;
        } unaryx;
        
        struct
        {
            struct _Expr* lhs;
            struct _Expr* rhs;
            int op;
        } binx;

        struct _Expr* parenValue;

        struct
        {
            struct _Expr* cond;
            List body;
        } forx;

        struct
        {
            VarDecl* iter;
            struct _Expr* range;
            List body;
        } forinx;

        struct
        {
            struct _Expr* cond;
            List body;
            struct _Expr* alt;
        } ifx;

        struct
        {
            FuncDecl* decl;
            List body;
        } funcx;

        struct _Expr* retVal;

        List arrayValues;
        List dictPairs;

        struct
        {
            struct _Expr* value;
            struct _Expr* index;
        } index;

        struct
        {
            struct _Expr* value;
            char* name;
        } dotx;

        struct
        {
            TypeHint* type;
            struct _Expr* value;
        } castx;
    };
} Expr;

Expr* CreateExpr(ExprType type, int fileIndex, int line, FuncDecl* curFunc, int varScope);

void DeleteExpr(Expr* exp);

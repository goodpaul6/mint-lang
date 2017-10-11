#pragma once

typedef struct _Typetag Typetag;
typedef struct _FuncDecl FuncDecl;

typedef struct
{
    // The func in which this was declared
    FuncDecl* func;

    // Var's index on the stack relative to the frame pointer
    int index;

    char* name;
    const Typetag* type;
} VarDecl;

typedef struct _FuncDecl
{
    struct _FuncDecl* outer;

    char* name;
    const Typetag* returnType;

    // args are all argument variables,
    // locals are all local variables
    List args, locals;
} FuncDecl;

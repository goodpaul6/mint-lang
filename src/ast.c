#include "utils.h"
#include "ast.h"

Expr* CreateExpr(ExprType type, int fileIndex, int line, FuncDecl* curFunc, int varScope)
{
    Expr* exp = emalloc(sizeof(Expr));

    exp->type = type;

    exp->fileIndex = fileIndex;
    exp->line = line;

    exp->curFunc = curFunc;
    exp->varScope = varScope;

    exp->pc = -1;

    return exp;
}

void DeleteExpr(Expr* exp)
{
    // TODO: Free the rest of the struct

    free(exp);
}

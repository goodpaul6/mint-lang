#include <string.h>

#include "typetag.h"

static const char* TagNames[NUM_PRIMITIVE_TAGS] = {
    "void",

    "int",
    "float",
    "string",
    "array",
    "dict",
    "func",
    "native",
    "any"
};

TypetagType TypetagTypeFromName(const char* name)
{
    for(int i = 0; i < NUM_PRIMITIVE_TAGS; ++i) {
        if(strcmp(TagNames[i], name) == 0) {
            return (TypetagType)i;
        }
    }

    return TAG_USER;
}

Typetag* CreateArrayTypetag(const Typetag* value)
{
    Typetag* tag = emalloc(sizeof(Typetag));

    tag->type = TAG_ARRAY;

    tag->valueType = value;

    return tag;
}

Typetag* CreateFuncTypetag(List args, const Typetag* returnType)
{
    Typetag* tag = emalloc(sizeof(Typetag));

    tag->type = TAG_FUNC;

    tag->func.args = args;
    tag->func.returnType = returnType;
    
    return tag;
}

Typetag* CreateUserTypetag(const char* name, bool defined)
{
    Typetag* tag = emalloc(sizeof(Typetag));

    tag->type = TAG_USER;

    tag->user.name = estrdup(name);
    InitList(&tag->user.fields);
    tag->user.defined = defined;

    return tag;
}

#pragma once

#include <stdbool.h>

typedef enum
{
    TAG_VOID,
    
    TAG_INT,
    TAG_FLOAT,
    TAG_STRING,
    TAG_ARRAY,
    TAG_DICT,
    TAG_FUNC,
    TAG_NATIVE,
    TAG_ANY,
    NUM_PRIMITIVE_TAGS,

    TAG_USER
} TypetagType;

typedef struct _Typetag
{
    TypetagType type;

    union
    {
        const struct _Typetag* valueType;

        struct
        {
            List args;
            const struct _Typetag* returnType;
        } func;

        struct
        {
            char* name;
            List fields;
            bool defined;
        } user; 
    };
} Typetag;

// Defaults to "USER" if the name isn't one of the primitive ones
TypetagType TypetagTypeFromName(const char* name);

Typetag* CreateArrayTypetag(const Typetag* value);
Typetag* CreateFuncTypetag(List args, const Typetag* returnType);
Typetag* CreateUserTypetag(const char* name, bool defined);

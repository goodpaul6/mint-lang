#include "SFML/System.h"
#include "SFML/Window.h"
#include "SFML/Graphics.h"
#include "vm.h"

#include <stdlib.h>
#include <string.h>

static void* emalloc(size_t size)
{
	void* mem = malloc(size);
	if(!mem) { fprintf(stderr, "Virtual machine ran out of memory!\n"); exit(1); }
	return mem;
}

static void* ecalloc(size_t size, size_t nmemb)
{
	void* mem = calloc(size, nmemb);
	if(!mem) { fprintf(stderr, "Virtual machine ran out of memory!\n"); exit(1); }
	return mem;
}

static void* erealloc(void* mem, size_t newSize)
{
	void* newMem = realloc(mem, newSize);
	if(!newMem) { fprintf(stderr, "Virtual machine ran out of memory!\n"); exit(1); }
	return newMem;
}

static char* estrdup(const char* string)
{
	char* newString = emalloc(strlen(string) + 1);
	strcpy(newString, string);
	return newString;
}

sfVector2f PopVector2f(VM* vm)
{
	Object* obj = PopArrayObject(vm);
	if(obj->array.length < 2)
	{
		fprintf(stderr, "Expected array of length 2 (or greater) in order to create sfVector2f\n");
		exit(1);
	}
	
	sfVector2f point = { obj->array.members[0]->number, obj->array.members[1]->number };
	return point;
}

sfColor PopColor(VM* vm)
{
	Object* obj = PopArrayObject(vm);
	if(obj->array.length < 4)
	{
		fprintf(stderr, "Expected array of length 4 (or greater) in order to create sfColor\n");
		exit(1);
	}
	
	sfColor color = { obj->array.members[0]->number, obj->array.members[1]->number, obj->array.members[2]->number, obj->array.members[3]->number };
	return color;
}

sfFloatRect PopFloatRect(VM* vm)
{	
	Object* obj = PopArrayObject(vm);
	if(obj->array.length < 4)
	{
		fprintf(stderr, "Expected array of length 4 (or greater) in order to create sfFloatRect\n");
		exit(1);
	}
	
	sfFloatRect rect = { obj->array.members[0]->number, obj->array.members[1]->number, obj->array.members[2]->number, obj->array.members[3]->number };
	return rect;
}

void Ext_sfTransform_create(VM* vm)
{
	sfTransform* transform = emalloc(sizeof(sfTransform));
	*transform = sfTransform_Identity;
	PushNative(vm, transform, free, NULL);
}

void Ext_sfTransform_identity(VM* vm)
{
	sfTransform* transform = PopNative(vm);
	*transform = sfTransform_Identity;
}

void Ext_sfTransform_transformPoint(VM* vm)
{
	const sfTransform* transform = PopNative(vm);
	sfVector2f p = PopVector2f(vm);
	
	p = sfTransform_transformPoint(transform, p);
	
	Object* obj = PushArray(vm, 2);
	
	PushNumber(vm, p.x);
	obj->array.members[0] = PopObject(vm);
	PushNumber(vm, p.y);
	obj->array.members[1] = PopObject(vm);
}

void Ext_sfTransform_transformRect(VM* vm)
{
	const sfTransform* transform = PopNative(vm);
	sfFloatRect rect = PopFloatRect(vm);
	
	rect = sfTransform_transformRect(transform, rect);
	
	Object* obj = PushArray(vm, 4);
	
	PushNumber(vm, rect.left);
	obj->array.members[0] = PopObject(vm);
	PushNumber(vm, rect.top);
	obj->array.members[1] = PopObject(vm);
	PushNumber(vm, rect.width);
	obj->array.members[2] = PopObject(vm);
	PushNumber(vm, rect.height);
	obj->array.members[3] = PopObject(vm);
}

void Ext_sfTransform_combine(VM* vm)
{
	sfTransform* transform = PopNative(vm);
	const sfTransform* otherTransform = PopNative(vm);
	
	sfTransform_combine(transform, otherTransform);
}

void Ext_sfTransform_translate(VM* vm)
{
	sfTransform* transform = PopNative(vm);
	sfVector2f amount = PopVector2f(vm);
	
	sfTransform_translate(transform, amount.x, amount.y);
}

void Ext_sfTransform_rotate(VM* vm)
{
	sfTransform* transform = PopNative(vm);
	float angle = PopNumber(vm);
	
	sfTransform_rotate(transform, angle); 
}

void Ext_sfTransform_rotateWithCenter(VM* vm)
{
	sfTransform* transform = PopNative(vm);
	float angle = PopNumber(vm);
	sfVector2f center = PopVector2f(vm);
	
	sfTransform_rotateWithCenter(transform, angle, center.x, center.y);
}

void Ext_sfTransform_scale(VM* vm)
{
	sfTransform* transform = PopNative(vm);
	sfVector2f factor = PopVector2f(vm);
	
	sfTransform_scale(transform, factor.x, factor.y);
}

void Ext_sfTransform_scaleWithCenter(VM* vm)
{
	sfTransform* transform = PopNative(vm);
	sfVector2f factor = PopVector2f(vm);
	sfVector2f center = PopVector2f(vm);
	
	sfTransform_scaleWithCenter(transform, factor.x, factor.y, center.x, center.y);
}

sfBlendMode Ext_sfBlendAlpha;
sfBlendMode Ext_sfBlendAdd;
sfBlendMode Ext_sfBlendMultiply;
sfBlendMode Ext_sfBlendNone;

void Ext_InitBlendModes()
{
	Ext_sfBlendAlpha = sfBlendAlpha;
	Ext_sfBlendAdd = sfBlendAdd;
	Ext_sfBlendMultiply = sfBlendMultiply;
	Ext_sfBlendNone = sfBlendNone;
}

void Ext_sfBlendMode_getBlendAlpha(VM* vm)
{
	PushNative(vm, &Ext_sfBlendAlpha, NULL, NULL);
}

void Ext_sfBlendMode_getBlendAdd(VM* vm)
{
	PushNative(vm, &Ext_sfBlendAdd, NULL, NULL);
}

void Ext_sfBlendMode_getBlendMultiply(VM* vm)
{
	PushNative(vm, &Ext_sfBlendMultiply, NULL, NULL);
}

void Ext_sfBlendMode_getBlendNone(VM* vm)
{
	PushNative(vm, &Ext_sfBlendNone, NULL, NULL);
}

void Ext_sfShader_destroy(void* shader)
{
	sfShader_destroy(shader);
}

void Ext_sfShader_createFromFile(VM* vm)
{
	const char* vertexShaderFilename = PopString(vm);
	const char* fragmentShaderFilename = PopString(vm);
	
	sfShader* shader = sfShader_createFromFile(vertexShaderFilename, fragmentShaderFilename);
	PushNative(vm, shader, Ext_sfShader_destroy, NULL);
}

void Ext_sfShader_createFromMemory(VM* vm)
{
	const char* vertexShader = PopString(vm);
	const char* fragmentShader = PopString(vm);
	
	sfShader* shader = sfShader_createFromMemory(vertexShader, fragmentShader);
	PushNative(vm, shader, Ext_sfShader_destroy, NULL);
}

void Ext_sfShader_setFloatParameter(VM* vm)
{
	sfShader* shader = PopNative(vm);
	const char* name = PopString(vm);
	float x = PopNumber(vm);
	
	sfShader_setFloatParameter(shader, name, x);
}

void Ext_sfShader_setFloat2Parameter(VM* vm)
{
	sfShader* shader = PopNative(vm);
	const char* name = PopString(vm);
	float x = PopNumber(vm);
	float y = PopNumber(vm);
	
	sfShader_setFloat2Parameter(shader, name, x, y);
}

void Ext_sfShader_setFloat3Parameter(VM* vm)
{
	sfShader* shader = PopNative(vm);
	const char* name = PopString(vm);
	float x = PopNumber(vm);
	float y = PopNumber(vm);
	float z = PopNumber(vm);
	
	sfShader_setFloat3Parameter(shader, name, x, y, z);
}

void Ext_sfShader_setFloat4Parameter(VM* vm)
{
	sfShader* shader = PopNative(vm);
	const char* name = PopString(vm);
	float x = PopNumber(vm);
	float y = PopNumber(vm);
	float z = PopNumber(vm);
	float w = PopNumber(vm);
	
	sfShader_setFloat4Parameter(shader, name, x, y, z, w);
}

void Ext_sfShader_setVector2Parameter(VM* vm)
{
	sfShader* shader = PopNative(vm);
	const char* name = PopString(vm);
	sfVector2f vector = PopVector2f(vm);
	
	sfShader_setVector2Parameter(shader, name, vector);
}

void Ext_sfShader_setColorParameter(VM* vm)
{
	sfShader* shader = PopNative(vm);
	const char* name = PopString(vm);
	sfColor color = PopColor(vm);
	
	sfShader_setColorParameter(shader, name, color);
}

void Ext_sfShader_setTransformParameter(VM* vm)
{
	sfShader* shader = PopNative(vm);
	const char* name = PopString(vm);
	sfTransform* transform = PopNative(vm);
	
	sfShader_setTransformParameter(shader, name, *transform);
}

void Ext_sfShader_setTextureParameter(VM* vm)
{
	sfShader* shader = PopNative(vm);
	const char* name = PopString(vm);
	sfTexture* texture = PopNative(vm);
	
	sfShader_setTextureParameter(shader, name, texture);
}

void Ext_sfShader_setCurrentTextureParameter(VM* vm)
{
	sfShader* shader = PopNative(vm);
	const char* name = PopString(vm);
	
	sfShader_setCurrentTextureParameter(shader, name);
}

void Ext_sfShader_isAvailable(VM* vm)
{
	PushNumber(vm, sfShader_isAvailable());
}

void Ext_sfRenderStates_create(VM* vm)
{
	sfRenderStates* states = emalloc(sizeof(sfRenderStates));
	memset(states, 0, sizeof(sfRenderStates));
	
	PushNative(vm, states, free, NULL);
}

void Ext_sfRenderStates_copy(VM* vm)
{
	sfRenderStates* states = PopNative(vm);
	sfRenderStates* copiedStates = emalloc(sizeof(sfRenderStates));
	memcpy(copiedStates, states, sizeof(sfRenderStates));
	
	PushNative(vm, copiedStates, free, NULL);
}

void Ext_sfRenderStates_getBlendMode(VM* vm)
{
	sfRenderStates* states = PopNative(vm);
}

void Ext_sfRenderStates_setBlendMode(VM* vm)
{
	sfRenderStates* states = PopNative(vm);
	sfBlendMode* blendMode = PopNative(vm);
	states->blendMode = *blendMode;
}

void Ext_sfRenderStates_setTransform(VM* vm)
{
	sfRenderStates* states = PopNative(vm);
	sfTransform* transform = PopNative(vm);
	states->transform = *transform;
}

void Ext_sfRenderStates_setTexture(VM* vm)
{
	sfRenderStates* states = PopNative(vm);
	states->texture = PopNativeOrNull(vm);
}

void Ext_sfRenderStates_setShader(VM* vm)
{
	sfRenderStates* states = PopNative(vm);
	states->shader = PopNativeOrNull(vm);
}

void Ext_sfRenderWindow_destroy(void* wp)
{
	sfRenderWindow* window = wp;
	sfRenderWindow_destroy(window);
}

void Ext_sfRenderWindow_create(VM* vm)
{
	sfVideoMode mode = { (int)PopNumber(vm), (int)PopNumber(vm), (int)PopNumber(vm) };
	sfRenderWindow* window = sfRenderWindow_create(mode, PopString(vm), sfResize | sfClose, NULL);

	PushNative(vm, window, Ext_sfRenderWindow_destroy, NULL);
}

void Ext_sfRenderWindow_setFramerateLimit(VM* vm)
{
	sfRenderWindow* window = PopNative(vm);
	int limit = PopNumber(vm);
	
	sfRenderWindow_setFramerateLimit(window, limit);
}

void Ext_sfRenderWindow_isOpen(VM* vm)
{
	sfRenderWindow* window = PopNative(vm);
	PushNumber(vm, sfRenderWindow_isOpen(window));
}

void Ext_sfRenderWindow_close(VM* vm)
{
	sfRenderWindow* window = PopNative(vm);
	sfRenderWindow_close(window);
}

void Ext_sfRenderWindow_pollEvent(VM* vm)
{
	sfRenderWindow* window = PopNative(vm);
	sfEvent* event = PopNative(vm);
	
	PushNumber(vm, sfRenderWindow_pollEvent(window, event));
}

void Ext_sfRenderWindow_clear(VM* vm)
{
	sfRenderWindow* window = PopNative(vm);
	sfRenderWindow_clear(window, sfBlack);
}

void Ext_sfRenderWindow_drawSprite(VM* vm)
{
	sfRenderWindow* window = PopNative(vm);
	sfSprite* sprite = PopNative(vm);
	sfRenderStates* states = PopNativeOrNull(vm);
	sfRenderWindow_drawSprite(window, sprite, states);
}

void Ext_sfRenderWindow_drawVertexArray(VM* vm)
{
	sfRenderWindow* window = PopNative(vm);
	sfVertexArray* vertexArray = PopNative(vm);
	sfRenderStates* states = PopNativeOrNull(vm);
	sfRenderWindow_drawVertexArray(window, vertexArray, states);
}

void Ext_sfRenderWindow_display(VM* vm)
{
	sfRenderWindow* window = PopNative(vm);
	sfRenderWindow_display(window);
}

void Ext_sfEvent_create(VM* vm)
{
	sfEvent* event = emalloc(sizeof(sfEvent));
	PushNative(vm, event, free, NULL);
}

void Ext_sfEvent_type(VM* vm)
{
	sfEvent* event = PopNative(vm);
	PushNumber(vm, event->type);
}

void Ext_sfEvent_size_width(VM* vm)
{
	sfEvent* event = PopNative(vm);
	PushNumber(vm, event->size.width);
}

void Ext_sfEvent_size_height(VM* vm)
{
	sfEvent* event = PopNative(vm);
	PushNumber(vm, event->size.height);
}

void Ext_sfEvent_key_code(VM* vm)
{
	sfEvent* event = PopNative(vm);
	PushNumber(vm, event->key.code);
}

void Ext_sfTexture_destroy(void* pt)
{
	sfTexture* texture = pt;
	sfTexture_destroy(texture);
}

void Ext_sfTexture_createFromFile(VM* vm)
{
	sfTexture* texture = sfTexture_createFromFile(PopString(vm), NULL);
	if(!texture)
	{
		fprintf(stderr, "Unable to load texture\n");
		exit(1);
	}
	
	PushNative(vm, texture, Ext_sfTexture_destroy, NULL);
}

void Ext_sfTexture_getSize(VM* vm)
{
	sfTexture* texture = PopNative(vm);
	sfVector2u size = sfTexture_getSize(texture);
	
	Object* obj = PushArray(vm, 2);
	PushNumber(vm, size.x);
	obj->array.members[0] = PopObject(vm);
	PushNumber(vm, size.y);
	obj->array.members[1] = PopObject(vm);
}

void Ext_sfSprite_destroy(void* sp)
{
	sfSprite* sprite = sp;
	sfSprite_destroy(sprite);
}

void Ext_sfSprite_create(VM* vm)
{
	sfSprite* sprite = sfSprite_create();
	PushNative(vm, sprite, Ext_sfSprite_destroy, NULL);
}

void Ext_sfSprite_setTexture(VM* vm)
{
	sfSprite* sprite = PopNative(vm);
	sfTexture* texture = PopNative(vm);
	
	sfSprite_setTexture(sprite, texture, sfTrue);
}

void Ext_sfSprite_setPosition(VM* vm)
{
	sfSprite* sprite = PopNative(vm);
	sfVector2f pos = PopVector2f(vm);
	
	sfSprite_setPosition(sprite, pos);
}

void Ext_sfSprite_move(VM* vm)
{
	sfSprite* sprite = PopNative(vm);	
	sfVector2f vel = PopVector2f(vm);
	
	sfSprite_move(sprite, vel);
}

void Ext_sfVertex_create(VM* vm)
{
	sfVertex* vertex = emalloc(sizeof(sfVertex));
	PushNative(vm, vertex, free, NULL);
}

void Ext_sfVertex_setPosition(VM* vm)
{
	sfVertex* vertex = PopNative(vm);
	sfVector2f position = PopVector2f(vm);
	
	vertex->position = position;
}

void Ext_sfVertex_setColor(VM* vm)
{
	sfVertex* vertex = PopNative(vm);
	sfColor color = PopColor(vm);
	
	vertex->color = color;
}

void Ext_sfVertex_setTexCoords(VM* vm)
{
	sfVertex* vertex = PopNative(vm);
	sfVector2f texCoords = PopVector2f(vm);
	
	vertex->texCoords = texCoords;
}

void Ext_sfVertexArray_destroy(void* vertexArray)
{
	sfVertexArray_destroy(vertexArray);
}

void Ext_sfVertexArray_create(VM* vm)
{
	sfVertexArray* vertexArray = sfVertexArray_create();
	PushNative(vm, vertexArray, Ext_sfVertexArray_destroy, NULL);
}

void Ext_sfVertexArray_copy(VM* vm)
{
	sfVertexArray* vertexArray = PopNative(vm);
	sfVertexArray* copiedVertexArray = sfVertexArray_copy(vertexArray);
	PushNative(vm, copiedVertexArray, Ext_sfVertexArray_destroy, NULL);
}

void Ext_sfVertexArray_getVertexCount(VM* vm)
{
	sfVertexArray* vertexArray = PopNative(vm);
	PushNumber(vm, sfVertexArray_getVertexCount(vertexArray));
}

void Ext_sfVertexArray_clear(VM* vm)
{
	sfVertexArray* vertexArray = PopNative(vm);
	sfVertexArray_clear(vertexArray);
}

void Ext_sfVertexArray_resize(VM* vm)
{
	sfVertexArray* vertexArray = PopNative(vm);
	size_t vertexCount = (size_t)PopNumber(vm);
	
	sfVertexArray_resize(vertexArray, vertexCount);
}

void Ext_sfVertexArray_append(VM* vm)
{
	sfVertexArray* vertexArray = PopNative(vm);
	sfVertex* vertex = PopNative(vm);
	
	sfVertexArray_append(vertexArray, *vertex);
}

void Ext_sfVertexArray_setPrimitiveType(VM* vm)
{
	sfVertexArray* vertexArray = PopNative(vm);
	int type = PopNumber(vm);
	
	sfVertexArray_setPrimitiveType(vertexArray, type);
}

void HookSFML(VM* vm)
{
	HookExternNoWarn(vm, "sfTransform_create", Ext_sfTransform_create);
	HookExternNoWarn(vm, "sfTransform_identity", Ext_sfTransform_identity);
	HookExternNoWarn(vm, "sfTransform_transformPoint", Ext_sfTransform_transformPoint);
	HookExternNoWarn(vm, "sfTransform_transformRect", Ext_sfTransform_transformRect);
	HookExternNoWarn(vm, "sfTransform_combine", Ext_sfTransform_combine);
	HookExternNoWarn(vm, "sfTransform_translate", Ext_sfTransform_translate);
	HookExternNoWarn(vm, "sfTransform_rotate", Ext_sfTransform_rotate);
	HookExternNoWarn(vm, "sfTransform_rotateWithCenter", Ext_sfTransform_rotateWithCenter);
	HookExternNoWarn(vm, "sfTransform_scale", Ext_sfTransform_scale);
	HookExternNoWarn(vm, "sfTransform_scaleWithCenter", Ext_sfTransform_scaleWithCenter);
	
	Ext_InitBlendModes();
	
	HookExternNoWarn(vm, "sfBlendMode_getBlendAlpha", Ext_sfBlendMode_getBlendAlpha);
	HookExternNoWarn(vm, "sfBlendMode_getBlendAdd", Ext_sfBlendMode_getBlendAdd);
	HookExternNoWarn(vm, "sfBlendMode_getBlendMultiply", Ext_sfBlendMode_getBlendMultiply);
	HookExternNoWarn(vm, "sfBlendMode_getBlendNone", Ext_sfBlendMode_getBlendNone);
	
	HookExternNoWarn(vm, "sfShader_createFromFile", Ext_sfShader_createFromFile);
	HookExternNoWarn(vm, "sfShader_createFromMemory", Ext_sfShader_createFromMemory);
	HookExternNoWarn(vm, "sfShader_setFloatParameter", Ext_sfShader_setFloatParameter);
	HookExternNoWarn(vm, "sfShader_setFloat2Parameter", Ext_sfShader_setFloat2Parameter);
	HookExternNoWarn(vm, "sfShader_setFloat3Parameter", Ext_sfShader_setFloat3Parameter);
	HookExternNoWarn(vm, "sfShader_setFloat4Parameter", Ext_sfShader_setFloat4Parameter);
	HookExternNoWarn(vm, "sfShader_setVector2Parameter", Ext_sfShader_setVector2Parameter);
	HookExternNoWarn(vm, "sfShader_setColorParameter", Ext_sfShader_setColorParameter);
	HookExternNoWarn(vm, "sfShader_setTransformParameter", Ext_sfShader_setTransformParameter);
	HookExternNoWarn(vm, "sfShader_setTextureParameter", Ext_sfShader_setTextureParameter);
	HookExternNoWarn(vm, "sfShader_setCurrentTextureParameter", Ext_sfShader_setCurrentTextureParameter);
	HookExternNoWarn(vm, "sfShader_isAvailable", Ext_sfShader_isAvailable);
	
	HookExternNoWarn(vm, "sfRenderStates_create", Ext_sfRenderStates_create);
	HookExternNoWarn(vm, "sfRenderStates_copy", Ext_sfRenderStates_copy);
	HookExternNoWarn(vm, "sfRenderStates_setBlendMode", Ext_sfRenderStates_setBlendMode);
	HookExternNoWarn(vm, "sfRenderStates_setTransform", Ext_sfRenderStates_setTransform);
	HookExternNoWarn(vm, "sfRenderStates_setTexture", Ext_sfRenderStates_setTexture);
	HookExternNoWarn(vm, "sfRenderStates_setShader", Ext_sfRenderStates_setShader);
	
	HookExternNoWarn(vm, "sfRenderWindow_create", Ext_sfRenderWindow_create);
	HookExternNoWarn(vm, "sfRenderWindow_setFramerateLimit", Ext_sfRenderWindow_setFramerateLimit);
	HookExternNoWarn(vm, "sfRenderWindow_isOpen", Ext_sfRenderWindow_isOpen);
	HookExternNoWarn(vm, "sfRenderWindow_pollEvent", Ext_sfRenderWindow_pollEvent);
	HookExternNoWarn(vm, "sfRenderWindow_close", Ext_sfRenderWindow_close);
	HookExternNoWarn(vm, "sfRenderWindow_clear", Ext_sfRenderWindow_clear);
	HookExternNoWarn(vm, "sfRenderWindow_display", Ext_sfRenderWindow_display);
	HookExternNoWarn(vm, "sfRenderWindow_drawSprite", Ext_sfRenderWindow_drawSprite);
	HookExternNoWarn(vm, "sfRenderWindow_drawVertexArray", Ext_sfRenderWindow_drawVertexArray);
	
	HookExternNoWarn(vm, "sfEvent_create", Ext_sfEvent_create);
	HookExternNoWarn(vm, "sfEvent_type", Ext_sfEvent_type);
	HookExternNoWarn(vm, "sfEvent_size_width", Ext_sfEvent_size_width);
	HookExternNoWarn(vm, "sfEvent_size_height", Ext_sfEvent_size_height);
	HookExternNoWarn(vm, "sfEvent_key_code", Ext_sfEvent_key_code);
	
	HookExternNoWarn(vm, "sfTexture_createFromFile", Ext_sfTexture_createFromFile);
	HookExternNoWarn(vm, "sfTexture_getSize", Ext_sfTexture_getSize);
	
	HookExternNoWarn(vm, "sfSprite_create", Ext_sfSprite_create);
	HookExternNoWarn(vm, "sfSprite_setTexture", Ext_sfSprite_setTexture);
	HookExternNoWarn(vm, "sfSprite_setPosition", Ext_sfSprite_setPosition);
	HookExternNoWarn(vm, "sfSprite_move", Ext_sfSprite_move);
	
	HookExternNoWarn(vm, "sfVertex_create", Ext_sfVertex_create);
	HookExternNoWarn(vm, "sfVertex_setPosition", Ext_sfVertex_setPosition);
	HookExternNoWarn(vm, "sfVertex_setColor", Ext_sfVertex_setColor);
	HookExternNoWarn(vm, "sfVertex_setTexCoords", Ext_sfVertex_setTexCoords);
	
	HookExternNoWarn(vm, "sfVertexArray_create", Ext_sfVertexArray_create);
	HookExternNoWarn(vm, "sfVertexArray_copy", Ext_sfVertexArray_copy);
	HookExternNoWarn(vm, "sfVertexArray_getVertexCount", Ext_sfVertexArray_getVertexCount);
	HookExternNoWarn(vm, "sfVertexArray_clear", Ext_sfVertexArray_clear);
	HookExternNoWarn(vm, "sfVertexArray_resize", Ext_sfVertexArray_resize);
	HookExternNoWarn(vm, "sfVertexArray_append", Ext_sfVertexArray_append);
	HookExternNoWarn(vm, "sfVertexArray_setPrimitiveType", Ext_sfVertexArray_setPrimitiveType);
}

int main(int argc, char* argv[])
{
	if(argc >= 2)
	{
		FILE* bin = fopen(argv[1], "rb");
		if(!bin)
		{
			fprintf(stderr, "Failed to open file '%s' for execution\n", argv[1]);
			return 1;
		}

		char debugFlag = 0;
		for(int i = 2; i < argc; ++i)
		{
			if(strcmp(argv[i], "-d") == 0)
				debugFlag = 1;
		}
		VM* vm = NewVM();

		vm->debug = debugFlag;
		
		LoadBinaryFile(vm, bin);
		fclose(bin);
		
		HookStandardLibrary(vm);
		HookSFML(vm);
		
		RunVM(vm);
		
		DeleteVM(vm);
	}
	else
	{
		fprintf(stderr, "Expected at least some command line arguments!\n");
		exit(1);
	}
	
	return 0;
}

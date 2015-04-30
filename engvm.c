#include "vm.h"
#include "SDL2/SDL.h"

#include <stdlib.h>
#include <string.h>

#define MAX_ENTITIES 32

typedef struct
{
	float x, y;
	float w, h;
} Entity;

Entity Entities[MAX_ENTITIES];
int NumEntities = 0;

void AddEntity(VM* vm)
{
	int id = NumEntities++;
	
	float w = PopNumber(vm);
	float h = PopNumber(vm);
	
	Entities[id].alive = 1;
	Entities[id].w = w;
	Entities[id].h = h;
	Entities[id].x = Entities[id].y = 0;
	
	PushNumber(vm, id);
}

void SetEntityPos(VM* vm)
{
	int id = (int)PopNumber(vm);
	float x = PopNumber(vm);
	float y = PopNumber(vm);
	
	Entities[id].x += x;
	Entities[id].y += y;
}

int main(int argc, char* argv[])
{
	FILE* in = fopen("game.mb", "rb");
}



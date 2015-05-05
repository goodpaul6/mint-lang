#include "vm.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <stdlib.h>

#define MAX_ENTITIES 1024

typedef struct
{
	char alive;
	float x, y;
	float radius;
} Entity;

Entity Entities[MAX_ENTITIES];
int NumEntities = 0;

void Ext_AddEnt(VM* vm)
{
	Entity* ent = &Entities[NumEntities++];
	ent->alive = 1;
	ent->x = 0;
	ent->y = 0;
	ent->radius = PopNumber(vm);
	PushNumber(vm, NumEntities - 1);
}

void Ext_Kill(VM* vm)
{
	int id = (int)PopNumber(vm);
	Entities[id].alive = 0;
}

void Ext_Move(VM* vm)
{
	int id = (int)PopNumber(vm);
	
	float x = (int)PopNumber(vm);
	float y = (int)PopNumber(vm);
	
	Entities[id].x += x;
	Entities[id].y += y;
}

void Ext_KeyDown(VM* vm)
{
	int id = (int)PopNumber(vm);
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	PushNumber(vm, keys[id]);
}

void DrawAll(SDL_Renderer* ren)
{
	SDL_SetRenderDrawColor(ren, 255, 0, 0, 255);
	for(int i = 0; i < NumEntities; ++i)
	{
		Entity* ent = &Entities[i];
		SDL_Rect dst = {ent->x - ent->radius, ent->y - ent->radius, ent->radius * 2, ent->radius * 2};
		SDL_RenderFillRect(ren, &dst);
	}
}

int main(int argc, char* argv[])
{
	VM* vm = NewVM();
	
	FILE* in = fopen("out.mb", "rb");
	if(!in)
	{
		fprintf(stderr, "engine failed to open 'out.mb'\n");
		exit(1);
	}
	
	LoadBinaryFile(vm, in);
	fclose(in);
	
	HookStandardLibrary(vm);
	
	HookExternNoWarn(vm, "add", Ext_AddEnt);
	HookExternNoWarn(vm, "kill", Ext_Kill);
	HookExternNoWarn(vm, "move", Ext_Move);
	HookExternNoWarn(vm, "keydown", Ext_KeyDown);
	
	SDL_Init(SDL_INIT_EVERYTHING);
	
	SDL_Window* window = SDL_CreateWindow("Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
	SDL_Renderer* ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_Event event;
	
	int initId = GetFunctionId(vm, "init");
	int updateId = GetFunctionId(vm, "update");
	
	if(initId >= 0)
		CallFunction(vm, initId, 0);
	
	char running = 1;
	
	int last = SDL_GetTicks();
	while(running)
	{
		while (SDL_PollEvent(&event))
		{
			if(event.type == SDL_QUIT)
				running = 0;
		}
		
		float dt = (SDL_GetTicks() - last) / 1000.0f;
		last = SDL_GetTicks();
		
		if(updateId >= 0)
		{
			PushNumber(vm, dt);
			CallFunction(vm, updateId, 1);
		}
		for(int i = 0; i < NumEntities; ++i)
		{
			if(!Entities[i].alive)
			{
				memmove(&Entities[i], &Entities[i + 1], sizeof(Entity) * (NumEntities - i - 1));
				--NumEntities;
			}
		}
		
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
		SDL_RenderClear(ren);
		DrawAll(ren);
		SDL_RenderPresent(ren);
	
		SDL_Delay(12);
	}
	
	SDL_Quit();
	DeleteVM(vm);
	return 0;
}

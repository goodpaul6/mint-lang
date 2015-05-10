#include "vm.h"
#include <SDL2/SDL.h>
#include <string.h>
#include <stdlib.h>

SDL_Window* window = NULL;
SDL_Renderer* ren = NULL;
SDL_Event event;

void Ext_SetColor(VM* vm)
{
	Object* color = PopArrayObject(vm);
	int r = (int)color->array.members[0]->number;
	int g = (int)color->array.members[1]->number;
	int b = (int)color->array.members[2]->number;
	int a = (int)color->array.members[3]->number;

	SDL_SetRenderDrawColor(ren, r, g, b, a);
}

void Ext_FillRect(VM* vm)
{
	SDL_Rect dst = { (int)PopNumber(vm), (int)PopNumber(vm), (int)PopNumber(vm), (int)PopNumber(vm) };
	SDL_RenderFillRect(ren, &dst);
}

void Ext_KeyDown(VM* vm)
{
	int id = (int)PopNumber(vm);
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	PushNumber(vm, keys[id]);
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
	
	if(argc == 2)
		vm->debug = 1;
	
	LoadBinaryFile(vm, in);
	fclose(in);
	
	HookStandardLibrary(vm);
	
	HookExternNoWarn(vm, "setcolor", Ext_SetColor);
	HookExternNoWarn(vm, "fillrect", Ext_FillRect);
	HookExternNoWarn(vm, "keydown", Ext_KeyDown);
	
	SDL_Init(SDL_INIT_EVERYTHING);
	
	window = SDL_CreateWindow("Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
	ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	
	int initId = GetFunctionId(vm, "init");
	int updateId = GetFunctionId(vm, "update");
	int drawId = GetFunctionId(vm, "draw");
	
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
		
		
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
		SDL_RenderClear(ren);
		if(drawId >= 0)
			CallFunction(vm, drawId, 0);
		SDL_RenderPresent(ren);
	
		SDL_Delay(12);
	}
	
	SDL_Quit();
	DeleteVM(vm);
	return 0;
}

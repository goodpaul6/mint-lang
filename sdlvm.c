#include "vm.h"
#include "SDL2/SDL.h"

#include <stdlib.h>
#include <string.h>

void Ext_SDL_Init(VM* vm)
{
	int value = (int)PopNumber(vm);
	PushNumber(vm, (int)SDL_Init(value));
}

void Ext_SDL_Quit(VM* vm)
{
	SDL_Quit();
}

void Ext_SDL_DestroyWindow(void* pWin)
{
	printf("destroying window\n");
	SDL_Window* win = pWin;
	SDL_DestroyWindow(win);
}

void Ext_SDL_CreateWindow(VM* vm)
{
	const char* name = PopString(vm);
	int x = PopNumber(vm);
	int y = PopNumber(vm);
	int width = PopNumber(vm);
	int height = PopNumber(vm);
	int flags = PopNumber(vm);

	PushNative(vm, SDL_CreateWindow(name, x, y, width, height, flags), Ext_SDL_DestroyWindow, NULL);
}

void Ext_SDL_DestroyRenderer(void* pRen)
{
	printf("destroying renderer\n");
	SDL_Renderer* ren = pRen;
	SDL_DestroyRenderer(ren);
}

void Ext_SDL_CreateRenderer(VM* vm)
{
	SDL_Window* window = PopNative(vm);
	int flags = (int)PopNumber(vm);

	PushNative(vm, SDL_CreateRenderer(window, -1, flags), Ext_SDL_DestroyRenderer, NULL);
}

void Ext_SDL_DestroyEvent(void* pEvent)
{
	free(pEvent);
}

void Ext_SDL_CreateEvent(VM* vm)
{
	PushNative(vm, malloc(sizeof(SDL_Event)), Ext_SDL_DestroyEvent, NULL);
}

void Ext_SDL_PollEvent(VM* vm)
{
	SDL_Event* ev = PopNative(vm);
	PushNumber(vm, SDL_PollEvent(ev));
}

void Ext_SDL_EventType(VM* vm)
{
	SDL_Event* ev = PopNative(vm);
	PushNumber(vm, ev->type);
}

void Ext_SDL_IsKeyDown(VM* vm)
{
	const Uint8* keys = SDL_GetKeyboardState(NULL);
	int key = (int)PopNumber(vm);
	PushNumber(vm, (char)keys[key]);
}

void Ext_SDL_RenderClear(VM* vm)
{
	SDL_Renderer* ren = PopNative(vm);
	SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
	SDL_RenderClear(ren);
}

void Ext_SDL_RenderPresent(VM* vm)
{
	SDL_Renderer* ren = PopNative(vm);
	SDL_RenderPresent(ren);
}

void Ext_SDL_RenderFillRect(VM* vm)
{
	SDL_Renderer* ren = (SDL_Renderer*)PopNative(vm);
	int x = (int)PopNumber(vm);
	int y = (int)PopNumber(vm);
	int w = (int)PopNumber(vm);
	int h = (int)PopNumber(vm);

	SDL_Rect rect = { x, y, w, h };
	SDL_RenderFillRect(ren, &rect);
}

void Ext_SDL_SetRenderDrawColor(VM* vm)
{
	SDL_Renderer* ren = (SDL_Renderer*)PopNative(vm);
	int r = (int)PopNumber(vm);
	int g = (int)PopNumber(vm);
	int b = (int)PopNumber(vm);
	int a = (int)PopNumber(vm);

	SDL_SetRenderDrawColor(ren, r, g, b, a);
}

void Ext_SDL(VM* vm)
{
	const char* name = PopString(vm);
	
	if(strcmp(name, "SDL_INIT_EVERYTHING") == 0)
		PushNumber(vm, SDL_INIT_EVERYTHING);
	else if(strcmp(name, "SDL_INIT_VIDEO") == 0)
		PushNumber(vm, SDL_INIT_VIDEO);
	else if(strcmp(name, "SDL_WINDOWPOS_CENTERED") == 0)
		PushNumber(vm, SDL_WINDOWPOS_CENTERED);
	else if(strcmp(name, "SDL_WINDOW_SHOWN") == 0)
		PushNumber(vm, SDL_WINDOW_SHOWN);
	else if(strcmp(name, "SDL_RENDERER_ACCELERATED") == 0)
		PushNumber(vm, SDL_RENDERER_ACCELERATED);
	else if(strcmp(name, "SDL_RENDERER_PRESENTVSYNC") == 0)
		PushNumber(vm, SDL_RENDERER_PRESENTVSYNC);
	else if(strcmp(name, "SDL_QUIT") == 0)
		PushNumber(vm, SDL_QUIT);
	else 
	{
		printf("Invalid SDL constant %s\nPushing 0...", name);
		PushNumber(vm, 0);
	}
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
		
		AllocateExterns(vm, 13);

		HookExtern(vm, 0, Ext_SDL_Init);
		HookExtern(vm, 1, Ext_SDL_Quit);
		HookExtern(vm, 2, Ext_SDL_CreateWindow);
		HookExtern(vm, 3, Ext_SDL_CreateRenderer);
		HookExtern(vm, 4, Ext_SDL_CreateEvent);
		HookExtern(vm, 5, Ext_SDL_PollEvent);
		HookExtern(vm, 6, Ext_SDL_EventType);
		HookExtern(vm, 7, Ext_SDL_IsKeyDown);
		HookExtern(vm, 8, Ext_SDL_RenderClear);
		HookExtern(vm, 9, Ext_SDL_RenderPresent);
		HookExtern(vm, 10, Ext_SDL_RenderFillRect);
		HookExtern(vm, 11, Ext_SDL_SetRenderDrawColor);
		HookExtern(vm, 12, Ext_SDL);

		vm->debug = debugFlag;
		LoadBinaryFile(vm, bin);
		RunVM(vm);
		DeleteVM(vm);

		return 0;
	}
	else 
	{
		fprintf(stderr, "Invalid number of command line arguments!\n");
		return 1;
	}
}
extern SDL
extern SDL_Init
extern SDL_Quit
extern SDL_CreateWindow
extern SDL_CreateEvent
extern SDL_CreateRenderer
extern SDL_PollEvent
extern SDL_EventType
extern SDL_RenderClear
extern SDL_RenderPresent
extern SDL_RenderFillRect
extern SDL_SetRenderDrawColor
extern SDL_GetTicks
extern SDL_IsKeyDown

var kright
var kleft
var kup
var kdown

func MSDL_Init()
	SDL_Init(SDL("SDL_INIT_EVERYTHING"))
	
	kright = $4F
	kleft = $50
	kdown = $51
	kup = $52
end

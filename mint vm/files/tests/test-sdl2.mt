extern SDL_Init(number) : void
extern SDL_CreateWindow(string, number, number, number, number, number) : native
extern SDL_CreateRenderer(native, number, number) : native
extern SDL_CreateEvent() : native
extern SDL_PollEvent(native) : number
extern SDL_GetEventType(native) : number
extern SDL_Quit() : void

var SDL_QUIT = 0x100

var SDL_WINDOW_SHOWN = 0x00000004

var SDL_RENDERER_SOFTWARE = 0x00000001
var SDL_RENDERER_ACCELERATED = 0x00000002
var SDL_RENDERER_PRESENTVSYNC = 0x00000004
var SDL_RENDERER_TARGETTEXTURE = 0x00000008

var SDL_INIT_TIMER = 0x00000001
var SDL_INIT_AUDIO = 0x00000010
var SDL_INIT_VIDEO = 0x00000020
var SDL_INIT_JOYSTICK = 0x00000200
var SDL_INIT_HAPTIC = 0x00001000
var SDL_INIT_GAMECONTROLLER = 0x00002000
var SDL_INIT_EVENTS = 0x00004000
var SDL_INIT_NOPARACHUTE = 0x00100000
var SDL_INIT_EVERYTHING = SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO |
	SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_GAMECONTROLLER |
	SDL_INIT_EVENTS
	
func run()
	SDL_Init(SDL_INIT_EVERYTHING)
	
	var window = SDL_CreateWindow("game", 0, 0, 640, 480, SDL_WINDOW_SHOWN)
	var renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)
	var event = SDL_CreateEvent()
	var running = true
	
	while true do
		write("hello")
	end
	
	SDL_Quit()
end

run()

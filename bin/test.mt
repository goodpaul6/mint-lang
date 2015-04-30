# sample SDL2 application

extern print

extern SDL_Init
extern SDL_Quit

extern SDL_CreateWindow

extern SDL_CreateEvent
extern SDL_PollEvent
extern SDL_EventType

extern SDL_IsKeyDown

extern SDL_CreateRenderer
extern SDL_RenderClear
extern SDL_RenderPresent
extern SDL_RenderFillRect
extern SDL_SetRenderDrawColor

extern SDL_GetTicks

extern SDL

var right
var left
var down
var up

var x
var y

func update()
	if SDL_IsKeyDown(left)
		x = x - 4
	end
	
	if SDL_IsKeyDown(right)
		x = x + 4
	end
	
	if SDL_IsKeyDown(up)
		y = y - 4
	end
	
	if SDL_IsKeyDown(down)
		y = y + 4
	end
end

func _main()
	x = 0
	y = 0
	
	right = $4F
	left = $50
	down = $51
	up = $52

	var video = SDL("SDL_INIT_VIDEO")
	var quit = SDL("SDL_QUIT")
	
	SDL_Init(video)
	
	var window = SDL_CreateWindow("Test", 100, 100, 640, 480, SDL("SDL_WINDOW_SHOWN"))
	var renderer = SDL_CreateRenderer(window, SDL("SDL_RENDERER_ACCELERATED"))
	var event = SDL_CreateEvent()
	
	var running = 1

	var time = 0
	var last = SDL_GetTicks()

	while running
		while SDL_PollEvent(event)
			if SDL_EventType(event) == quit
				running = 0
			end
		end
		
		time = time + (SDL_GetTicks() - last)
		last = SDL_GetTicks()
		
		while time >= (1000 / 60)				
			update()
			time = time - (1000 / 60)
		end
	
		SDL_RenderClear(renderer)
		
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255)
		SDL_RenderFillRect(renderer, x, y, 32, 32)
		
		SDL_RenderPresent(renderer)
	end
	
	SDL_Quit()
	
	return;
end

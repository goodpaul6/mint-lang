extern print

extern SDL
extern SDL_Init
extern SDL_Quit
extern SDL_CreateWindow
extern SDL_CreateRenderer
extern SDL_CreateEvent
extern SDL_PollEvent
extern SDL_GetTicks

func _main()
	if SDL_Init() < 0
		print("could not initialize SDL2")
		return;
	end
	
	var window = SDL_CreateWindow("Game", 0, 0, 640, 480, SDL("SDL_WINDOW_SHOWN"))
	var renderer = SDL_CreateRenderer(renderer, -1, SDL("SDL_RENDERER_ACCELERATED"))
	var event = SDL_CreateEvent()

	var running = true
	
	var timePerFrame = (1000 / 60)
	var last = SDL_GetTicks()
	var time = 0
	
	init_bullets()
	
	add_bullet(320, 240, 0.5)
	
	while running
		while SDL_PollEvent(event)
			if SDL_EventType(event) == SDL("SDL_QUIT")
				running = false
			end
		end
		
		time = time + (SDL_GetTicks() - last)
		last = SDL_GetTicks()
		
		while time >= timePerFrame
			update_bullets()
			time = time - timePerFrame
		end
		
		SDL_RenderClear(renderer)
		draw_bullets(renderer)
		SDL_RenderPresent(renderer)
	end

	SDL_Quit()
end

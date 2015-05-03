extern print
extern push
extern len

extern cos
extern sin

extern sqrt
extern atan2

func _main()
	var pos = [0]
	
	for var y = 0, y < 10, y = y + 1
		for var x = 0, x < 10, x = x + 1 
			var v2 = [2]
			v2[0] = x * 36
			v2[1] = y * 36
			push(pos, v2)
		end
	end

	SDL_Init(SDL("SDL_INIT_EVERYTHING"))
	
	var cen = SDL("SDL_WINDOWPOS_CENTERED")
	
	var window = SDL_CreateWindow("Game", cen, cen, 640, 480, SDL("SDL_WINDOW_SHOWN"))
	var renderer = SDL_CreateRenderer(window, SDL("SDL_RENDERER_ACCELERATED"))
	var event = SDL_CreateEvent()
	
	var last = SDL_GetTicks()
	var time = 0
	
	var running = true
	while running
		while SDL_PollEvent(event)
			if SDL_EventType(event) == SDL("SDL_QUIT")
				running = false
			end
		end
		
		time = time + (SDL_GetTicks() - last)
		last = SDL_GetTicks()
		
		while time >= (1000 / 60)
			time = time - (1000 / 60)
		end
		
		SDL_RenderClear(renderer)
		SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255)
		for var i = 0, i < len(pos), i = i + 1
			SDL_RenderFillRect(renderer, pos[i][0], pos[i][1], 32, 32)
		end
		SDL_RenderPresent(renderer)
	end
	
	SDL_Quit()
	return 0
end

extern printf

extern cos
extern sin

extern sqrt
extern atan2

extern floor

var pos
var vel
var size

func _main()
	MSDL_Init()
	
	pos = [0]
	vel = [0]
	size = [0]
	
	var cen = SDL("SDL_WINDOWPOS_CENTERED")
	
	var window = SDL_CreateWindow("Game", cen, cen, 640, 480, SDL("SDL_WINDOW_SHOWN"))
	var renderer = SDL_CreateRenderer(window, SDL("SDL_RENDERER_ACCELERATED"))
	var event = SDL_CreateEvent()
	
	var last = SDL_GetTicks()
	var time = 0
	
	var tpf = (1000 / 60)
	
	var running = true
	var quit = SDL("SDL_QUIT")
	var mousedown = SDL("SDL_MOUSEBUTTONDOWN")
	var mouseup = SDL("SDL_MOUSEBUTTONUP")
	
	init_player(100, 100)
	
	while running
		while SDL_PollEvent(event)
			if SDL_EventType(event) == quit
				running = false
			end
			
			if SDL_EventType(event) == mousedown
				is_mouse_down = true
			elif SDL_EventType(event) == mouseup
				is_mouse_down = false
			end
		end
		
		SDL_GetMousePos(mouse_x, mouse_y)
		
		time = time + (SDL_GetTicks() - last)
		last = SDL_GetTicks()
		
		if time > tpf * 10
			time = tpf * 10
		end
		
		while time >= tpf
			for var i = 0, i < len(pos), i = i + 2
				pos[i] = pos[i] + vel[i]
				pos[i + 1] = pos[i + 1] + vel[i + 1]
			end
			
			update_player()
			
			time = time - tpf
		end
			
		SDL_RenderClear(renderer)
		for var i = 0, i < len(pos), i = i + 2
			if (pos[i + 1] >= -32) && (pos[i] >= -32)
				if (pos[i] < 640) && (pos[i + 1] < 480)
					SDL_SetRenderDrawColor(renderer, 128, 255, 255, 255)
					SDL_RenderFillRect(renderer, pos[i], pos[i + 1], size[floor(i / 2)], size[floor(i / 2)])
				end
			end
		end
		SDL_RenderPresent(renderer)
		
		SDL_Delay(12)
	end
	
	SDL_Quit()
	return 0
end

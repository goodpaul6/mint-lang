extern print
extern push
extern len

extern cos
extern sin

extern sqrt
extern atan2

var pos
var vel

# mint virtual machine (mint-lang)

func abs(x)
	if x < 0
		return -x
	else 
		return x
	end
end

func accel(x, y)
	for var i = 0, i < len(vel), i = i + 2	
		vel[i] = vel[i] + x
		vel[i + 1] = vel[i + 1] + y
	end
end

func _main()
	MSDL_Init()
	
	pos = [0]
	vel = [0]
	
	for var y = 0, y < 30, y = y + 1
		for var x = 0, x < 30, x = x + 1
			push(pos, x * 36)
			push(pos, y * 36)
			
			push(vel, 0)
			push(vel, 0)
		end
	end
	
	var cen = SDL("SDL_WINDOWPOS_CENTERED")
	
	var window = SDL_CreateWindow("Game", cen, cen, 640, 480, SDL("SDL_WINDOW_SHOWN"))
	var renderer = SDL_CreateRenderer(window, SDL("SDL_RENDERER_ACCELERATED"))
	var event = SDL_CreateEvent()
	
	var last = SDL_GetTicks()
	var time = 0
	
	var plen = len(pos)
	var tpf = (1000 / 60)
	
	var running = true
	var quit = SDL("SDL_QUIT")
	while running
		while SDL_PollEvent(event)
			if SDL_EventType(event) == quit
				running = false
			end
		end
		
		time = time + (SDL_GetTicks() - last)
		last = SDL_GetTicks()
		
		if time > tpf * 10
			time = tpf * 10
		end
		
		while time >= tpf
			for var i = 0, i < plen, i = i + 2
				pos[i] = pos[i] + vel[i]
				pos[i + 1] = pos[i + 1] + vel[i + 1]
				
				vel[i] = vel[i] * 0.90
				vel[i + 1] = vel[i + 1] * 0.90
			end
		
			if SDL_IsKeyDown(kleft) 
				accel(-2, 0)
			elif SDL_IsKeyDown(kright)
				accel(2, 0)
			elif SDL_IsKeyDown(kup)
				accel(0, -2)
			elif SDL_IsKeyDown(kdown)
				accel(0, 2)
			end
			
			time = time - tpf
		end
			
		SDL_RenderClear(renderer)
		for var i = 0, i < plen, i = i + 2
			if pos[i] >= -32 
				if pos[i + 1] >= -32
					if pos[i] < 640
						if pos[i + 1] < 480
							var v = (1 - i / len(pos)) * 255
							SDL_SetRenderDrawColor(renderer, 255 - v, v, v, 255)
							SDL_RenderFillRect(renderer, pos[i], pos[i + 1], 32, 32)
						end
					end
				end
			end
		end
		SDL_RenderPresent(renderer)
		
		SDL_Delay(12)
	end
	
	SDL_Quit()
	return 0
end

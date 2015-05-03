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
	for var yy = 0, yy < 10, yy = yy + 1
		for var xx = 0, xx < 10, xx = xx + 1
			var i = yy * 10 + xx
			
			vel[i][0] = vel[i][0] + x
			vel[i][1] = vel[i][1] + y
		end
	end
end

func _main()
	MSDL_Init()
	
	pos = [0]
	vel = [0]
	
	for var y = 0, y < 10, y = y + 1
		for var x = 0, x < 10, x = x + 1
			var p2 = [2]
			p2[0] = x * 36
			p2[1] = y * 36
			
			push(pos, p2)
			
			var v2 = [2]
			v2[0] = 0
			v2[1] = 0
			
			push(vel, v2)
		end
	end
	
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
			for var i = 0, i < len(pos), i = i + 1
				pos[i][0] = pos[i][0] + vel[i][0]
				pos[i][1] = pos[i][1] + vel[i][1]
				
				vel[i][0] = vel[i][0] * 0.90
				vel[i][1] = vel[i][1] * 0.90
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
		
			time = time - (1000 / 60)
		end
		
		SDL_RenderClear(renderer)
		for var i = 0, i < len(pos), i = i + 1			
			SDL_SetRenderDrawColor(renderer, (1 - i / len(pos)) * 255, (1 - i / len(pos)) * 255, (1 - i / len(pos)) * 255, 255)
			SDL_RenderFillRect(renderer, pos[i][0], pos[i][1], 32, 32)
		end
		SDL_RenderPresent(renderer)
	end
	
	SDL_Quit()
	return 0
end

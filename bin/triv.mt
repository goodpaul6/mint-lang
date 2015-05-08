extern tostring
extern printf

func entity()
	var e = dict()
	e.type = "none"
	e.x = 0
	e.y = 0
	e.dx = 0
	e.dy = 0
	e.radius = 8
	
	e.color = [255, 255, 255]
	
	e.update = @entity_update
	e.draw = @entity_draw
	
	return e
end

func entity_update(self)
	var x = self.x
	var y = self.y
	
	x = x + self.dx
	y = y + self.dy
	
	self.x = x
	self.y = y
end

func entity_draw(self, ren)
	SDL_SetRenderDrawColor(ren, self.color[0], self.color[1], self.color[2], 255)
	
	var rad = self.radius
	SDL_RenderFillRect(ren, self.x - rad, self.y - rad, rad * 2, rad * 2) 
end

func player(x, y)
	var e = entity()
	e.type = "player"
	e.x = x
	e.y = y
	e.color[1] = 0
	e.color[2] = 0
	e.update = @player_update
	
	return e
end

func player_update(self)
	self.dx = self.dx * 0.9
	self.dy = self.dy * 0.9

	if SDL_IsKeyDown(kleft)
		self.dx = -2
	end
	if SDL_IsKeyDown(kright)
		self.dx = 2
	end
	if SDL_IsKeyDown(kup)
		self.dy = -2
	end
	if SDL_IsKeyDown(kdown)
		self.dy = 2
	end
	
	entity_update(self)
end

func _main()
	MSDL_Init()
	
	var ents = []
	
	for var y = 0, y < 20, y = y + 1
		for var x = 0, x < 20, x = x + 1
			push(ents, player(x * 32, y * 32))
		end
	end
	
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
			for var i = 0, i < len(ents), i = i + 1
				var ent = ents[i]
				call(ent.update, ent)
			end	

			time = time - tpf
		end
			
		SDL_RenderClear(renderer)
		for var i = 0, i < len(ents), i = i + 1
			var ent = ents[i]
			if ent.x > 0 && ent.y > 0 && ent.x < 640 && ent.y < 480
				call(ent.draw, ent, renderer)
			end
		end
		SDL_RenderPresent(renderer)
		
		SDL_Delay(12)
	end
	
	SDL_Quit()
	return 0
end

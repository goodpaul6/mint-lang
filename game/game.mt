var assets

func load_assets()
	assets = {
		tileset = sfTexture_createFromFile("assets/tileset.png"),
		player = sfTexture_createFromFile("assets/player.png")
	}
end

func tilemap(window, texture, data, width, height)
	var self = {}

	self.batch = sprite_batch(window)
	self.texture = texture
	self.data = data
	self.width = width
	self.height = height
	
	self.update = tilemap_update
	self.draw = tilemap_draw
	
	return self
end

func tilemap_update(self)
	self.batch:clear()
	
	for var y = 0, y < self.height, y += 1
		for var x = 0, x < self.width, x += 1
			var tile = self.data[x + y * self.width]

			if tile >= 0
				var size = sfTexture_getSize(self.texture)
				
				var columns = size[0] / 8
				
				var u = floor(tile % columns)
				var v = floor(tile / columns)
				
				self.batch:appendEx(x * 32, y * 32, self.texture, u * 8, v * 8, 8, 8, 0, 4, 4, 0, 0) 
			end
		end
	end
end

func tilemap_draw(self, window, states)
	self:update()
	self.batch:draw(window, states)
end

func main()
	sfInit()

	load_assets()

	var window = sfRenderWindow_create(800, 600, 32, "Game")
	var event = sfEvent_create()
	var map = tilemap(window, assets.tileset,
	[0,1,0,1,0,0,1,0,0,0,
	 0,2,2,2,2,2,2,2,2,0,
	 0,2,2,2,2,2,2,2,2,1,
	 1,2,2,2,2,2,2,2,2,0,
	 0,2,2,2,2,2,2,2,2,0,
	 1,2,2,2,2,2,2,2,2,1,
	 0,2,2,2,2,2,2,2,2,0,
	 1,2,2,2,2,2,2,2,2,0,
	 0,2,2,2,2,2,2,2,2,0,
	 0,0,0,1,0,1,0,0,0,0], 10, 10)
	
	var sprite = sprite()
	
	sprite:setScale([2, 2])
	sprite:setTexture(assets.player)
	sprite:setTextureRect([16, 0, 16, 16])
	sprite:setPosition([100, 100])
	
	sfRenderWindow_setFramerateLimit(window, 60)
	
	while sfRenderWindow_isOpen(window)
		while sfRenderWindow_pollEvent(window, event)
			if sfEvent_type(event) == sfEvtClosed
				sfRenderWindow_close(window)
			end
		end
		
		sfRenderWindow_clear(window)
		sfRenderWindow_draw(window, map, null)
		sfRenderWindow_draw(window, sprite, null)
		sfRenderWindow_display(window)
	end
end

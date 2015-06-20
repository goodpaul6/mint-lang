var assets
var keys

func load_assets()
	assets = {
		tileset = sfTexture_createFromFile("assets/tileset.png"),
		player = sfTexture_createFromFile("assets/player.png"),
		bullet = sfTexture_createFromFile("assets/bullet.png"),
		zombie = sfTexture_createFromFile("assets/zombie.png")
	}
end

func tilemap_create(window)
	var self = {}

	self.batch = sprite_batch(window)
	self.texture = null
	self.data = null
	self.width = 0
	self.height = 0
	self.tileWidth = 0
	self.tileHeight = 0
	self.tileScaleX = 0
	self.tileScaleY = 0
	
	self.set = tilemap_set
	self.update = tilemap_update
	self.draw = tilemap_draw
	self.getTile = tilemap_getTile
	self.setTile = tilemap_setTile
	
	return self
end

func tilemap_set(self, data, width, height, texture, tileWidth, tileHeight, tileScaleX, tileScaleY)
	self.data = data
	self.width = width
	self.height = height
	self.texture = texture
	self.tileWidth = tileWidth
	self.tileHeight = tileHeight
	self.tileScaleX = tileScaleX
	self.tileScaleY = tileScaleY
end

func tilemap_getTile(self, x, y)
	var xx = floor(x)
	var yy = floor(y)
	
	if xx < 0 || xx >= self.width return -1 end
	if yy < 0 || yy >= self.height return -1 end
	
	return self.data[xx + yy * self.width]
end

func tilemap_setTile(self, x, y, tile)
	var xx = floor(x)
	var yy = floor(y)
	
	if xx < 0 || xx >= self.width return -1 end
	if yy < 0 || yy >= self.height return -1 end
	
	self.data[xx + yy * self.width] = tile
end

func tilemap_update(self)
	self.batch:clear()
	
	for var y = 0, y < self.height, y += 1
		for var x = 0, x < self.width, x += 1
			var tile = self.data[x + y * self.width]
			if tile == null
				tile = -1
			end
			
			# printf("%g", tile)

			if tile >= 0
				var size = sfTexture_getSize(self.texture)
				
				var columns = size[0] / self.tileWidth
				
				var u = floor(tile % columns)
				var v = floor(tile / columns)
				
				self.batch:appendEx(x * (self.tileWidth * self.tileScaleX), y * (self.tileHeight * self.tileScaleY), self.texture, u * self.tileWidth, v * self.tileHeight, self.tileWidth, self.tileHeight, 0, self.tileScaleX, self.tileScaleY, 0, 0) 
			end
		end
		
		# printf("\n")
	end
end

func tilemap_draw(self, window, states)
	self:update()
	sfRenderWindow_draw(window, self.batch, states)
end

func room_create(window)
	var self = {}
	
	self.player = null
	
	self.entities = []
	self.toAdd = []
	self.map = tilemap_create(window)
		
	var width = 10
	var height = 10
	var data = array(width * height)
	
	self.add = room_add
	self.update = room_update
	self.draw = room_draw
	self.hasCollision = room_hasCollision
	
	for var y = 0, y < height, y += 1
		data[y * width] = rand() % 2
		data[(width - 1) + y * width] = rand() % 2
	end
	
	for var x = 0, x < width, x += 1
		data[x] = rand() % 2
		data[x + (height - 1) * width] = rand() % 2
	end
	
	for var y = 1, y < height - 1, y += 1
		for var x = 1, x < width - 1, x += 1
			var r = rand() % 50
			if r > 1
				data[x + y * width] = 2
			else
				data[x + y * width] = rand() % 2
			end
		end
	end
	
	self.map:set(data, width, height, assets.tileset, 8, 8, 4, 4)
	
	return self
end

func room_add(self, e)
	setvmdebug(true)
	push(self.toAdd, e)
	setvmdebug(false)
end

func room_update(self)
	for var i = 0, i < len(self.entities), i += 1
		var e = self.entities[i]
		e:update()
		
		for var j = i + 1, j < len(self.entities), j += 1
			var a = self.entities[i]
			var b = self.entities[j]
			
			if a:overlaps(b)
				a:seperate(b)
			end
		end
		
		if e.hp <= 0
			erase(self.entities, i)
		end
	end
	
	for var i = 0, i < len(self.toAdd), i += 1
		self.toAdd[i].room = self
		push(self.entities, self.toAdd[i])
	end
	clear(self.toAdd)
end

func room_draw(self, window, states)
	sfRenderWindow_draw(window, self.map, states)
	for var i = 0, i < len(self.entities), i += 1
		var e = self.entities[i]
		sfRenderWindow_draw(window, e, states)
	end
end

func room_hasCollision(self, x, y, radius)
	var tw = self.map.tileWidth * self.map.tileScaleX
	var th = self.map.tileHeight * self.map.tileScaleY
	
	var minX = floor((x - radius) / tw)
	var minY = floor((y - radius) / th)
	var maxX = floor((x + radius) / tw)
	var maxY = floor((y + radius) / th)
	
	for var y = minY, y <= maxY, y = y + 1
		for var x = minX, x <= maxX, x = x + 1
			var tile = self.map:getTile(x, y)
			
			if tile == 0 || tile == 1
				return true
			end
		end
	end
	
	return false
end

func entity_create()
	return {
		room = null,
		sprite = sprite(),
		rect = sfVertexArray_create(),
		x = 0, y = 0,
		dx = 0, dy = 0,
		radius = 0,
		tag = "none",
		hp = 10,
		
		update = entity_update,
		hitWallX = entity_hitWallX,
		hitWallY = entity_hitWallY,
		draw = entity_draw,
		overlaps = entity_overlaps,
		seperate = entity_seperate,
		contact = entity_contact,
		dist = entity_dist
	}
end

func entity_update(self)
	for var i = 0, i < 5, i += 1
		if self.room:hasCollision(self.x + self.dx / 5, self.y, self.radius)
			if self:hitWallX() break
			else self.x = self.x + self.dx / 5 end
		else
			self.x = self.x + self.dx / 5
		end
	end

	for var i = 0, i < 5, i += 1
		if self.room:hasCollision(self.x, self.y + self.dy / 5, self.radius)
			if self:hitWallY() break
			else self.y = self.y + self.dy / 5 end
		else
			self.y = self.y + self.dy / 5
		end
	end
	
	sfVertexArray_clear(self.rect)
	sfVertexArray_appendQuadEx(self.rect, self.x - self.radius, self.y - self.radius, self.radius * 2, self.radius * 2, [255, 0, 0, 100], [0, 0], [0, 0], [0, 0], [0, 0])

	self.sprite:setPosition([self.x - self.radius, self.y - self.radius])
end

func entity_hitWallX(self)
	self.dx = 0
	return true
end

func entity_hitWallY(self)
	self.dy = 0
	return true
end

func entity_draw(self, window, states)
	# sfRenderWindow_drawVertexArray(window, self.rect, states)
	sfRenderWindow_draw(window, self.sprite, states)
end

func entity_overlaps(self, other)
	var dist2 = (self.x - other.x) * (self.x - other.x) + (self.y - other.y) * (self.y - other.y)
	var rad2 = (self.radius + other.radius) * (self.radius + other.radius)
	
	if dist2 < rad2 return true end
	return false
end

func entity_seperate(self, other)
	if(self:contact(other)) && other:contact(self)
		var dist = sqrt((self.y - other.y) * (self.y - other.y) + (self.x - other.x) * (self.x - other.x))
		
		var angle = atan2(other.y - self.y, other.x - self.x)
		var force = 0.2
		var factor = (self.radius + other.radius) / dist
	
		self.dx = self.dx - cos(angle) * force * factor
		self.dy = self.dy - sin(angle) * force * factor
		
		other.dx = other.dx + cos(angle) * force * factor
		other.dy = other.dy + sin(angle) * force * factor
	end
end

func entity_contact(self, other)
	return true
end

func entity_dist(self, other)
	var dist2 = (other.x - self.x) * (other.x - self.x) + (other.y - self.y) * (other.y - self.y)
	return sqrt(dist2)
end

var weapons

func player_create(x, y)
	var self = entity_create()
	
	self.grounded = false
	
	self.x = x
	self.y = y
	
	self.sprite:setTexture(assets.player)
	self.sprite:setScale([4, 4])
	
	self.update = player_update
	self.shoot = shoot_machine
	
	self.radius = 12
	self.shotCooldown = 0
	
	self.hitWallY = player_hitWallY
	
	return self
end

func player_update(self)
	self.room.player = self

	self.dx = self.dx * 0.92
	
	if !self.grounded
		self.dy = self.dy + 0.5
	end
	
	if keys[sfKeyW]
		if self.grounded
			self.dy = -5
			self.grounded = false
		end
	end
	
	self.grounded = false
	
	if keys[sfKeyA] self.dx = -2 end
	if keys[sfKeyD] self.dx = 2 end
	
	if keys[sfKeyTab]
		self.weaponIndex = self.weaponIndex + 1
		if self.weaponIndex >= len(weapons)
			self.weaponIndex = 0
		end
		
		self.shoot = weapons[self.weaponIndex]
		keys[sfKeyTab] = false
	end
	
	if self.shotCooldown > 0
		self.shotCooldown = self.shotCooldown - 1
	end
	
	if self.shotCooldown <= 0
		var dir = -1
		if keys[sfKeyUp] dir = 0
		elif keys[sfKeyRight] dir = 1
		elif keys[sfKeyDown] dir = 2
		elif keys[sfKeyLeft] dir = 3 end

		if dir == 0 self.shotCooldown = self.shoot(self.room, self.x, self.y, atan2(-1, 0))
		elif dir == 1 self.shotCooldown = self.shoot(self.room, self.x, self.y, atan2(0, 1))
		elif dir == 2 self.shotCooldown = self.shoot(self.room, self.x, self.y, atan2(1, 0))
		elif dir == 3 self.shotCooldown = self.shoot(self.room, self.x, self.y, atan2(0, -1)) end
	
		if dir == 0 self.dy = self.dy + 0.05 * self.shotCooldown
		elif dir == 1 self.dx = self.dx - 0.05 * self.shotCooldown
		elif dir == 2 self.dy = self.dy - 0.05 * self.shotCooldown
		elif dir == 3 self.dx = self.dx + 0.05 * self.shotCooldown end
	end
	
	entity_update(self)
	self.sprite:setPosition([self.x - 16, self.y - 16])
end

func player_hitWallY(self)
	if self.dy > 0
		self.grounded = true
	end
	self.dy = 0
	return true
end

func shoot_pistol(room, x, y, angle)
	room:add(bullet_create(x, y, angle))
	return 20
end

func shoot_shotgun(room, x, y, angle)
	for var i = 0, i < 6, i = i + 1
		var angleOffset = rand() % 20 - 10
		angleOffset = angleOffset * (sfPi / 180)
		
		room:add(bullet_create(x, y, angle + angleOffset))
	end
	return 40
end

func shoot_machine(room, x, y, angle)
	shoot_pistol(room, x, y, angle)
	return 8
end

func zombie_create(x, y)
	var self = entity_create()
	
	self.type = "zombie"
	
	self.x = x
	self.y = y
	
	self.sprite:setTexture(assets.zombie)
	self.sprite:setScale([4, 4])
	
	self.radius = 12
	
	self.update = zombie_update

	return self
end

func zombie_update(self)
	self.dx = self.dx * 0.2
	self.dy = self.dy * 0.2
	if self.room.player != null
		var player = self.room.player
		
		var ang = atan2(player.y - self.y, player.x - self.x)
		var dist = player:dist(self)
		
		if dist > self.radius + player.radius + 100
			self.dx += cos(ang)
			self.dy += sin(ang)
		else
			# TODO: make zombie attack with cooldown
		end
	end
	
	entity_update(self)
	self.sprite:setPosition([self.x - 16, self.y - 16])
end

var bullet_pool
var bullet_pool_index

func bullet_create(x, y, angle)
	if bullet_pool == null
		bullet_pool = array(300)
		for var i = 0, i < len(bullet_pool), i = i + 1
			var self = entity_create()
			
			self.sprite:setTexture(assets.bullet)
			
			self.radius = 2
			
			self.contact = bullet_contact
			self.hitWallX = bullet_hitWallX
			self.hitWallY = bullet_hitWallY
			
			bullet_pool[i] = self
		end
		bullet_pool_index = 0
	end
	
	var self = bullet_pool[bullet_pool_index]
		
	bullet_pool_index = bullet_pool_index + 1
	if bullet_pool_index >= len(bullet_pool)
		bullet_pool_index = 0
	end
	
	self.sprite:setPosition([x, y])
	
	self.x = x
	self.y = y
	
	self.dx = cos(angle) * 4
	self.dy = sin(angle) * 4

	self.hp = 10

	return self
end

func bullet_contact(self, other)
	if other.type == "zombie"
		self.hp = 0
		other.hp = other.hp - 2
		if other.hp <= 0
			emit_blood(self.room, other.x, other.y, 5, 30)
		end
	end
	return false
end

func bullet_hitWallX(self)
	self.hp = 0
	emit_blood(self.room, self.x, self.y, 2)
end

func bullet_hitWallY(self)
	self.hp = 0
	emit_blood(self.room, self.x, self.y, 2)
end

var blood_pool
var blood_pool_index

func emit_blood(room, x, y, amount, ...)
	var minLife = 0
	
	if len(args) >= 1
		minLife = args[0]
	end
	
	if blood_pool == null
		blood_pool = array(400)
		
		var update = blood_update
		var contact = blood_contact
		
		for var i = 0, i < len(blood_pool), i = i + 1
			var e = entity_create()
	
			e.sprite:setTexture(assets.bullet)
			e.sprite:setColor([255, 0, 0, 255])
			
			e.radius = 2
			
			e.update = update
			e.contact = contact
			
			blood_pool[i] = e
		end
		
		blood_pool_index = 0
	end
	
	for var i = 0, i < amount, i = i + 1
		var e = blood_pool[blood_pool_index]
		blood_pool_index = blood_pool_index + 1
		if blood_pool_index >= len(blood_pool)
			blood_pool_index = 0
		end
		
		e.sprite:setPosition([x, y])
		
		e.x = x
		e.y = y

		var angle = rand() % 360
		angle = angle * (sfPi / 180)
		
		var speed = rand() % 3 + 2
		
		e.dx = cos(angle) * speed
		e.dy = sin(angle) * speed
		
		e.hp = rand() % 20 + minLife
		
		room:add(e)
	end
end

func blood_update(self)
	self.hp = self.hp - 1
	entity_update(self)
end

func blood_contact(self, other)
	return false
end

func main()
	sfInit()

	srand()

	load_assets()

	keys = array(sfNumKeys)

	weapons = [shoot_pistol, shoot_shotgun, shoot_machine]

	var window = sfRenderWindow_create(800, 600, 32, "Game")
	var event = sfEvent_create()
	
	var room = room_create(window)
	
	room:add(player_create(100, 100))
	for var i = 0, i < 10, i = i + 1
		room:add(zombie_create(200 + i, 100))
	end
	
	sfRenderWindow_setFramerateLimit(window, 60)
	
	while sfRenderWindow_isOpen(window)
		while sfRenderWindow_pollEvent(window, event)
			if sfEvent_type(event) == sfEvtClosed
				sfRenderWindow_close(window)
			elif sfEvent_type(event) == sfEvtKeyPressed
				var code = sfEvent_key_code(event)
				if code != -1
					keys[sfEvent_key_code(event)] = true
				end
			elif sfEvent_type(event) == sfEvtKeyReleased
				var code = sfEvent_key_code(event)
				if code != -1
					keys[sfEvent_key_code(event)] = false
				end
			end
		end
		
		room:update()
		
		sfRenderWindow_clear(window)
		sfRenderWindow_draw(window, room, null)
		sfRenderWindow_display(window)
	end
end

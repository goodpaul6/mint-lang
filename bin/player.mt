extern keydown

func player(x, y)
	var self = entity()
	self.x = x
	self.y = y
	self.w = 10
	self.h = 25
	
	self.update = @player_update
	
	return self
end

func player_update(self, dt)
	self.dx *= 0.9
	self.dy *= 0.9
	
	if keydown($4F)
		self.dx += 1000 * dt
	end
	
	if keydown($50)
		self.dx -= 1000 * dt
	end
	
	if keydown($51)
		self.dy += 1000 * dt
	end
	
	if keydown($52)
		self.dy -= 1000 * dt
	end
	
	entity_update(self, dt)
end

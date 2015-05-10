extern setcolor
extern fillrect

func entity()
	return 
	{ 
		type = "none", 
		x = 0, y = 0, 
		w = 0, h = 0, 
		dx = 0, dy = 0, 
		hp = 10, color = [255, 255, 255, 255],
		
		update = @entity_update,
		draw = @entity_draw,
	}
end

func entity_update(self, dt)
	self.x += self.dx * dt
	self.y += self.dy * dt
end

func entity_draw(self)
	setcolor(self.color)
	fillrect(self.x, self.y, self.w, self.h)
end

func check_rects(x1, y1, w1, h1, x2, y2, w2, h2)
	if x1 + w1 < x2 || x2 + w2 < x1 return false end
	if y1 + h1 < y2 || y2 + h2 < y1 return false end
	
	return true
end

func entity_collide(self, x, y, types)
	for var i = 0, i < len(entities), i += 1
		var ent = entities[i]
		if ent == self return; end
		
		if check_rects(self.x, self.y, self.w, self.h, ent.x, ent.y, ent.w, ent.h)			
			for var j = 0, j < len(types), j += 1
				if ent.type == types[i]
					return ent
				end
			end
		end
	end
	
	return null
end

func entity_moveby(self, x, y, types)
	for var i = 0, i < 3, i += 1
		var e = entity_collide(self, self.x + x / 3, self.y, types)
		
		if e != null
			if call(self.collx, self, e)
				
			end
		end
	end
end

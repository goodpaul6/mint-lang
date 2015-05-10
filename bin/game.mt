var entities

func init()
	entities = []
	
	push(entities, player(100, 100))
end

func update(dt)
	for var i = 0, i < len(entities), i += 1
		var ent = entities[i]
		call(ent.update, ent, dt)
	end
end

func draw()
	for var i = 0, i < len(entities), i += 1
		var ent = entities[i]
		call(ent.draw, ent)
	end
end

extern add
extern kill
extern move
extern keydown

var player

func init()
	ents = [0]
	player = add(8)
end

func update(dt)
	if keydown($4F) 
		move(player, 100 * dt, 0)
	end
	if keydown($50)
		move(player, -100 * dt, 0)
	end
	if keydown($51)
		move(player, 0, 100 * dt)
	end
	if keydown($52)
		move(player, 0, -100 * dt)
	end
end

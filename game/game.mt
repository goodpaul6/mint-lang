extern setcolor
extern fillrect
extern keydown

var player
var camera

func init()
	camera = { x = 0, y = 0 }
	player = { x = 100, y = 100, color = [255, 255, 255, 255] }
end

func update(dt)
	camera.x = player.x - 320
	camera.y = player.y - 240
	
	write(camera.x)
	
	# right
	if keydown($4F)
		player.x += 100 * dt
	end
	
	# left
	if keydown($50)
		player.x -= 100 * dt
	end
	
	# down
	if keydown($51)
		player.y += 100 * dt
	end
	
	# up
	if keydown($52)
		player.y -= 100 * dt
	end
end

func draw()
	setcolor(player.color)
	fillrect(player.x - camera.x, player.y - camera.y, 32, 32)
end

extern sin
extern cos
extern SDL_RenderFillRect
extern push
extern len

var bullet_pos
var bullet_vel
var bullet_speed

func init_bullets()
	bullet_pos = [0]
	bullet_vel = [0]
	bullet_speed = 2
end

func add_bullet(x, y, angle)
	var pos = [2]
	pos[0] = x
	pos[1] = y
	
	var vel = [2]
	vel[0] = cos(angle) * bullet_speed
	vel[1] = sin(angle) * bullet_speed
	
	push(bullet_pos, pos)
	push(bullet_vel, vel)
end

func update_bullets()
	for var i = 0, i < len(bullet_pos), i = i + 1
		var pos = bullet_pos[i]
		var vel = bullet_vel[i]
		
		pos[0] = pos[0] + vel[0]
		pos[1] = pos[1] + vel[1]
	end
end

func draw_bullets(ren)
	SDL_SetRenderDrawColor(ren, 255, 0, 0, 255)
	
	for var i = 0, i < len(bullet_pos), i = i + 1
		var pos = bullet_pos[i]
		SDL_RenderFillRect(ren, pos[0], pos[1], 8, 8)
	end
end

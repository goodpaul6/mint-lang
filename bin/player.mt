# runs on sdlvm

func init_player(x, y)
	var my_pos = len(pos)
	push(pos, x)
	push(pos, y)
	
	var my_vel = len(vel)
	push(vel, 0)
	push(vel, 0)
	
	push(size, 32)
	push(update, @update_player)
	
	var data = [3]
	data[0] = my_pos
	data[1] = my_vel
	data[2] = 0
		
	push(update_data, data)
end

func update_player(data)
	var my_pos = data[0]
	var my_vel = data[1]
	var shoot_timer = data[2]

	if shoot_timer > 0
		shoot_timer = shoot_timer - 1
	end

	vel[my_vel] = vel[my_vel] * 0.9
	vel[my_vel + 1] = vel[my_vel + 1] * 0.9

	if SDL_IsKeyDown(kleft)
		vel[my_vel] = vel[my_vel] - 1
	end
	
	if SDL_IsKeyDown(kright)
		vel[my_vel] = vel[my_vel] + 1
	end
	
	if SDL_IsKeyDown(kup)
		vel[my_vel + 1] = vel[my_vel + 1] - 1
	end
	
	if SDL_IsKeyDown(kdown)
		vel[my_vel + 1] = vel[my_vel + 1] + 1
	end
	
	#if is_mouse_down && (shoot_timer <= 0)
	#	var ang = atan2(mouse_y - pos[my_pos + 1], mouse_x - pos[my_pos])
	#	shoot(pos[my_pos], pos[my_pos + 1], ang)
	#	shoot_timer = 10
	#end
end

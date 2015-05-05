# runs on sdlvm

var player_pos
var player_vel
var player_shoot_timer

func init_player(x, y)
	player_pos = len(pos)
	push(pos, x)
	push(pos, y)
	
	player_vel = len(vel)
	push(vel, 0)
	push(vel, 0)
	
	push(size, 32)
	
	player_shoot_timer = 0
end

func update_player()
	if player_shoot_timer > 0
		player_shoot_timer = player_shoot_timer - 1
	end

	vel[player_vel] = vel[player_vel] * 0.9
	vel[player_vel + 1] = vel[player_vel + 1] * 0.9

	if SDL_IsKeyDown(kleft)
		vel[player_vel] = vel[player_vel] - 1
	end
	
	if SDL_IsKeyDown(kright)
		vel[player_vel] = vel[player_vel] + 1
	end
	
	if SDL_IsKeyDown(kup)
		vel[player_vel + 1] = vel[player_vel + 1] - 1
	end
	
	if SDL_IsKeyDown(kdown)
		vel[player_vel + 1] = vel[player_vel + 1] + 1
	end
	
	#if is_mouse_down && (player_shoot_timer <= 0)
	#	var ang = atan2(mouse_y - pos[player_pos + 1], mouse_x - pos[player_pos])
	#	shoot(pos[player_pos], pos[player_pos + 1], ang)
	#	player_shoot_timer = 10
	#end
end

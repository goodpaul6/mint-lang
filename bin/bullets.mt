# runs on sdlvm

extern erase

var bullets

func init_bullets()
	bullets = [0]
end

func shoot(x, y, angle)
	push(bullets, len(pos))
	push(pos, x)
	push(pos, y)
	
	push(bullets, len(vel))
	push(vel, cos(angle) * 2)
	push(vel, sin(angle) * 2)
	
	push(bullets, len(size))
	push(size, 8)

	push(bullets, 10)
end

func update_bullets()
	for var i = 0, i < len(bullets), i = i + 3
		var life = bullets[i + 2]
		bullets[i + 2] = life - 1
		
		if life <= 0
			write("hello")
			erase(pos, bullets[i])
			erase(pos, bullets[i] + 1)
			
			erase(vel, bullets[i + 1])
			erase(vel, bullets[i + 1] + 1)
			
			erase(size, bullets[i + 2])
			
			erase(bullets, i)
			erase(bullets, i + 1)
			erase(bullets, i + 2)
		end
	end
end

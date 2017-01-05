# edge.mt -- general testing for mint-lang

struct vec2
	x : number
	y : number
end

func vec2(x : number, y : number)
	return { x = x, y = y } as vec2
end

operator +(a : vec2, b : vec2)
	return vec2(a.x + b.x, a.y + b.y)
end

operator -(a : vec2, b : vec2)
	return vec2(a.x - b.x, a.y - b.y)
end

operator -(a : vec2)
	return vec2(-a.x, -a.y)
end

func run()
	var a = vec2(10, 10)
	write(-a)
end

run()

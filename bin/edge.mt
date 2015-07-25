# edge.mt -- testing cutting edge features in mint-lang

func vec2(x, y)
	return {
		x = x,
		y = y,
		
		toString = vec2_toString,
		mag = vec2_mag,
		normalize = vec2_normalize,
		
		ADD = vec2_add,
		SUB = vec2_sub,
		MUL = vec2_mul,
		DIV = vec2_div
	}
end

func vec2_toString(self)
	return mprintf("(%g, %g)", self.x, self.y)
end

func vec2_add(self, other)
	return vec2(self.x + other.x, self.y + other.y)
end

func vec2_sub(self, other)
	return vec2(self.x - other.x, self.y - other.y)
end

func vec2_mul(self, other)
	return vec2(self.x * other.x, self.y * other.y)
end

func vec2_div(self, other)
	return vec2(self.x / other.x, self.y / other.y)
end

func vec2_mag(self)
	return sqrt(self.x * self.x + self.y * self.y)
end

func vec2_normalize(self)
	var mag = self:mag()
	
	self.x = self.x / mag
	self.y = self.y / mag
end

func main()

	return 0
end

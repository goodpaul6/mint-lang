extern type

func _list_push(self, x)
	push(self._raw, x)
	self.length += 1
end

func _list_pop(self)
	var v = pop(self.raw)
	self.length -= 1
	return v
end

func _list_print(self)
	for var i = 0, i < self.length, i += 1
		print(self.raw[i])
	end
end

func list(v)
	return { length = len(v), raw = v, push = _list_push, pop = _list_pop, print = _list_print }
end

func print(v)
	if type(v) == type({})
		if v.print != null
			v:print()
			return;
		end
	elif type(v) == type([])
		for var i = 0, i < len(v), i += 1
			print(v[i])
		end
		return;
	end
	
	write(v)
end

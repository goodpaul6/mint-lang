# edge.mt -- testing cutting edge features in mint-lang

extern printf(string) : void
extern tonumber(dynamic) : number
extern type(dynamic) : string
extern strcat(string, string) : string

var fixed_mt = {
	SETINDEX = _fixed_set_index	
}

func _fixed_set_index(self, index, value)
	if self.message != null write(self.message)
	else printf("Attempted to add field '%s' to fixed object\n", index) end
end

func fixed(d)
	setmeta(d, fixed_mt)
	return d
end

var list = fixed {
	new = list_new,
	from_raw = list_from_raw,
	each = list_each,
	ieach = list_ieach,
	rev = list_reversed,
	mt = {
		GETINDEX = list_get_index,
		SETINDEX = list_set_index,
		LENGTH = list_length,
		EQUALS = list_equals
	}
}

func list_new(...)
	var self = {
		values = getargs()
	}

	setmeta(self, list.mt)

	return self
end

func list_from_raw(raw)
	var self = {
		values = raw
	}

	setmeta(self, list.mt)

	return self
end

func list_get_index(self, index)
	var list_method = list[index]
	if list_method != null return list_method end
	
	return self.values[index]
end

func list_set_index(self, index, value)
	self.values[index] = value
end

func list_equals(self, other)
	var values = self.values
	var bvalues = other.values
	for var i = 0, i < len(values), i = i + 1
		if values[i] != bvalues[i]
			return false
		end
	end

	return true
end

func list_each(self, a, ...)
	var values = self.values
	var args = getargs(a)
	for var i = 0, i < len(values), i = i + 1
		a(values[i], expand(args, len(args)))
	end
end

func list_ieach(self, a, ...)
	var values = self.values
	var args = getargs(a)
	for var i = 0, i < len(values), i = i + 1
		a(values[i], i, expand(args, len(args)))
	end
end

func list_length(self)
	return len(self.values)
end

func list_reversed(self)
	var values = self.values
	return list.from_raw(for var i = len(values) - 1, i >= 0, i = i - 1 
		values[i]
	end)
end

func add(x : number, y : number)
	return x + y
end

func main()
	var grades = [10, 20, 30]
	write(grades.hi)

	return 0
end

main()

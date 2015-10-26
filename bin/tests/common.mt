extern erase
extern arraycopy

var _list = {
	each = _list_each,
	ieach = _list_ieach,
	rev = _list_reversed,
	push = _list_push,
	pop = _list_pop,
	index_of = _list_index_of,
	remove = _list_remove,
	clear = _list_clear,
	copy = _list_copy,
	mt = {
		GETINDEX = _list_get_index,
		SETINDEX = _list_set_index,
		LENGTH = _list_length,
		EQUALS = _list_equals
	}
}

func list(...)
	var self = {
		values = getargs()
	}

	setmeta(self, _list.mt)

	return self
end

func list_from_raw(raw)
	var self = {
		values = raw
	}

	setmeta(self, _list.mt)

	return self
end

func _list_get_index(self, index)
	var list_method = _list[index]
	if list_method != null then return list_method end
	
	return self.values[index]
end

func _list_set_index(self, index, value)
	self.values[index] = value
end

func _list_equals(self, other)
	var values = self.values
	var bvalues = other.values
	for var i = 0, i < len(values), i = i + 1 do
		if values[i] != bvalues[i] then
			return false
		end
	end

	return true
end

func _list_each(self, a, ...)
	var values = self.values
	var args = getargs(a)
	for var i = 0, i < len(values), i = i + 1 do
		a(values[i], expand(args, len(args)))
	end
end

func _list_ieach(self, a, ...)
	var values = self.values
	var args = getargs(a)
	for var i = 0, i < len(values), i = i + 1 do
		a(values[i], i, expand(args, len(args)))
	end
end

func _list_push(self, value)
	push(self.values, value)
end

func _list_pop(self)
	return pop(self.values)
end

func _list_index_of(self, value)
	var values = self.values
	for var i = 0, i < len(values), i = i + 1 do
		if values[i] == value then
			return i
		end
	end

	return -1
end

func _list_remove(self, index)
	erase(self.values, index)
end

func _list_clear(self)
	clear(self.values)
end

func _list_length(self)
	return len(self.values)
end

func _list_reversed(self)
	var values = self.values
	return list.from_raw(for var i = len(values) - 1, i >= 0, i = i - 1 do
		values[i]
	end)
end

func _list_copy(self)
	
	var values = self.values
	var length = len(self.values)
	var copied = array(length)	
	
	arraycopy(copied, values, 0, 0, length)
	
	return list_from_raw(copied)
end

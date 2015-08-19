extern joinchars(array) : string
extern printf : void
extern floor(number) : number
extern halt : void
extern tostring(dynamic) : string
extern tonumber(dynamic) : number
extern erase(array, number) : void
extern typeof(dynamic) : string
extern strcat(string, string) : string
extern fopen(string, string) : native
extern getc(native) : number
extern putc(native, number) : void
extern bytes(number) : native
extern getbyte(native, number) : number
extern setbyte(native, number, number) : void
extern lenbytes(native) : number
extern assert(number) : void
extern rand : number
extern srand : void
extern sqrt(number) : void
extern atan2(number, number) : number
extern sin(number) : number
extern cos(number) : number
extern getfuncbyname(string) : function
extern getnumargs(function) : number
extern hasellipsis(function) : number
extern stringhash(string) : number
extern number_to_bytes(number, number) : native
extern bytes_to_number(native, number) : native
extern arraycopy(array, array, number, number, number) : void
extern arrayfill(array, dynamic) : void
extern arraysort(array, function-number) : void

var EOF = -1

var numbertype = typeof(0)
var stringtype = typeof("")
var arraytype = typeof([])
var dicttype = typeof({})
var functype = "function"
var nativetype = "native"

var on_exit = []

var _fixed_mt = {
	SETINDEX = _fixed_set_index	
}

func _fixed_set_index(self, index, value)
	if self.message != null then write(self.message)
	else printf("Attempted to add field '%s' to fixed object\n", index) end
end

func fixed(d)
	setmeta(d, _fixed_mt)
	return d
end

func char_to_string(c : number)
	return joinchars([c])
end

func string_to_array(s : string)
	var a = array(len(s))
	for var i = 0, i < len(s), i = i + 1 do
		a[i] = s[i]
	end
	
	return a
end

func absorb(a, b)
	var pairs = b.pairs
	for var i = 0, i < len(pairs), i = i + 1 do
		rawset(a, pairs[i][0], pairs[i][1])
	end
end

func dict_from_pairs(pairs : array-array-dynamic)
	var d = {}
	for var i = 0, i < len(pairs), i = i + 1 do
		rawset(d, pairs[i][0], pairs[i][1])
	end
	
	return d
end

func numberhash(n : number)
	return n
end

func objecthash(o : dynamic) : number
	if typeof(o) == dicttype then
		if getmeta(o) != null then
			return getmeta(o):HASH()
		end
	end
	
	return tonumber(o)
end

func map(hash : function-number)
    return {
        buckets = array(1024),
        hash = hash,
        SETINDEX = _map_set_index,
        GETINDEX = _map_get_index 
    }
end

func _map_set_index(self, index, value)
    var hash = hash(index)

    var i = hash % len(self.buckets)
    var b = {
        next = self.buckets[i],
        key = index,
        value = value
    }

    self.buckets[i] = b
end

func _map_get_index(self, index)
    var hash = self.hash(index)

    var bucket = self.buckets[hash % len(self.buckets)]
    while bucket do
        if bucket.key == index then
            return bucket.value
        else
            bucket = bucket.next
        end
    end

    return null
end

var _list = fixed {
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
	return list_from_raw(for var i = 0, i < len(values), i = i + 1 do
		values[i]
	end)
end

func mprintf(format : string, ...)
	return joinchars(_mprintf_raw_args(format, getargs(format)))
end

func _mprintf_add_chars(buf, string)
	for var i = 0, i < len(string), i = i + 1 do
		push(buf, string[i])
	end
end

func _mprintf_tostring(v)
	var t = typeof(v)
	if t == "string" then
		return v
	elif t == "dict" then
		var buf = []
		var p = v.pairs
		
		push(buf, '{')
		push(buf, ' ')
		
		for var i = 0, i < len(p), i = i + 1 do
			_mprintf_add_chars(buf, p[i][0])
			push(buf, ' ')
			push(buf, '=')
			push(buf, ' ')
			_mprintf_add_chars(buf, _mprintf_tostring(p[i][1]))
			
			if i + 1 < len(p) then
				push(buf, ',')
				push(buf, ' ')
			end
		end
		
		push(buf, ' ')
		push(buf, '}')
		
		return joinchars(buf)
	elif t == "array" then
		var buf = []
		push(buf, '[')
		for var i = 0, i < len(v), i = i + 1 do
			_mprintf_add_chars(buf, _mprintf_tostring(v[i]))
			if i + 1 < len(v) then
				push(buf, ',')
			end
		end
		push(buf, ']')
		
		return joinchars(buf)
	else 
		return tostring(v)
	end
end

func mprintf_raw(format : string, ...) : array-number
	return _mprintf_raw_args(format, getargs(format))
end

func _mprintf_number_to_byte_array(n : number, size : number)
	var b = byte_array(size)
	for var i = 0, i < size, i = i + 1 do
		b[i] = (n >> (8 * i)) & 0xff
	end
	
	return b
end

func number_to_byte_array(n : number, size : number)
	return _mprintf_number_to_byte_array(n, size)
end

func _mprintf_hex_string_from_byte_array(b : dict)
	var buf = array(b.length * 2)
	var hex = "0123456789ABCDEF"
	for var i = 0, i < b.length, i = i + 1 do
		var v = b[b.length - i - 1] & 0xff
		buf[i * 2] = hex[v >> 4]
		buf[i * 2 + 1] = hex[v & 0x0F]
	end
	return joinchars(buf)
end

func _mprintf_get_variable_number_size(n : number)
	var i = 0
	while n != 0 do
		n = n >> 8
		i = i + 1
	end
	
	return i
end

func _mprintf_raw_args(format : string, args : array-dynamic)
	var buf = []
	var arg = 0
	
	for var i = 0, i < len(format), i = i + 1 do
		if format[i] == '%' then
			i = i + 1
			if arg >= len(args) then
				printf("mprintf format overflow\n")
				return buf
			end
			
			if format[i] == 'c' then
				push(buf, args[arg])
			elif format[i] == 'x' then
				i = i + 1
				var nbuf = []
				while i < len(format) && isdigit(format[i]) do
					push(nbuf, format[i])
					i = i + 1
				end
				var size = tonumber(joinchars(nbuf))
				if size == 0 then size = _mprintf_get_variable_number_size(args[arg]) end
				
				_mprintf_add_chars(buf, _mprintf_hex_string_from_byte_array(_mprintf_number_to_byte_array(args[arg], size)))
			elif format[i] == 'g' then
				_mprintf_add_chars(buf, tostring(args[arg]))
			elif format[i] == 's' then
				_mprintf_add_chars(buf, args[arg])
			elif format[i] == 'o' then
				_mprintf_add_chars(buf, _mprintf_tostring(args[arg]))
			else
				printf("invalid mprintf format specifier '%c'\n", format[i])
				return buf
			end
			arg = arg + 1
		else
			push(buf, format[i])
		end
	end
	
	return buf
end


func mprintf_args(format : string, args : array-dynamic)
	return joinchars(_mprintf_raw_args(format, args))
end

func exprintf(format : string, ...)
	printf("%s", mprintf_args(format, getargs(format)))
end

func numcmp(x : number, y : number)
	return x - y
end

func strcmp(x : string, y : string)
	if len(x) < len(y) then
		for var i = 0, i < len(x), i = i + 1 do
			if x[i] != y[i] then
				return x[i] - y[i]
			end
		end
	else
		for var i = 0, i < len(y), i = i + 1 do
			if x[i] != y[i] then
				return x[i] - y[i]
			end
		end
	end
	
	return 0
end

func sort(v : array-dynamic, comp : function-number)
	arraysort(v, comp)
end

func isalpha(c : number)
	return if (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') then true else false end
end

func isdigit(c : number)
	return if (c >= '0' && c <= '9') then true else false end
end

func isalnum(c : number)
	return if isalpha(c) || isdigit(c) then true else false end
end

func isspace(c : number)
	return if c == '\n' || c == '\t' || c == ' ' then true else false end
end

func stream(...)
	var args = getargs()
	var raw = ""
	if len(args) > 0 then
		raw = args[0]
	end

	return { _raw = string_to_array(raw), pos = 0, read = _stream_read, write = _stream_write, write_byte = _stream_write_byte,
			 write_bytes = _stream_write_bytes, write_binary_value = _stream_write_binary_value,
			 close = _stream_close, _file_handle = null, _mode = null }
end

func _stream_is_eof(self)
	return (len(self._raw) - self.pos) == 0
end

func _stream_read(self)
	if self.pos >= len(self._raw) then
		return EOF
	end
	
	var c = self._raw[self.pos]
	self.pos = self.pos + 1
	
	return c
end

func _stream_write(self, c)
	if typeof(c) == numbertype then
		if self.pos < len(self._raw) then
			self._raw[self.pos] = c
		else
			push(self._raw, c)
		end
	
		self.pos = self.pos + 1
	elif typeof(c) == stringtype || typeof(c) == arraytype then
		for var i = 0, i < len(c), i = i + 1 do
			_stream_write(self, c[i])
		end
	end
end

func _stream_write_byte(self, b)
	if self.pos < len(self._raw) then
		self._raw[self.pos] = b
	else
		push(self._raw, b)
	end
	self.pos = self.pos + 1
end

func _stream_write_binary_value(self, v, type)
	_stream_write_bytes(self, byte_array_from_raw(number_to_bytes(v, type)))
end

func _stream_write_bytes(self, array)
	for var i = 0, i < len(array), i = i + 1 do
		_stream_write_byte(self, array[i])
	end
end

func _stream_close(self)
	if self._file_handle != null && self._mode[0] == 'w' then
		for var i = 0, i < len(self._raw), i = i + 1 do
			putc(self._file_handle, self._raw[i])
		end
	end
end

func fstream(path : string, mode : string)
	var f = fopen(path, mode)
	var buf = []
	
	if mode[0] == 'r' then
		var c = getc(f)
		
		while c != EOF do
			push(buf, c)
			c = getc(f)
		end
	end
	
	var s = stream(joinchars(buf))
	s._file_handle = f
	s._mode = mode
	return s
end

var _byte_array_mt = {
	SETINDEX = _byte_array_set_index,
	GETINDEX = _byte_array_get_index,
	LENGTH = _byte_array_get_length
}

func byte_array(n : number)
	var self = { 
		_raw = bytes(n), 
		length = n, 
	}

	setmeta(self, _byte_array_mt)

	return self
end

func byte_array_from_raw(b : native)
	var self = { 
		_raw = b, 
		length = lenbytes(b), 
	}

	setmeta(self, _byte_array_mt)

	return self
end

func _byte_array_get_length(self)
	return self.length
end

func _byte_array_set_index(self, index, value)
	setbyte(self._raw, index, value)
end

func _byte_array_get_index(self, index)
	return getbyte(self._raw, index)
end

func at_exit(f : function-void, ...)
	var args = getargs(f)
	push(on_exit, [f, args])
end

func exit(result : number)
	for var i = 0, i < len(on_exit), i = i + 1 do
		var f = on_exit[i][0]
		var data = on_exit[i][1]
		
		f(expand(data, len(data)))
	end
	
	halt(result)
end

func run()
	var main_pointer = getfuncbyname("main")
	assert(main_pointer != null, "Missing 'main' function")

	var r = main_pointer()

	exit(r) 
end

run()

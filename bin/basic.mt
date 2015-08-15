extern joinchars(array) : string
extern printf : void
extern floor(number) : number
extern halt : void
extern tostring(dynamic) : string
extern tonumber(dynamic) : number
extern erase(array, number) : void
extern type(dynamic) : string
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

var numbertype = type(0)
var stringtype = type("")
var arraytype = type([])
var dicttype = type({})
var functype = "function"
var nativetype = "native"

var on_exit = []

func fixed(x)
	dict_set(x, "SETINDEX", _fixed_set_index)
	return x
end

func _fixed_set_index(self, index, value)
	if dict_get(self, index) == null
		printf("Attempted to add field to fixed dict\n")
	else
		dict_set(self, index, value)
	end
end

func char_to_string(c : number)
	return joinchars([c])
end

func string_to_array(s : string)
	var a = array(len(s))
	for var i = 0, i < len(s), i = i + 1
		a[i] = s[i]
	end
	
	return a
end

func absorb(a, b)
	var pairs = b.pairs
	for var i = 0, i < len(pairs), i = i + 1
		dict_set(a, pairs[i][0], pairs[i][1])
	end
end

func dict_from_pairs(pairs : array-array-dynamic)
	var d = {}
	for var i = 0, i < len(pairs), i = i + 1
		dict_set(d, pairs[i][0], pairs[i][1])
	end
	
	return d
end

func numberhash(n : number)
	return n
end

func objecthash(o : dynamic) : number
	if type(o) == dicttype
		if o.HASH != null
			return o:HASH()
		end
	end
	
	return tonumber(o)
end

func map(hash : function-number) : dict
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
    while bucket != null
        if bucket.key == index
            return bucket.value
        else
            bucket = bucket.next
        end
    end

    return null
end

func list(...)
	var args = getargs()
	return { 
		_raw = args,
		length = len(args),
		push = _list_push,
		pop = _list_pop,
		clear = _list_clear,
		indexOf = _list_indexOf,
		remove = _list_remove,
		removeAll = _list_removeAll,
		toString = _list_toString,
		each = _list_each,
		GETINDEX = _list_getIndex,
		SETINDEX = _list_setIndex,
		LENGTH = _list_length
	}
end

func list_of_length(length : number)
	return {
		_raw = array(length),
		length = length,
		push = _list_push,
		pop = _list_pop,
		clear = _list_clear,
		indexOf = _list_indexOf,
		remove = _list_remove,
		removeAll = _list_removeAll,
		toString = _list_toString,
		each = _list_each,
		GETINDEX = _list_getIndex,
		SETINDEX = _list_setIndex,
		LENGTH = _list_length
	}
end

func list_from_array(raw)
	return {
		_raw = raw,
		length = len(raw),
		push = _list_push,
		pop = _list_pop,
		clear = _list_clear,
		indexOf = _list_indexOf,
		remove = _list_remove,
		removeAll = _list_removeAll,
		toString = _list_toString,
		each = _list_each,
		GETINDEX = _list_getIndex,
		SETINDEX = _list_setIndex,
		LENGTH = _list_length
	}
end

func _list_getIndex(self, index)
	return dict_get(self, "_raw")[index]
end

func _list_setIndex(self, index, value)
	dict_get(self, "_raw")[index] = value
end

func _list_push(self, value)
	push(self._raw, value)
	self.length = self.length + 1
end

func _list_pop(self)
	assert(self.length > 0, "attempted to pop from empty list")
	self.length = self.length - 1
	return pop(self._raw)
end

func _list_clear(self)
	clear(self._raw)
end

func _list_indexOf(self, value)
	var index = -1
	
	for var i = 0, i < self.length, i = i + 1
		if self._raw[i] == value
			index = i
		end
	end
	
	return index
end

func _list_remove(self, value)
	var index = self:indexOf(value)
	if index < 0 return;
	else erase(self._raw, index) end
end

func _list_removeAll(self, pred, data)
	var raw = []
	for var i = 0, i < self.length, i = i + 1
		if !pred(self._raw[i], data)
			push(raw, self._raw[i])
		end
	end
	
	self._raw = raw
end

func _list_toString(self)
	return mprintf("%o", self._raw)
end

func _list_length(self)
	return self.length
end

func _list_each(self, a)
	for var i = 0, i < self.length, i = i + 1
		a(self._raw[i])
	end
end

func mprintf(format : string, ...)
	return joinchars(_mprintf_raw_args(format, getargs(format)))
end

func _mprintf_add_chars(buf, string)
	for var i = 0, i < len(string), i = i + 1
		push(buf, string[i])
	end
end

func _mprintf_tostring(v)
	var t = type(v)
	if t == "string"
		return v
	elif t == "dict"
		var buf = []
		var p = v.pairs
		
		push(buf, '{')
		push(buf, ' ')
		
		for var i = 0, i < len(p), i = i + 1
			_mprintf_add_chars(buf, p[i][0])
			push(buf, ' ')
			push(buf, '=')
			push(buf, ' ')
			_mprintf_add_chars(buf, _mprintf_tostring(p[i][1]))
			
			if i + 1 < len(p)
				push(buf, ',')
				push(buf, ' ')
			end
		end
		
		push(buf, ' ')
		push(buf, '}')
		
		return joinchars(buf)
	elif t == "array"
		var buf = []
		push(buf, '[')
		for var i = 0, i < len(v), i = i + 1
			_mprintf_add_chars(buf, _mprintf_tostring(v[i]))
			if i + 1 < len(v)
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

func _mprintf_number_to_byte_array(n, size)
	var b = byte_array(size)
	for var i = 0, i < size, i = i + 1
		b[i] = (n >> (8 * i)) & 0xff
	end
	
	return b
end

func number_to_byte_array(n, size)
	return _mprintf_number_to_byte_array(n, size)
end

func _mprintf_hex_string_from_byte_array(b)
	var buf = array(b.length * 2)
	var hex = "0123456789ABCDEF"
	for var i = 0, i < b.length, i = i + 1
		var v = b[b.length - i - 1] & 0xff
		buf[i * 2] = hex[v >> 4]
		buf[i * 2 + 1] = hex[v & 0x0F]
	end
	return joinchars(buf)
end

func _mprintf_get_variable_number_size(n)
	var i = 0
	while n != 0
		n = n >> 8
		i = i + 1
	end
	
	return i
end

func _mprintf_raw_args(format, args)
	var buf = []
	var arg = 0
	
	for var i = 0, i < len(format), i = i + 1
		if format[i] == '%'
			i = i + 1
			if arg >= len(args)
				printf("mprintf format overflow\n")
				return buf
			end
			
			if format[i] == 'c'
				push(buf, args[arg])
			elif format[i] == 'x'
				i = i + 1
				var nbuf = []
				while i < len(format) && isdigit(format[i])
					push(nbuf, format[i])
					i = i + 1
				end
				var size = tonumber(joinchars(nbuf))
				if size == 0 size = _mprintf_get_variable_number_size(args[arg]) end
				
				_mprintf_add_chars(buf, _mprintf_hex_string_from_byte_array(_mprintf_number_to_byte_array(args[arg], size)))
			elif format[i] == 'g'
				_mprintf_add_chars(buf, tostring(args[arg]))
			elif format[i] == 's'
				_mprintf_add_chars(buf, args[arg])
			elif format[i] == 'o'
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
	if len(x) < len(y)
		for var i = 0, i < len(x), i = i + 1
			if x[i] != y[i]
				return x[i] - y[i]
			end
		end
	else
		for var i = 0, i < len(y), i = i + 1
			if x[i] != y[i]
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
	if (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') return true end
	return false
end

func isdigit(c : number)
	if (c >= '0' && c <= '9') return true end
	return false
end

func isalnum(c : number)
	if isalpha(c) || isdigit(c) return true end
	return false
end

func isspace(c : number)
	if c == '\n' || c == '\t' || c == ' ' return true end
	return false
end

func stream(...)
	var args = getargs()
	var raw = ""
	if len(args) > 0
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
	if self.pos >= len(self._raw)
		return EOF
	end
	
	var c = self._raw[self.pos]
	self.pos = self.pos + 1
	
	return c
end

func _stream_write(self, c)
	if type(c) == numbertype
		if self.pos < len(self._raw)
			self._raw[self.pos] = c
		else
			push(self._raw, c)
		end
	
		self.pos = self.pos + 1
	elif type(c) == stringtype || type(c) == arraytype
		for var i = 0, i < len(c), i = i + 1
			_stream_write(self, c[i])
		end
	end

end

func _stream_write_byte(self, b)
	if self.pos < len(self._raw)
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
	for var i = 0, i < len(array), i = i + 1
		_stream_write_byte(self, array[i])
	end
end

func _stream_close(self)
	if self._file_handle != null && self._mode[0] == 'w'
		for var i = 0, i < len(self._raw), i = i + 1
			putc(self._file_handle, self._raw[i])
		end
	end
end

func fstream(path : string, mode : string)
	var f = fopen(path, mode)
	var buf = []
	
	if mode[0] == 'r'
		var c = getc(f)
		
		while c != EOF
			push(buf, c)
			c = getc(f)
		end
	end
	
	var s = stream(joinchars(buf))
	s._file_handle = f
	s._mode = mode
	return s
end

func byte_array(n : number)
	return { _raw = bytes(n), length = n, SETINDEX = _byte_array_set_index, GETINDEX = _byte_array_get_index, LENGTH = _byte_array_get_length }
end

func byte_array_from_raw(b : native)
	return { _raw = b, length = lenbytes(b), SETINDEX = _byte_array_set_index, GETINDEX = _byte_array_get_index, LENGTH = _byte_array_get_length }
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
	
	if !hasellipsis(f)
		if len(args) != getnumargs(f)
			printf("%o expects %g arguments but you were going to pass %g arguments upon exit\n", f, getnumargs(f), len(args)) 
			exit(1)
		end
	else
		if len(args) < getnumargs(f)
			printf("%o expects at least %g arguments but you were going to pass %g arguments upon exit\n", f, getnumargs(f), len(args))
			exit(1)
		end
	end

	push(on_exit, [f, args])
end

func exit(result : number)
	for var i = 0, i < len(on_exit), i = i + 1
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

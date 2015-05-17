extern joinchars
extern printf
extern floor
extern halt
extern tostring
extern erase
extern type

var std

func _std_printf(format, args)
	printf("%s", _std_mprintf(format, args))
end

func _std_mprintf_add_chars(buf, string)
	for var i = 0, i < len(string), i += 1
		push(buf, string[i])
	end
end

func _std_mprintf_tostring(v)
	var t = type(v)
	if t == "string"
		return v
	elif t == "dict"
		var buf = []
		var p = v.pairs
		
		push(buf, '{')
		push(buf, ' ')
		
		for var i = 0, i < len(p), i += 1
			_std_mprintf_add_chars(buf, p[i][0])
			push(buf, ' ')
			push(buf, '=')
			push(buf, ' ')
			_std_mprintf_add_chars(buf, _std_mprintf_tostring(p[i][1]))
			
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
		for var i = 0, i < len(v), i += 1
			_std_mprintf_add_chars(buf, _std_mprintf_tostring(v[i]))
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

func _std_mprintf(format, args)
	var buf = []
	var arg = 0
	
	for var i = 0, i < len(format), i += 1
		if format[i] == '%'
			i += 1
			if arg >= len(args)
				printf("mprintf format overflow\n")
				return joinchars(buf)
			end
			
			if format[i] == 'c'
				push(buf, args[arg])
			elif format[i] == 'g'
				_std_mprintf_add_chars(buf, tostring(args[arg]))
			elif format[i] == 's'
				_std_mprintf_add_chars(buf, args[arg])
			elif format[i] == 'o'
				_std_mprintf_add_chars(buf, _std_mprintf_tostring(args[arg]))
			else
				printf("invalid mprintf format specifier '%c'\n", format[i])
				return joinchars(buf)
			end
			arg += 1
		else
			push(buf, format[i])
		end
	end
	
	return joinchars(buf)
end

func _std_sort(v, comp)
	for var i = 0, i < len(v), i += 1
		for var j = 0, j < len(v) - 1, j += 1
			var c = comp(v[j], v[j + 1])
			
			if c > 0
				var temp = v[j + 1]
				v[j + 1] = v[j]
				v[j] = temp
			end
		end
	end
end

func _main()
	std = {
		printf = _std_printf,
		mprintf = _std_mprintf,
		sort = _std_sort
	}
	
	return main()
end

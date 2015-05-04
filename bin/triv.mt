extern printf
extern strcat
extern tonumber
extern tostring
extern len
extern type
extern assert

var number
var string
var array

func typestring(t)
	if t == number return "number"
	elif t == string return "string"
	elif t == array return "array"
	else return "native pointer" end
end

func join(a, sep)
	assert(type(a) == array, "Expected array as first argument to 'join'")
	assert(type(sep) == string, "Expected string as seperator argument to 'join'")

	var str = "{"
	for var i = 0, i < len(a), i = i + 1
		var v = a[i]
		if type(v) == number 
			str = strcat(str, tostring(v))
		elif type(v) == string
			str = strcat(str, "'")
			str = strcat(str, v)
			str = strcat(str, "'")
		elif type(v) == array
			str = strcat(str, join(v, sep))
		end
		
		if i + 1 < len(a)
			str = strcat(str, sep)
		end
	end
	
	str = strcat(str, "}")
	return str
end

func _main()
	number = type(0)
	string = type("")
	array = type([0])
	
	return 0
end

# operator.mt -- compile time operator overloading

extern joinchars(array-number) : string
extern arraycopy(array, array, number, number, number) : void

struct char
	code : number
end

operator !(c : char)
	return joinchars([c.code, 0])
end

struct stream
	buffer : array-number
	to_string : function(stream)-string
end

func char(code : number)
	return { code = code } as char
end

func stream()
	return { buffer = [], to_string = func(s)
		var buf = s.buffer
		var temp = array(len(buf))
		arraycopy(temp, buf, 0, 0, len(buf))
		push(temp, 0)
		return joinchars(temp)
	end } as stream
end

operator <<(a : stream, b : char)
	push(a.buffer, b.code)
	return a
end

operator <<(a : stream, b : string)
	var buf = a.buffer
	for var i = 0, i < len(b), i = i + 1 do
		push(buf, b[i])
	end
	return a
end

operator [](a : stream, b : number)
	return char(a.buffer[b])
end

operator [](a : stream, b : string)
	write("indexing stream by string: " .. b)
	return "nothing"
end

func run()
	var s = stream()
	s << "hello world" << char('!')
	
	write(s:to_string())
	
	# calls [] operator on stream with number as argument, and then calls the ! unary operator on char to get the string
	write(!s[0])
	write(s["hello world"])
end

run()

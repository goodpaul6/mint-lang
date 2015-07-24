# edge.mt -- testing cutting edge features in mint-lang

func parse_args(s)
	var args = []
	
	var c = s:read()
	
	while isspace(c)
		c = s:read()
	end
	
	while c != EOF
		if isdigit(c)
			var buf = []
			while isdigit(c) || c == '.'
				push(buf, c)
				c = s:read()
			end
			
			push(args, tonumber(joinchars(buf)))
		end
		
		if c == '"'
			var buf = []
			while c != '"'
				push(buf, c)
				c = s:read()
			end
			s:read()
		
			push(args, joinchars(buf))
		end
		
		if c == ','
			c = s:read()
		end
	end
	
	return args
end

func add(x, y)
	return x + y
end

func main()
	var function_name = read()
	var function_args = parse_args(stream(read()))
	
	var function = getfuncbyname(function_name)
	if function != null
		write(function(expand(function_args, getnumargs(function))))
	end
	
	return 0
end

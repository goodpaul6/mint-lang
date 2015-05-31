extern tonumber

var last_char

var tok_ident
var tok_number
var tok_string
var tok_eof
var tok_end

var lexeme

var cur_tok

func lex(s)
	while std.isspace(last_char)
		last_char = s:read()
	end
	
	if std.isalpha(last_char)
		var buf = []
		
		while std.isalnum(last_char) || last_char == '_'
			push(buf, last_char)
			last_char = s:read()
		end
		
		lexeme = joinchars(buf)
		
		if lexeme == "end" return tok_end end
		
		return tok_ident
	elif std.isdigit(last_char)
		var buf = []
		
		while std.isdigit(last_char) || last_char == '.'
			push(buf, last_char)
			last_char = s:read()
		end
		
		lexeme = tonumber(joinchars(buf))
		return tok_number
	elif last_char == '"'
		var buf = []
		last_char = s:read()
		
		while last_char != '"'
			push(buf, last_char)
			last_char = s:read()
		end
		
		last_char = s:read()
		
		lexeme = joinchars(buf)
		return tok_string
	elif last_char == std.eof
		return tok_eof
	end
	
	var l = last_char
	last_char = s:read()
	return l
end

func get_next_tok(s)
	cur_tok = lex(s)
	return cur_tok
end

func parse_factor(s)
	if cur_tok == tok_number
		var value = lexeme
		get_next_tok(s)
		
		return { type = "number", value = value }
	elif cur_tok == tok_string
		var value = lexeme
		get_next_tok(s)
		
		return { type = "string", value = value }
	elif cur_tok == tok_ident
		if lexeme == "print"
			get_next_tok(s)
			var e = parse_expr(s)
			
			return { type = "print", expr = e }
		elif lexeme == "func"
			get_next_tok(s)
			
			assert(cur_tok == tok_ident, "expected identifier after 'func'")
			var name = lexeme
			
			get_next_tok(s)
			
			assert(cur_tok == '(', "expected '(' after func id")
			get_next_tok(s)
			
			var args = []
			while cur_tok != ')'
				assert(cur_tok == tok_ident, "expected identifier in function argument list")
				push(args, lexeme)
				get_next_tok(s)
				if cur_tok == ',' get_next_tok(s) end
			end
			get_next_tok(s)
			
			var body = []
			while cur_tok != tok_end
				push(body, parse_expr(s))
			end
			get_next_tok(s)
			
			return { type = "func", args = args, body = body }
		else
			var id = lexeme
			get_next_tok(s)
		
			return { type = "ident", name = id }
		end
	elif cur_tok == '('
		get_next_tok(s)
		var e = parse_expr(s)
		if cur_tok != ')'
			printf("Expected ')' after previous '('\n")
			std.exit()
		end
		get_next_tok(s)
		
		return { type = "paren", expr = e }
	end
	
	printf("Unexpected token %g, %c\n", cur_tok, cur_tok)
	std.exit()
end

func get_token_prec()
	if cur_tok == '*' || cur_tok == '/' || cur_tok == '#' return 2
	elif cur_tok == '+' || cur_tok == '-' return 1
	elif cur_tok == '=' return 0 end
	
	return -1
end

func parse_bin_rhs(s, eprec, lhs)
	while true
		var prec = get_token_prec()
		
		if prec < eprec
			return lhs
		end
		
		var op = cur_tok
		
		get_next_tok(s)
		
		var rhs = parse_factor(s)
		var next_prec = get_token_prec()
		
		if next_prec > prec
			rhs = parse_bin_rhs(s, prec + 1, rhs)
		end
		
		var newLhs = { type = "bin", lhs = lhs, rhs = rhs, op = op }
		lhs = newLhs
	end
end

func parse_expr(s)
	var lhs = parse_factor(s)
	return parse_bin_rhs(s, 0, lhs)
end

func write_expr(e)
	if e.type == "number"
		printf("%g", e.value)
	elif e.type == "string"
		printf("%s", e.value)
	elif e.type == "ident"
		printf("%s", e.name)
	elif e.type == "bin"
		write_expr(e.lhs)
		printf("%c", e.op)
		write_expr(e.rhs)
	elif e.type == "paren"
		printf("(")
		write_expr(e.expr)
		printf(")")
	end
end

func compile(e)
	if e.type == "number" || e.type == "string"
		push(vm_code, op_push)
		push(vm_code, e.value)
	elif e.type == "ident"
		push(vm_code, op_get)
		push(vm_code, e.name)
	elif e.type == "print"
		compile(e.expr)
		push(vm_code, op_write)
	elif e.type == "func"
		for var i = 0, i < len(e.body), i += 1
			compile(e.body[i])
		end
	elif e.type == "bin"
		compile(e.rhs)
		if e.op == '='
			if e.lhs.type != "ident"
				printf("left hand side of assignment operator must be an identifier\n")
				std.exit()
			end
			
			push(vm_code, op_set)
			push(vm_code, e.lhs.name)
		else
			compile(e.lhs)
			
			if e.op == '+'
				push(vm_code, op_add)
			elif e.op == '-'
				push(vm_code, op_sub)
			elif e.op == '*'
				push(vm_code, op_mul)
			elif e.op == '/'
				push(vm_code, op_div)
			elif e.op == '#'
				push(vm_code, op_cat)
			end
		end
	elif e.type == "paren"
		compile(e.expr)
	end
end

func main()
	last_char = ' '
	
	tok_ident = -1
	tok_number = -2
	tok_string = -3
	tok_eof = -4
	tok_end = -5
	
	lexeme = ""
	
	cur_tok = 0

	var s = std.fstream("boot.mt", "r")
	
	vm_reset()
	
	get_next_tok(s)
	while cur_tok != tok_eof
		compile(parse_expr(s))
	end
	push(vm_code, op_halt)
	
	vm_ip = 0
	
	while vm_ip != -1
		vm_cycle()
	end
	
	return 0
end



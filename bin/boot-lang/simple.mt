extern tonumber

var last_char

var tok_ident
var tok_number
var tok_string
var tok_eof
var tok_end
var tok_else
var tok_elif

var lexeme

var cur_tok

func lex(s)
	while isspace(last_char)
		last_char = s:read()
	end
	
	if isalpha(last_char)
		var buf = []
		
		while isalnum(last_char) || last_char == '_'
			push(buf, last_char)
			last_char = s:read()
		end
		
		lexeme = joinchars(buf)
		
		if lexeme == "end" return tok_end end
		
		return tok_ident
	elif isdigit(last_char)
		var buf = []
		
		while isdigit(last_char) || last_char == '.'
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
	elif last_char == EOF
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

func parse_if(s)
	get_next_tok(s)
	
	var cond = parse_expr(s)
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
		elif lexeme == "return"
			get_next_tok(s)
			if cur_tok != ';'
				var e = parse_expr(s)
				return { type = "retval", expr = e }
			end
			get_next_tok(s)
			return { type = "ret" }
		elif lexeme == "len"
			get_next_tok(s)
			
			var e = parse_expr(s)
			
			return { type = "len", expr = e }
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
			
			return { type = "func", name = name, args = args, body = body }
		else
			var id = lexeme
			get_next_tok(s)
		
			return { type = "ident", name = id }
		end
	elif cur_tok == '['
		get_next_tok(s)
		var values = []
		var i = 0
		while cur_tok != ']'
			var e = parse_expr(s)
			if cur_tok == ','
				get_next_tok(s)
			end
			push(values, e)
			i = i + 1
		end
		get_next_tok(s)
		
		return { type = "array_literal", values = values }
	elif cur_tok == '('
		get_next_tok(s)
		var e = parse_expr(s)
		if cur_tok != ')'
			printf("Expected ')' after previous '('\n")
			exit(1)
		end
		get_next_tok(s)
		
		return { type = "paren", expr = e }
	end
	
	printf("Unexpected token %g, %c\n", cur_tok, cur_tok)
	exit(1)
end

func parse_post(s, pre)
	if cur_tok == '('
		get_next_tok(s)
		
		var args = []
		while cur_tok != ')'
			push(args, parse_expr(s))
			if cur_tok == ','
				get_next_tok(s)
			end
		end
		get_next_tok(s)
		
		return parse_post(s, { type = "call", efunc = pre, args = args })
	elif cur_tok == '['
		get_next_tok(s)
		
		var index = parse_expr(s)
		assert(cur_tok == ']', "expected ']' after previous '['\n")
		get_next_tok(s)
		
		return parse_post(s, { type = "array_index", arr = pre, index = index })
	else
		return pre
	end
end

func parse_unary(s)
	var e = parse_factor(s)
	return parse_post(s, e)
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
		
		var rhs = parse_unary(s)
		var next_prec = get_token_prec()
		
		if next_prec > prec
			rhs = parse_bin_rhs(s, prec + 1, rhs)
		end
		
		var newLhs = { type = "bin", lhs = lhs, rhs = rhs, op = op }
		lhs = newLhs
	end
end

func parse_expr(s)
	var lhs = parse_unary(s)
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

func compile(e, code)
	if e.type == "number" || e.type == "string"
		push(code, op_push)
		push(code, e.value)
	elif e.type == "ident"
		push(code, op_get)
		push(code, e.name)
	elif e.type == "print"
		compile(e.expr, code)
		push(code, op_write)
	elif e.type == "func"
		push(code, op_pushfunc)
		var func_code = []
		for var i = 0, i < len(e.body), i = i + 1
			compile(e.body[i], func_code)
		end
		push(code, { arg_names = e.args, code = func_code })
		
		push(code, op_set)
		push(code, e.name)
	elif e.type == "ret"
		push(code, op_ret)
	elif e.type == "retval"
		compile(e.expr, code)
		push(code, op_retval)
	elif e.type == "len"
		compile(e.expr, code)
		push(code, op_len)
	elif e.type == "array_literal"
		push(code, op_createarray)
		if len(e.values) == 0
			push(code, [])
		else
			var values = []
			for var i = 0, i < len(e.values), i = i + 1
				var rcode = []
				compile(e.values[i], rcode)
				push(values, rcode)
			end
			push(code, values)
		end
	elif e.type == "array_index"
		compile(e.arr, code)
		compile(e.index, code)
		push(code, op_getindex)
	elif e.type == "bin"
		if e.op == '='
			if e.lhs.type == "ident"
				compile(e.rhs, code)
				push(code, op_set)
				push(code, e.lhs.name)
			elif e.lhs.type == "array_index"
				compile(e.lhs.arr, code)
				compile(e.lhs.index, code)
				compile(e.rhs, code)
				push(code, op_setindex)
			end
		else
			compile(e.rhs, code)
			compile(e.lhs, code)
			
			if e.op == '+'
				push(code, op_add)
			elif e.op == '-'
				push(code, op_sub)
			elif e.op == '*'
				push(code, op_mul)
			elif e.op == '/'
				push(code, op_div)
			elif e.op == '#'
				push(code, op_cat)
			end
		end
	elif e.type == "paren"
		compile(e.expr, code)
	elif e.type == "call"
		compile(e.efunc, code)
		push(code, op_call)
		var args = []
		for var i = 0, i < len(e.args), i = i + 1
			var acode = []
			compile(e.args[i], acode)
			push(args, acode)
		end
		if len(e.args) == 0
			push(code, [])
		else
			push(code, args)
		end
	end
end

func main()
	last_char = ' '
	
	tok_ident = -1
	tok_number = -2
	tok_string = -3
	tok_eof = -4
	tok_end = -5
	tok_else = -6
	tok_elif = -7
	
	lexeme = ""
	
	cur_tok = 0

	var s = fstream("boot.mt", "r")
	
	vm_reset()
	
	get_next_tok(s)
	while cur_tok != tok_eof
		compile(parse_expr(s), vm_code)
	end
	push(vm_code, op_halt)
	
	vm_ip = 0
	
	while vm_ip != -1
		vm_ip = vm_cycle(vm_code, vm_ip)
	end
	
	return 0
end



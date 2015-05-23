extern tonumber

var last_char

var tok_ident
var tok_number
var tok_string
var tok_eof
var tok_print

var lexeme

var cur_tok

var constants
var number_amount
var string_amount

var env

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
		
		if lexeme == "print" return tok_print end
		
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

func register_number(n)
	if constants[tostring(n)] != null
		return constants[tostring(n)]
	end

	constants[tostring(n)] = number_amount
	number_amount += 1
	return number_amount - 1
end

func register_string(s)
	if constants[s] != null
		return constants[s]
	end
	
	constants[s] = string_amount
	string_amount += 1
	return string_amount - 1
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
		var id = lexeme
		get_next_tok(s)
		
		return { type = "ident", name = id }
	elif cur_tok == '('
		get_next_tok(s)
		var e = parse_expr(s)
		if cur_tok != ')'
			std.printf("Expected ')' after previous '('\n")
			std.exit()
		end
		get_next_tok(s)
		
		return { type = "paren", expr = e }
	elif cur_tok == tok_print
		get_next_tok(s)
		var e = parse_expr(s)
		return { type = "print", expr = e }
	end
	
	std.printf("Unexpected token %g, %c\n", cur_tok, cur_tok)
	std.exit()
end

func get_token_prec()
	if cur_tok == '*' || cur_tok == '\\' || cur_tok == '#' return 2
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
	elif e.type == "print"
		printf("print ")
		write_expr(e.expr)
	end
end

func interpret(e)
	if e.type == "number"
		return e.value
	elif e.type == "string"
		return e.value
	elif e.type == "ident"
		var v = env[e.name]
		if v != null
			return v
		else
			return 0
		end
	elif e.type == "bin"
		if e.op == '='
			if e.lhs.type != "ident"
				printf("left hand side of '=' operator must be an identifier\n")
				std.exit()
			end
			
			var v = interpret(e.rhs)
			env[e.lhs.name] = v
			return v
		elif e.op == '+'
			return interpret(e.lhs) + interpret(e.rhs)
		elif e.op == '-'
			return interpret(e.lhs) - interpret(e.rhs)
		elif e.op == '*'
			return interpret(e.lhs) * interpret(e.rhs)
		elif e.op == '/'
			return interpret(e.lhs) / interpret(e.rhs)
		elif e.op == '#'
			return strcat(interpret(e.lhs), interpret(e.rhs))
		end
	elif e.type == "paren"
		return interpret(e.expr)
	elif e.type == "print"
		var v = interpret(e.expr)
		write(v)
		return v
	end
	return 0
end

func main()
	last_char = ' '
	
	tok_ident = -1
	tok_number = -2
	tok_string = -3
	tok_eof = -4
	tok_print = -5
	
	lexeme = ""
	
	cur_tok = 0
	
	constants = {}
	number_amount = 0
	string_amount = 0
	
	env = {}
	
	var s = std.fstream("boot.mt", "r")
	
	get_next_tok(s)
	while cur_tok != tok_eof
		interpret(parse_expr(s))
	end
	
	return 0
end



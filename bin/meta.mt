# meta.mt -- used by the compiler to create asts (accessible at runtime by the mint program) from program code

#~ potential compiler code
func try_operation(exp)
	if exp.type == _exp_number return exp.number
	elif exp.type == _exp_string return exp.string
	elif exp.type == _exp_call 
		for var i = 0, i < len(exp.args), i = i + 1
			exp.args[i] = optimized(exp.args[i])
		end
		return null
	elif exp.type == _exp_bin 
		var lhs = try_operation(exp.lhs)
		var rhs = try_operation(exp.rhs)
		
		if (lhs == null || rhs == null)
			return null
		end
		
		if type(lhs) == stringtype 
			return strcat(lhs, rhs)
		end
	
		if exp.op == '+'
			return lhs + rhs
		elif exp.op == '-'
			return lhs - rhs
		elif exp.op == '*'
			return lhs * rhs
		elif exp.op == '/'
			return lhs / rhs
		end
		return null
	end
end


func optimized(exp)
	var result = try_operation(exp)
	if result != null
		if type(result) == numbertype return _number_expr(result)
		elif type(result) == stringtype return _string_expr(result) end
	else return exp end
end

func register_unique(v, values)	
	for var i = 0, i < len(values), i = i + 1
		if values[i] == v
			return i
		end
	end
	
	push(values, v)
	return len(values) - 1
end

func append_int(n, ctx)
	var b = number_to_byte_array(n, 4)
	for var i = 0, i < 4, i = i + 1
		push(ctx.code, b[i])
	end
end

func compile(exp, ctx)
	if exp.type == _exp_number 
		push(ctx.code, _op_push_number)
		append_int(register_unique(exp.number, ctx.numbers), ctx)
	elif exp.type == _exp_string
		push(ctx.code, _op_push_string)
		append_int(register_unique(exp.string, ctx.strings), ctx)
	elif exp.type == _exp_paren
		compile(exp.expr, ctx) 
	elif exp.type == _exp_call
		if exp.fexp.type == _exp_ident
			if exp.fexp.id == "write"
				for var i = 0, i < len(exp.args), i = i + 1
					compile(exp.args[i], ctx)
					push(ctx.code, _op_write)
				end
			else
				printf("invalid function name %s\n", exp.fexp.id)
				exit(1)
			end
		end
	elif exp.type == _exp_bin
		compile(exp.rhs, ctx)
		compile(exp.lhs, ctx)
		
		if exp.op == '+' push(ctx.code, _op_add)
		elif exp.op == '-' push(ctx.code, _op_sub)
		elif exp.op == '*' push(ctx.code, _op_mul)
		elif exp.op == '/' push(ctx.code, _op_div) end
	end
end

func output_code(s, ctx)
	s:write_binary_value(0, nb_s32)							# entry point
	s:write_binary_value(len(ctx.code), nb_s32)				# program length 
	s:write_bytes(ctx.code)									# program code
	s:write_binary_value(0, nb_s32)							# num globals
	s:write_binary_value(0, nb_s32)							# num functions
	s:write_binary_value(0, nb_s32)							# num externs
	
	s:write_binary_value(len(ctx.numbers), nb_s32)			# num numbers
	
	for var i = 0, i < len(ctx.numbers), i = i + 1
		s:write_binary_value(ctx.numbers[i], nb_double)		# number constant
	end
	
	s:write_binary_value(len(ctx.strings), nb_s32)			# num strings
	
	for var i = 0, i < len(ctx.strings), i = i + 1
		s:write_binary_value(len(ctx.strings[i]), nb_s32)	# string length
		s:write_bytes(ctx.strings[i])						# string data
	end
	
	s:write_binary_value(0, nb_s8)							# has debug metadata
end
~#

#~_OP_ADD = 18,
OP_SUB,
OP_MUL,
OP_DIV,
OP_MOD,
OP_OR,
OP_AND,
OP_LT,
OP_LTE,
OP_GT,
OP_GTE,
OP_EQU,
OP_NEQU,
OP_NEG,
OP_LOGICAL_NOT,
OP_LOGICAL_AND,
OP_LOGICAL_OR,
OP_SHL,
OP_SHR,~#

var _op_push_null
var _op_push_number
var _op_push_string
var _op_write

var _op_add
var _op_sub
var _op_mul
var _op_div
var _op_halt

var _exp_unknown
var _exp_number
var _exp_string
var _exp_ident
var _exp_call
var _exp_var
var _exp_bin
var _exp_paren
var _exp_while
var _exp_func
var _exp_if
var _exp_return
var _exp_extern
var _exp_array_literal
var _exp_array_index
var _exp_unary
var _exp_for
var _exp_dot
var _exp_dict_literal
var _exp_null
var _exp_continue
var _exp_break
var _exp_colon

# used by the compiler to see if the meta.mt module has been included in the program
# and to initialize the module
func __meta_mt()
	_op_push_null = 0
	_op_push_number = 1
	_op_push_string = 2
	_op_write = 41
	_op_add = 18
	_op_sub = 19
	_op_mul = 20
	_op_div = 21
	_op_halt = 52

	_exp_unknown = -1
	_exp_number = 0
	_exp_string = 1
	_exp_ident = 2
	_exp_call = 3
	_exp_var = 4
	_exp_bin = 5
	_exp_paren = 6
	_exp_while = 7
	_exp_func = 8
	_exp_if = 9
	_exp_return = 10
	_exp_extern = 11
	_exp_array_literal = 12
	_exp_array_index = 13
	_exp_unary = 14
	_exp_for = 15
	_exp_dot = 16
	_exp_dict_literal = 17
	_exp_null = 18
	_exp_continue = 19
	_exp_break = 20
	_exp_colon = 21
end

func _number_expr(n)
	return {
		type = _exp_number,
		number = n
	}
end

func _string_expr(s)
	return {
		type = _exp_string,
		string = s
	}
end

func _ident_expr(i)
	return {
		type = _exp_ident,
		id = i
	}
end

func _paren_expr(e)
	return {
		type = _exp_paren,
		expr = e
	}
end

func _binary_expr(l, r, op)
	return {
		type = _exp_bin,
		lhs = l, rhs = r,
		op = op
	}
end

func _call_expr(f, ...)
	return {
		type = _exp_call,
		fexp = f,
		args = getargs(f)
	}
end

func _array_literal_expr(...)
	return {
		type = _exp_array_literal,
		values = getargs()
	}
end

func _unknown_expr()
	return {
		type = _exp_unknown
	}
end


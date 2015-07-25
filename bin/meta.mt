# meta.mt -- metaprogramming essentials for mint-lang

var exp_number
var exp_string
var exp_ident
var exp_call
var exp_var
var exp_bin
var exp_paren
var exp_while
var exp_func
var exp_if
var exp_return
var exp_extern
var exp_array_literal
var exp_array_index
var exp_unary
var exp_for
var exp_dot
var exp_dict_literal
var exp_null
var exp_continue
var exp_break
var exp_colon

func number_expr(n)
	return {
		type = exp_number,
		number = n
	}
end

func string_expr(s)
	return {
		type = exp_string,
		string = s
	}
end

func ident_expr(id)
	return {
		type = exp_ident,
		
	}
end


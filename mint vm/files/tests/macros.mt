# macros.mt -- test macros

extern macro_multi
extern macro_declare_variable
extern macro_num_expr
extern macro_ident_expr
extern macro_bin_expr

macro fact(n)
	if n < 2 then return macro_num_expr(1)
	else return macro_bin_expr("*", macro_num_expr(n), fact(n - 1)) end
end

macro enum(...)
	var args = getargs()
	
	var gen = []
	for var i = 0, i < len(args), i = i + 1 do
		var decl = macro_declare_variable(args[i])
		write(i)
		push(gen, macro_bin_expr("=", macro_ident_expr(decl), macro_num_expr(i)))
	end
	
	return macro_multi(gen)
end

macro test()
	return macro_bin_expr("=", macro_ident_expr(macro_declare_variable("x")), macro_num_expr(1))
end

enum!("hello", "world", "good")
write(good)

# edge.mt -- testing cutting edge features in mint-lang

extern printf
extern getnumargs
extern assert
extern type

func bind(f, has_return, ...)
	return {
		has_return = has_return
		bound = f,
		args = args,
		CALL = _call_bound
	}
end

func _call_bound(b, unbound_args)
	if b.has_return
		return b.bound(expand(b.args, len(b.args)), expand(unbound_args, getnumargs(b.bound) - len(b.args)))
	else
		b.bound(expand(b.args, len(b.args)), expand(unbound_args, getnumargs(b.bound) - len(b.args)))
	end
end

func a(...)
	return args
end

func add(x, y)
	return x + y
end

func _main()
	var add_to_10 = bind(add, true, 10)
	write(add_to_10(a(20)))
	
	return 0
end

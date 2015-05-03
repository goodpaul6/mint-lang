# mint-lang
A programming language. Compiles to bytecode, useful for games.

Example:
'''
extern print

func fact(n)
	if n < 2 return 1 end
	return n * fact(n - 1)
end

func _main()
	print(fact(5))
end
'''

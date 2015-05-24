extern clock
extern getclockspersec

func fact(n)
	if n < 2 return 1 end
	return n * fact(n - 1)
end

func _main()
	var start = clock()
	write(fact(5))
	write((((clock() - start) * 1000) / getclockspersec()) / 1000)
	return 0
end

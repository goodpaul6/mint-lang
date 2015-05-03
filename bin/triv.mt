extern print
extern push
extern pop
extern len

var arr

func arith(n)
	return 10 + (n - 1) * 5
end

func _main()
	arr = [0]

	for var i = 1, i <= 100, i = i + 1
		print(arith(i))
		push(arr, arith(i))
	end
	for true, len(arr) > 0, print(pop(arr)) end
	
	return 0
end

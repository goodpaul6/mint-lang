# edge.mt -- testing cutting edge features in mint-lang

extern tonumber(dynamic) : number
extern floor(number) : number
extern sqrt(number) : number


func abs(x : number)
	if x < 0 return -x end
	return x
end

func msqrt(n : number)
	var a = n
	var lim = floor(n)
	var x = 1
	while abs((x * x) - n) > 0.001 
		x = 0.5 * (x + (a / x))
	end
	
	return x
end

func test()
	return test
end

func _main()
	write(typename(test()))
		
	return 0
end

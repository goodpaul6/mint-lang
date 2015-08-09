# edge.mt -- testing cutting edge features in mint-lang

func add(x : number, y : number, ...)
	var sum = x + y
	var args = getargs(y)
	
	for var i = 0, i < len(args), i = i + 1
		sum = sum + args[i]
	end
	
	return sum
end

func main()
	var x = 10
	var y = 20
	
	write(add(x, y))
	
	return 0
end

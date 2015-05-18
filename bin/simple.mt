func test(...)
	for var i = 0, i < len(args), i += 1
		write(args[i])
	end
end

func test2(x, y, ...)
	write(x + y)
	write(len(args))
end

func main()
	return 0
end

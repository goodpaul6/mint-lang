func test(x, y)
	write(x)
	write(y)
end


func _main()
	var arr = [2]
	arr[0] = 10
	arr[1] = 20
	
	test(expand(arr))

	call(@test, expand(arr))

	return 0
end

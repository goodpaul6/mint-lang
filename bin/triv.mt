func test(x, y)
	write(x)
	write(y)
end

func test_expander(a)
	test(a[0], a[1])
end

func _main()
	var arr = [2]
	arr[0] = 10
	arr[1] = 20

	call(@test, expand(arr))

	return 0
end

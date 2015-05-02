extern print

func _main()
	var arr = [5]
	arr[0] = [2]
	arr[1] = arr[0]
	arr[2] = arr[1]
	arr[3] = arr[2]
	arr[4] = arr[3]
	arr[4][0] = 2

	if 10 > 20
		print("1")
	elif 20 > 30
		print("2")
	elif 30 > 40
		print("3")
	else
		print(arr[4][0])
		print(arr[3][0])
	end
end

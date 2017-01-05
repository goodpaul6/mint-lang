# edge.mt -- testing cutting edge features in mint-lang

func _main()
	var x = [[10, 20], [20], [30]]
	var y = x[0]

	write(typename(y))

	write(y)
end
func _main()
	var l = list([10, 20, 30])
	
	for true, l.length > 0, write(l:pop())
	end
	
	return 0
end

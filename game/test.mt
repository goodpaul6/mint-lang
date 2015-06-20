func _main()
	var d = { x = 10 }
	setvmdebug(true)
	d.x = 20
	setvmdebug(false)
	
	return 0
end

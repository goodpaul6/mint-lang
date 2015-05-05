
func testp(x, y, z)
	write(x)
	write(y)
	write(z)
end

func _main()
	var i = @testp
	call(i, 10, 20, 30)
	return 0
end

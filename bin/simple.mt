func _main()
	for var i = 0, i < 10, i += 1
		if i == 4
			continue
		end
		if i == 5
			continue
		end
		write(i)
	end
	return 0
end

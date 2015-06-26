func test(array)
	func anon()
		func anon2(index)
			return array[index]
		end
		
		return anon2
	end
	
	return anon()
end


print test([10, 20, 30])(2)


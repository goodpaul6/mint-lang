func closure(array)
	func get_index(i)
		return array[i]
	end
	
	return get_index
end

x = closure([10, 20, 30])
print len [10, 20, 30]

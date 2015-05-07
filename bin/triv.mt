extern tostring
extern printf

func _object_equals(self, o)
	return self == o
end

func object()
	var o = dict()
	o["type"] = "object"
	o["equals"] = @_object_equals

	return o
end

func _number_equals(self, n)
	if n["type"] != "number"
		return _object_equals(self, n)
	end
	
	return self["value"] == n["value"]
end

func number(value)
	var n = object()
	
	n["type"] = "number"
	n["equals"] = @_number_equals
	n["value"] = value
	
	return n
end

func equals(o1, o2)
	return call(o1["equals"], o1, o2)
end

func _main()
	var n1 = number(10)
	var n2 = number(10)
	var n3 = object()
	
	if (equals(n1, n2)) write("true") else write("false") end
	if (equals(n1, n1))
	
	return 0
end

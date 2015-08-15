# edge.mt -- testing cutting edge features in mint-lang

extern printf(string) : void
extern tonumber(dynamic) : number
extern type(dynamic) : string

func each(a : function-void, ...)
	var array = getargs(a)
	for var i = 0, i < len(array), i = i + 1
		a(array[i])
	end
end

var list_mt = {
	GETINDEX = _list_get_index
	SETINDEX = _list_set_index
}

func list(...)
	var self = {
		values = getargs(),
		each = _list_each
	}

	setmeta(self, list_mt)

	return self
end

func _list_get_index(self, index)
	return rawget(self, "values")[index]
end

func _list_set_index(self, index, value)
	rawget(self, "values")[index] = value
end

func _list_each(self, a)
	var values = rawget(self, "values")
	for var i = 0, i < len(values), i = i + 1
		a(values[i])
	end
end

func record(exp)
	var members = getargs()
	var mem_map = {}
	
	for var i = 0, i < len(members), i = i + 1
		
	end
end

func main()
	var l = list(10, 20, 30)
	l:each(lambda (x) 
		write(x) 
	end)
	l:each(lambda(x)
		write(x * 2)
	end)
	
	return 0
end

main()

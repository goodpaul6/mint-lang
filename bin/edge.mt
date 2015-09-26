# edge.mt -- testing cutting edge features in mint-lang

extern tostring(dynamic) : string

trait printable
	print : function(dynamic)-void
end

type test is printable
	x : number
	print : function(test)-void
end

func test()
	var t = inst test
	
	t.x = 10
	t.print = func (self)
		write("hello world " .. tostring(self.x))
	end
	
	return t
end

(test() as printable):print()

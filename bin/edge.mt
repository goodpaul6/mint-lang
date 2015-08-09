# edge.mt -- testing cutting edge features in mint-lang

extern strcat(string, string) : string

func mprintf(format, ...)
	return "hello"
end

func _main()
	var test : function-number = mprintf

	return 0
end

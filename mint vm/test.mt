
func runMyFunc(f : function-void)
	f()
end

func run()
	var i = 10

	var f = lam ()
		write("hello world", i)
		i = i + 1
	end

	runMyFunc(f)
end

run()
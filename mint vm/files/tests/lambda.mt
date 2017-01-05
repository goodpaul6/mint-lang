# lambda.mt -- lambdas!

func run()
	var values = [0, 1, 2]
	var f = lam (i)
		write(values[i])
	end
	
	f(0)
	f(1)
	f(2)
end

run()

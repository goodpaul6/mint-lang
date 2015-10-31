# lambda.mt -- lambdas!

func run()
	var x = 10
	var f = lam ()
		return x * x
	end
	for var i = -3, i <= 3, i = i + 1 do
		x = i
		write(f())
	end
end

run()

# edge.mt -- testing cutting edge features in mint-lang

func run()
	var c = coroutine(lam (x)
		for var i = 0, i < x, i = i + 1 do
			yield(i)
		end
		
		return null
	end)

	while !c.is_done do
		write(c:call(10))
	end
end

run()

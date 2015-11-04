# coroutines.mt -- testing coroutines (cooperative threading)

func run()
	var x = 10
	var count10 = coroutine(lam (data)
		for var i = 0, i < x, i = i + 1 do
			yield(i)
		end

		return null
	end)

	while !count10.is_done do
		var result = count10:call(null)
		if result != null then
			write(result)
		else
			break
		end
	end
end

run()

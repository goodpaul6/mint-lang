# coroutines.mt -- testing coroutines (cooperative threading)

func run()
	var x = 10
	var count10 = coroutine(func (data)
		for var i = 0, i < 10, i = i + 1 do
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

# edge.mt -- testing cutting edge features in mint-lang

var total_iterations = 0

func counter(n : number)
	return thread(lam ()
        var x = n
		for var i = 1, i <= x, i = i + 1 do
            total_iterations = total_iterations + 1
			yield(i)
		end

		return;
	end)
end

func write_values(t : dynamic)
	while true do
		var val = run_thread(t)
		if is_thread_done(t) then
			break
		end

		write(val)
	end
end

func run()
	var x = counter(10)
	var y = counter(20)
	var z = counter(30000)

	write_values(x)
	write_values(y)
	write_values(z)

    write(total_iterations)
end

run()

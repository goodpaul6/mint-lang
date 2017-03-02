# edge.mt -- testing cutting edge features in mint-lang

extern typeof(dynamic) : string

func counter(n : number)
	return thread(lam ()
		for var i = 1, i <= n, i = i + 1 do
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

func _main()
	var x = counter(10)
	var y = counter(20)
	var z = counter(30)

	write_values(x)
	write_values(y)
	write_values(z)
end
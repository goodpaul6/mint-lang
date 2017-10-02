# edge.mt -- testing cutting edge features in mint-lang

extern sqrt(number) : number

var total_iterations = 0

struct vec2 {
    x : number
    y : number
}

operator [](a : vec2, n : number) {
    return [a.x, a.y][n]
}

operator ()(a : vec2) {
    return sqrt(a.x * a.x + a.y * a.y)
}

operator +(a : vec2, b : vec2) {
    return {
        x = a.x + b.x,
        y = a.y + b.y
    } as vec2
}

func counter(n : number) {
	return thread(lam () {
        var x = n
		for var i = 1, i <= x, i = i + 1 {
            total_iterations = total_iterations + 1
			yield(i)
		}

		return;
	})
}

func write_values(t : dynamic) {
	while true {
		var val = run_thread(t)
		if is_thread_done(t) {
			break
        }

		write(val)
	}
}

func run() {
    var a = { x = 10, y = 20 } as vec2
    var b = { x = 20, y = 10 } as vec2

    # This is all resolved at compile time
    # pretty cool eh.
    write(a + b)
    write(typename(a[0]))
    write(typename(a()))

	# var x = counter(10)
	# var y = counter(20)
	# var z = counter(5)

	# write_values(x)
	# write_values(y)
	# write_values(z)

    # write(total_iterations)
}

run()

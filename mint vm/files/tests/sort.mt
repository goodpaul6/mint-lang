extern arraysort(array, function-number) : void
extern srand() : void
extern rand() : void
extern tonumber(string) : number

func run()
	srand()
	write("how many?")
	var amt = tonumber(read())
	var a = for var i = 0, i < amt, i = i + 1 do rand() end
	write("sorting...")
	arraysort(a, func(a : number, b : number) return a - b end)
	write("sorted. How many indices should be displayed?")
	var n = tonumber(read())
	for var i = 0, i < n, i = i + 1 do write(a[i]) end
end

run()

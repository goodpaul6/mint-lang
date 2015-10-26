# lists.mt -- tests the high-level list functionality from common.mt

func run()
	var x = list(10, 20, 30)
	x:each(func (v) write(v) end)
	x:copy():each(func (v) write(-v) end)
end

run()

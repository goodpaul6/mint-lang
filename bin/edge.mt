# edge.mt -- testing cutting edge features in mint-lang

extern rand : number

func main()
	srand()
	var numbers = array(10)
	for var i = 0, i < len(numbers), i = i + 1
		numbers[i] = rand() % 100
	end
	
	write(typename(numbers))
	
	write(numbers)
	arraysort(numbers, numcmp)
	write(numbers)
	
	return 0
end

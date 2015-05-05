# mint-lang
A programming language. Compiles to bytecode, useful for games.

Example:
```
# fact.mt -- prints factorial of 5

func fact(n)
	if n < 2 return 1 end
	return n * fact(n - 1)
end

func _main()
	write(fact(5))
end
```
The syntax is fairly similar to that of Lua or Python. 
Here is a more complex example:
```
# types_and_arrays.mt -- type handling and arrays

extern type
extern assert

var number
var string
var array

func write_object(o)
	var ot = type(o)
	if ot == number
		write(tostring(o))
	elif ot == string
		write(o)
	elif ot == array
		write_array(o)
	end
end

func write_array(a)
	assert(type(a) == array, "Expected array as first argument to write_array")

	for var i = 0, i < len(a), i = i + 1
		write_object(a[i])
	end
end

func _main()
	number = type(0)
	string = type("")
	array = type([0])
	
	# creates array of length 4 and puts it in variable 'arr'
	var arr = [4]
	
	arr[0] = "hello"
	arr[1] = 10
	arr[2] = "world"
	arr[3] = [2]
	arr[3][0] = "goodbye"
	arr[3][1] = "world"
	
	write_array(arr)
end
```
'write' isn't designated as an extern because it is a compiler intrinsic; it compiles directly
to a bytecode instruction which writes the first argument to stdout.

The language also supports function pointers:
```
# writer.mt -- demonstrates function pointers

# printf is a standard library function (supports %s, %g, and %c)
extern printf

var writer

func write_string(s)
	printf("'%s'\n", s)
end

func write_number(g)
	printf("%g\n", g)
end

func _main()
	writer = @write_string
	
	# call is a compiler intrinsic which takes a variable number of 
    # arguments and passes them on to the function in
	# the function pointer
	call(writer, "hello world")

	writer = @write_number
	call(writer, 10)
end
```


# zlib License
Copyright (c) 2015 Apaar Madan

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgement in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

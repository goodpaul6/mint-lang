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
	return 0
end
```
The syntax is fairly similar to that of Lua or Python. 
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
	writer = write_string
	write("hello world")

	writer = write_number
	writer(10)
	
	return 0
end
```
There are also dictionaries (with similar functionality to tables in Lua, or dictionaries in python)
```
# dict.mt -- demonstrates dictionaries
func _main()
	var d = { x = 10, y = 20 }
	write(d.x)
	write(d.y)
	
	d.color = [255, 255, 255] # shorthand for d["color"], d.color is also faster than d["color"]
	
	write("d.color:")
	write(d.color[0])
	write(d.color[1])
	write(d.color[2])
	
	return 0
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

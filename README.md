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
The language also supports operator overloading. This is done using dictionaries with functions as members.
These functions correspond with the names of the operations.
```
# operator.mt -- demonstrates operator overloading

extern printf

# standard library extern, returns the type of the object as a string
extern type

# halts the vm (exits but frees all the allocated memory)
extern halt

func me(value)
	return {
		value = value,
		ADD = my_add 		# other supported operators: SUB, MUL, DIV, MOD, OR, AND, LOGICAL_AND, LOGICAL_OR,
							# 							 GETINDEX, SETINDEX
	}
end

func my_add(self, other)
	if type(other) == type(0) # if the value being added is a number
		
		# since the language doesn't support compound operators (with good reason)
		# we must return a new 'me' object with the new value
		return me(self.value + other) 
	
	else if type(other) == type({}) # or it is another 'me' object
		return me(self.value + other.value)
	else # or something else, in which case there is an error
		printf("attempted to add %s with 'me'\n", type(value))
		halt()
	end
end

func _main()
	var a = me(10)
	var b = me(20)
	
	# output: 30
	write((a + b).value)
	
	return 0
end
```
Here is a more practical usage of operator overloading: a generic hashmap implementation
which can use any type as a key as long as it's provided with a hash function:
```
# map.mt -- demonstrates a generic hashmap implementation

func map(f)
	return {
		buckets = array(1024),
		hash = f,
		SETINDEX = map_set_index,
		GETINDEX = map_get_index 
	}
end

func map_set_index(self, index, value)
	var hash = self.hash(index)
	
	var i = hash % len(self.buckets)
	var b = {
		next = self.buckets[i],
		key = index,
		value = value
	}

	self.buckets[i] = b
end

func map_get_index(self, index)
	var hash = self.hash(index)
	
	var bucket = self.buckets[hash % len(self.buckets)]
	while bucket.key != index
		bucket = bucket.next
		if bucket == null
			break
		end
		
		if bucket.key == index
			return bucket.value
		end
	end
	
	if bucket.key == index
		return bucket.value
	end
	
	return null
end

func inthash(i)
	return i
end

func _main()
	var imap = map(inthash)
	
	imap[0] = 0
	imap[1] = 10
	imap[2] = 20
	imap[3] = 30
	
	for var i = 0, i <= 3, i = i + 1
		write(imap[i])
	end
	
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

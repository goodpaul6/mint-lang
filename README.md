# mint-lang
A gradually-typed dynamic programming language. Compiles to bytecode, easily embedded within any native application.

Example:
```
# each.mt -- print values in an array using a fancy function
extern printf(string) : void

func each(values : array, f : function-void, ...)
	var extra_args = getargs(f) # an array of all arguments after 'f'

	for var i = 0, i < len(values), i = i + 1 do
		# call the function passing in the value
		# as well as the extra arguments to this function
		f(values[i], expand(extra_args, len(extra_args))) 
	end
end

func run()
	# array comprehension
	var values = for var i = 1, i <= 10, i = i + 1 do i end

	# lambdas :D
	each(values, lam (x, pre)
		printf("%s %g\n", pre, x)
	end, "number")
end

run()

```
The syntax is fairly similar to that of Lua or Python. 
'write' isn't designated as an extern because it is a compiler intrinsic; it compiles directly
to a bytecode instruction which writes the first argument to stdout.

Most of the abstractions in the language are
facilitated through dictionaries (with similar functionality to tables in Lua, or dictionaries in python)
```
# dict.mt -- demonstrates dictionaries
func run()
	var d = { x = 10, y = 20 }
	write(d.x)
	write(d.y)
	
	d.color = [255, 255, 255] # shorthand for d["color"]
	
	write("d.color:")
	write(d.color[0])
	write(d.color[1])
	write(d.color[2])
	
	return 0
end

run()
```
The language also supports operator overloading. This is done using "meta" dictionaries (similar to metatables in lua).
The functions in these metadicts correspond with the names of the operations.
```
# operator.mt -- demonstrates operator overloading

# takes a variable number of arguments, but the first one must be a string
extern printf(string) : void

# standard library extern, returns the type of the object as a string
extern type(dynamic) : string

# halts the vm (exits but frees all the allocated memory)
extern halt

var meta = {
	ADD = my_add			# other supported operators: SUB, MUL, DIV, MOD, OR, AND, LT, GT,
							#							 LTE, GTE, LOGICAL_AND, LOGICAL_OR,
							# 							 GETINDEX, SETINDEX, CALL, EQUALS,
							#							 LENGTH
}

func me(value)
	var self = {
		value = value
	}

	setmeta(self, meta)

	return self
end

func my_add(self, other)
	if type(other) == type(0) # if the value being added is a number
		return me(self.value + other)
	else if type(other) == type({}) # or it is another 'me' object
		return me(self.value + other.value)
	else # or something else, in which case there is an error
		printf("attempted to add %s with 'me'\n", type(value))
		halt()
	end
end

func run()
	var a = me(10)
	var b = me(20)
	
	# output: 30
	write((a + b).value)
	
	return 0
end

run()
```
Here is a more practical usage of operator overloading: a generic hashmap implementation
which can use any type as a key as long as it's provided with a hash function:
```
# map.mt -- demonstrates a generic hashmap implementation

var map_mt = {
	SETINDEX = map_set_index,
	GETINDEX = map_get_index
}

func map(f : function-number)
	var self = {
		buckets = array(1024),
		hash = f
	}

	setmeta(self, map_mt)

	return self
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
	while bucket != null
		if bucket.key == index
			return bucket.value
		else
			bucket = bucket.next
		end
	end
	
	return null
end

func numhash(i)
	return i
end

func run()
	var imap = map(numhash)
	
	imap[0] = 0
	imap[1] = 10
	imap[2] = 20
	imap[3] = 30
	
	for var i = 0, i <= 3, i = i + 1
		write(imap[i])
	end

	return 0
end

run()
```
Observe that even though GETINDEX is overloaded, the code is still able to 
access the 'buckets' and 'hash' keys inside the dictionary without recursively
calling the 'map_get_index' function. This is because GETINDEX is only invoked
when an index which does not exist on the dictionary is accessed. The same goes
for SETINDEX.

Of course, there are situations in which GETINDEX and SETINDEX must be bypassed,
in which case you should use 'rawget(dict, index)' and 'rawset(dict, index, value)'.

# Gradual typing
Mint is gradually typed: this means that the compiler is neither statically nor
dynamically typed. There is no need to declare any types, but this does not 
mean that the compiler has no type information.

There are a few advantages to doing this:

1. It is not necessary to specify types everywhere to have a compilable program
2. Type errors are still detected by the compiler whereever possible
3. It does not detract from the dynamic nature of the language

In short, many of the common type errors which most scripting languages
would pick up at runtime are recognized at compile time, but at little to 
no extra cost for the programmer. The errors which aren't detected at
compile time are still picked up at runtime (described and located).

All mint variables and values have one of the following types:

1. unknown: matches every type (inferred).
2. dynamic: matches every type (inferred and can be specified).
3. number: matches other numbers (inferred and can be specified).
4. string: matches other strings (inferred and can be specified).
5. array: this is the most broad array type (an array containing any type of value).
		  It is possible to specify array subtypes by using '-': array-array-number.
		  (inferred and can be specified).
6. dict: matches other dicts (inferred and can be specified).
7. function: this is the most broad function type.
			 It is possible to specify function return type using '-': 
			 function-string. (inferred and can be specified).
8. native: matches other native values (inferred and can be specified).
9. void: matches only void and unknown (inferred, functions only).

The following example shows the extent of mint's type inference:
```
# types.mt -- type inference and warnings
var x = 10
write(typename(x)) # output: number

var y = [10, 20, 30]
write(typename(y)) # output: array-number
write(typename(y[0])) # output: number

# the compiler selects the broadest type possible
# with the given information
var nested = [[10, 20, 30], [10, 20, 30]]
write(typename(nested)) # output: array-array-number
write(typename(nested[0])) # output : array-number

var nested_dyn = [[10, {}], ["", x]]
write(typename(nested_dyn)) # output: array-array-dynamic

# it can even infer array types from assignments
var valve_pls_infer = []
valve_pls_infer[0] = [10, 20, 30]

write(typename(valve_pls_infer)) # output: array-array-number

# dictionary accesses, unfortunately, are not statically typed
# considering the fact that they can contain any value
# mapped to indices which may or may not exist at compile
# time
var z = {}
write(typename(z)) # output: dict-dynamic
write(typename(z.x)) # output: dynamic

func add(x : number, y : number)
	return x + y
end

write(typename(add)) # output: number

# this is another case where type inference does not work
# this is because, contrary to what the text portrays,
# the type of x and y is unknown to the lambda as they 
# are members of the lambda's environment dictionary
# internally: env.x, env.y
func adder(x : number, y : number)
	return lam ()
		return x + y
	end
end

write(typename(adder)) # function-function:

# the problem described above can be circumvented by annotating the
# lambda's return type manually (like the cave people did)
func nicer_adder(x : number, y : number)
	return lam () : number
		return x + y
	end
end

write(typename(nicer_adder)) # output: function-function-number
```
Mint puts all the type information it can figure out to good use:
```
# type_errors.mt -- demonstrates compile time type checking

func add(x : number, y : number)
	return x + y
end

add(10, "hello") # Warning (type_errors.mt:6): Argument 2's type 'string-number' does not match the expected type 'number'

var x : dict = add(10, 20) # Warning (type_errors.mt:8): Initializing variable of type 'dict-dynamic' with value of type 'number'

var grades = [100, 98, 72, 84, 60]
write(grades.test) # Warning (type_errors.mt:11): Attempted to use dot '.' operator on value of type 'array-number'
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

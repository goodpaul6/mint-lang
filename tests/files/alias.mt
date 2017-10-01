# alias.mt -- testing extern aliasing
extern tostring as _tostring
extern tostring as __tostring

func run()
	# will not compile
	# write(tostring(10))
	# write(_tostring(10))
	
	write(__tostring(10))
end

run()

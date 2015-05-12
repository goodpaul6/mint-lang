extern printf
extern tostring
extern strcat

func _person_print(self)
	printf("name: %s, age: %g\n", self.name, self.age)
end

func mprintf(format, args)
	var s = ""
	var arg = 0
	
	for var i = 0, i < len(format), i += 1
		if format[i] == "%"
			i += 1
			if arg >= len(args)
				write("length of argument buffer to mprintf exceeded")
				return;
			end
			
			if format[i] == "g" 
				s = strcat(s, tostring(args[arg]))
				arg += 1
			elif format[i] == "s"
				s = strcat(s, args[arg])
				arg += 1
			end
		else
			s = strcat(s, format[i])
		end
	end
	
	return s
end

func _main()
	write(mprintf("%s.google.%s", ["www", "ca"]))
	return 0
end

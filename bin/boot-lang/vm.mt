# TODO:
# dicts

var op_push
var op_add
var op_sub
var op_mul
var op_div
var op_cat
var op_write
var op_get
var op_set
var op_halt
var op_pushenv
var op_popenv
var op_pushfunc
var op_call
var op_retval
var op_ret
var op_createarray
var op_getindex
var op_setindex
var op_len

var vm_code
var vm_stack
var vm_env
var vm_ip

func vm_reset()
	op_push = 0
	op_add = 1
	op_sub = 2
	op_mul = 3
	op_div = 4
	op_cat = 5
	op_write = 6
	op_get = 7
	op_set = 8
	op_halt = 9
	op_pushenv = 10
	op_popenv = 11
	op_pushfunc = 12
	op_call = 13
	op_retval = 14
	op_ret = 15
	op_createarray = 16
	op_getindex = 17
	op_setindex = 18
	op_len = 19
	
	vm_code = []
	vm_stack = []
	vm_env = { outer = null }
	vm_ip = -1
end

func dict_copy(dest, src)
	var pairs = src.pairs
	for var i = 0, i < len(pairs), i = i + 1
		dest[pairs[i][0]] = pairs[i][1]
	end
end

func vm_cycle(exec_code, current_ip)
	var code = exec_code
	var ip = current_ip
	
	var c = code[ip]
	
	if ip < 0 return; end
	
	if c == op_push
		ip = ip + 1
		push(vm_stack, code[ip])
		ip = ip + 1
	elif c == op_len
		ip = ip + 1
		var container = pop(vm_stack)
		push(vm_stack, len(container))
	elif c == op_createarray
		ip = ip + 1
		var values = code[ip]
		ip = ip + 1
		
		var arr = array(len(values))
		for var i = 0, i < len(values), i = i + 1
			 var aip = 0
			 while aip < len(values[i]) && aip >= 0
				aip = vm_cycle(values[i], aip)
			 end
			 arr[i] = pop(vm_stack)
		end
		push(vm_stack, arr)
	elif c == op_getindex
		ip = ip + 1
		
		var index = pop(vm_stack)
		var container = pop(vm_stack)
		
		push(vm_stack, container[index])
	elif c == op_setindex
		ip = ip + 1
		
		var value = pop(vm_stack)
		var index = pop(vm_stack)
		var container = pop(vm_stack)
		
		container[index] = value
	elif c == op_add
		ip = ip + 1
		var a = pop(vm_stack)
		var b = pop(vm_stack)
		push(vm_stack, a + b)
	elif c == op_sub
		ip = ip + 1
		var a = pop(vm_stack)
		var b = pop(vm_stack)
		push(vm_stack, a - b)
	elif c == op_mul
		ip = ip + 1
		var a = pop(vm_stack)
		var b = pop(vm_stack)
		push(vm_stack, a * b)
	elif c == op_div
		ip = ip + 1
		var a = pop(vm_stack)
		var b = pop(vm_stack)
		push(vm_stack, a / b)
	elif c == op_cat
		ip = ip + 1
		var a = pop(vm_stack)
		var b = pop(vm_stack)
		push(vm_stack, strcat(a, b))
	elif c == op_write
		ip = ip + 1
		var a = pop(vm_stack)
		write(a)
	elif c == op_get
		ip = ip + 1
		var id = code[ip]
		ip = ip + 1
		var env = vm_env
		var value = vm_env[id]
		while value == null
			env = env.outer
			if env == null
				break
			end
			value = env[id]
		end
		push(vm_stack, value)
	elif c == op_set
		ip = ip + 1
		var id = code[ip]
		ip = ip + 1
		vm_env[id] = pop(vm_stack)
	elif c == op_pushenv
		ip = ip + 1
		var new_env = { outer = vm_env }
		vm_env = new_env
	elif c == op_popenv
		if vm_env.outer != null
			vm_env = vm_env.outer
		end
	elif c == op_pushfunc
		ip = ip + 1
		var f = code[ip]
		ip = ip + 1
		f.env = vm_env
		push(vm_stack, f)
	elif c == op_ret
		ip = -1
	elif c == op_retval
		ip = -1
	elif c == op_call
		ip = ip + 1
		var args = code[ip]
		ip = ip + 1
		
		var f = pop(vm_stack)
		
		var new_env = { outer = vm_env }
		var pairs = f.env.pairs
		for var i = 0, i < len(pairs), i = i + 1
			if(pairs[i][0] != "outer")
				new_env[pairs[i][0]] = pairs[i][1]
			end
		end
		
		vm_env = new_env
		
		if len(args) != len(f.arg_names)
			write("invalid number of arguments in function call")
			exit(1)
		end
		
		for var i = 0, i < len(f.arg_names), i = i + 1
			var acode = args[i]
			var aip = 0
			
			while aip < len(acode) && aip >= 0
				aip = vm_cycle(acode, aip)
			end
			
			vm_env[f.arg_names[i]] = pop(vm_stack)
		end
		
		var fip = 0
		var fcode = f.code
		while fip < len(fcode) && fip >= 0
			fip = vm_cycle(fcode, fip)
		end
		
		vm_env = vm_env.outer
	elif c == op_halt
		ip = -1
	end
	
	return ip
end

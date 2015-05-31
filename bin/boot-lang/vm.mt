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
	
	vm_code = []
	vm_stack = []
	vm_env = { outer = null }
	vm_ip = -1
end

func vm_cycle()
	var c = vm_code[vm_ip]
	
	if vm_ip < 0 return; end
	
	if c == op_push
		vm_ip += 1
		push(vm_stack, vm_code[vm_ip])
		vm_ip += 1
	elif c == op_add
		vm_ip += 1
		var a = pop(vm_stack)
		var b = pop(vm_stack)
		push(vm_stack, a + b)
	elif c == op_sub
		vm_ip += 1
		var a = pop(vm_stack)
		var b = pop(vm_stack)
		push(vm_stack, a - b)
	elif c == op_mul
		vm_ip += 1
		var a = pop(vm_stack)
		var b = pop(vm_stack)
		push(vm_stack, a * b)
	elif c == op_div
		vm_ip += 1
		var a = pop(vm_stack)
		var b = pop(vm_stack)
		push(vm_stack, a / b)
	elif c == op_cat
		vm_ip += 1
		var a = pop(vm_stack)
		var b = pop(vm_stack)
		push(vm_stack, strcat(a, b))
	elif c == op_write
		vm_ip += 1
		var a = pop(vm_stack)
		write(a)
	elif c == op_get
		vm_ip += 1
		var id = vm_code[vm_ip]
		vm_ip += 1
		push(vm_stack, vm_env[id])
	elif c == op_set
		vm_ip += 1
		var id = vm_code[vm_ip]
		vm_ip += 1
		vm_env[id] = pop(vm_stack)
	elif c == op_pushenv
		vm_ip += 1
		var new_env = { outer = vm_env }
		vm_env = new_env
	elif c == op_popenv
		if vm_env.outer != null
			vm_env = vm_env.outer
		end
	elif c == op_halt
		vm_ip = -1
	end
end

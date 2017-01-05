# coroutine.mt -- makes using coroutines easier

extern thread() : native
extern start_thread(native, function(dynamic)-dynamic, dynamic) : void
extern resume_thread(native) : void
extern is_thread_complete(native) : number
extern get_thread_result(native) : dynamic
extern yield(dynamic) : void

struct coroutine
	handle : native
	callee : function(dynamic)-dynamic
	_is_started : number
	is_done : number
	call : function(coroutine, dynamic)-dynamic
end

func coroutine(callee : function(dynamic)-dynamic)
	return {
		handle = thread(),
		callee = callee,
		_is_started = false,
		is_done = false,
		call = coroutine_call
	} as coroutine
end

func coroutine_call(self : coroutine, arg : dynamic)
	if !self._is_started then
		start_thread(self.handle, self.callee, arg)
		self._is_started = 1
	else
		resume_thread(self.handle)
	end

	self.is_done = is_thread_complete(self.handle)

	return get_thread_result(self.handle)
end

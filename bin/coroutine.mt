# coroutine.mt -- makes using coroutines easier

extern thread() : native
extern start_thread(native, function(dynamic)-dynamic, dynamic) : void
extern resume_thread(native) : void
extern is_thread_complete(native) : number
extern get_thread_result(native) : dynamic
extern yield(dynamic) : void

type coroutine
	handle : native
	callee : function(dynamic)-dynamic
	_is_started : number
	is_done : number
	call : function(coroutine, dynamic)-dynamic
end

func coroutine(callee : function(dynamic)-dynamic)
	var c = inst coroutine
	c.handle = thread()
	c.callee = callee
	c._is_started = 0
	c.is_done = 0
	c.call = coroutine_call
	return c
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
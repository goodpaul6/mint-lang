# cffi.mt -- mint code used by the compiler for interfacing with c code
extern loadlib
extern getprocaddress

extern type

extern malloc
extern memcpy
extern free
extern getmem
extern setmem
extern sizeof
extern addressof
extern ataddress

extern dc_reset
extern dc_arg_bool
extern dc_arg_char
extern dc_arg_short
extern dc_arg_int
extern dc_arg_long
extern dc_arg_long_long
extern dc_arg_float
extern dc_arg_double
extern dc_arg_pointer

extern dc_call_void
extern dc_call_bool
extern dc_call_char
extern dc_call_short
extern dc_call_int
extern dc_call_long
extern dc_call_long_long
extern dc_call_float
extern dc_call_double
extern dc_call_pointer

extern dc_new_struct
extern dc_struct_field
extern dc_sub_struct
extern dc_close_struct
extern dc_struct_size
extern dc_struct_offset

var dc_sigchar_void
var dc_sigchar_bool
var dc_sigchar_char
var dc_sigchar_uchar
var dc_sigchar_short
var dc_sigchar_ushort
var dc_sigchar_int
var dc_sigchar_uint
var dc_sigchar_long
var dc_sigchar_ulong
var dc_sigchar_longlong
var dc_sigchar_ulonglong
var dc_sigchar_float
var dc_sigchar_double
var dc_sigchar_pointer
var dc_sigchar_string
var dc_sigchar_struct

func init_cffi()
	dc_sigchar_void = 'v'
	dc_sigchar_bool = 'B'
	dc_sigchar_char = 'c'
	dc_sigchar_uchar = 'C'
	dc_sigchar_short = 's'
	dc_sigchar_ushort = 'S'
	dc_sigchar_int = 'i'
	dc_sigchar_uint = 'I'
	dc_sigchar_long = 'j'
	dc_sigchar_ulong = 'J'
	dc_sigchar_longlong = 'l'
	dc_sigchar_ulonglong = 'L'
	dc_sigchar_float = 'f'
	dc_sigchar_double = 'd'
	dc_sigchar_pointer = 'p'
	dc_sigchar_string = 'Z'
	dc_sigchar_struct = 'T'
end

func type_from_sig(s)
	if s == dc_sigchar_void return null
	elif s == dc_sigchar_bool return c_uchar
	elif s == dc_sigchar_char return c_char
	elif s == dc_sigchar_uchar return c_uchar
	elif s == dc_sigchar_short return c_short
	elif s == dc_sigchar_ushort return c_ushort
	elif s == dc_sigchar_int return c_int
	elif s == dc_sigchar_uint return c_uint
	elif s == dc_sigchar_long return c_long
	elif s == dc_sigchar_ulong return c_ulong
	elif s == dc_sigchar_float return c_float
	elif s == dc_sigchar_double return c_double
	elif s == dc_sigchar_pointer return c_pointer
	elif s == dc_sigchar_string return c_pointer end
	return null
end

func cffi_struct(dc_struct, types, ...)
	var s = dc_struct
	var members = getargs(sig)
	var offset_map = {}
	
	for var i = 0, i < len(members), i = i + 1
		offset_map[members[i]] = [dc_struct_offset(s, i), types[i]]
	end
	
	return {
		size = dc_struct_size(s),
		members = members,
		offset_map = offset_map,
		ref = _create_struct_ref,
		CALL = _create_struct
	}
end

func _create_struct_ref(desc, ref)
	return {
		ptr = ref,
		_offset_map = desc.offset_map,
		GETINDEX = _struct_get_index,
		SETINDEX = _struct_set_index
	}
end

func _create_struct(desc, ...)
	var ptr = malloc(desc.size)
	var args = getargs(desc)
	
	for var i = 0, i < len(args), i = i + 1
		var data = desc.offset_map[desc.members[i]]
		var v = args[i]
		if type(v) == nativetype
			v = addressof(v)
		end
		
		setmem(ptr, data[0], data[1], v) 
	end
	
	return {
		ptr = ptr,
		_desc = desc,
		_offset_map = desc.offset_map,
		_refs = 1,
		retain = _struct_retain,
		release = _struct_release,
		free = _struct_free,
		copy = _struct_copy,
		GETINDEX = _struct_get_index,
		SETINDEX = _struct_set_index
	}
end

func _struct_copy(self)
	var copied = _create_struct(self._desc)
	memcpy(copied.ptr, self.ptr, self._desc.size)
	return copied
end

func _struct_get_index(self, index)
	var data = self._offset_map[index]
	return getmem(self.ptr, data[0], data[1])
end

func _struct_set_index(self, index, v)
	var data = self._offset_map[index]
	
	var value = v
	# cannot store pointers directly (must store pointer to pointer)
	if type(v) == nativetype
		value = addressof(v)
	end
	
	return setmem(self.ptr, data[0], data[1], value)
end

func _struct_retain(self)
	self._refs = self._refs + 1
end

func _struct_release(self)
	self._refs = self._refs - 1
	if self._refs <= 0 
		_struct_free(self)
	end
end

func _struct_free(self)
	free(self.ptr)
	self.ptr = null
end

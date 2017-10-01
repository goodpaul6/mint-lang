# cffi.mt -- mint code used by the compiler for interfacing with c code

extern loadlib
extern getprocaddress

extern malloc
extern memcpy
extern free
extern getmem
extern setmem
extern sizeof
extern addressof
extern ataddress

extern ffi_prim_type
extern ffi_struct_type
extern ffi_struct_type_offsets
extern ffi_type_size
extern ffi_call

var c_uchar
var c_ushort
var c_uint
var c_ulong
var c_char
var c_short
var c_int
var c_long
var c_float
var c_double
var c_pointer
var c_void

var ffi_uchar
var ffi_ushort
var ffi_uint
var ffi_ulong
var ffi_char
var ffi_short
var ffi_int
var ffi_long
var ffi_float
var ffi_double
var ffi_pointer
var ffi_void

var ffi_result

func cffi_init()
	c_uchar = 0
	c_ushort = 1
	c_uint = 2 
	c_ulong = 3
	c_char = 4
	c_short = 5
	c_int = 6
	c_long = 7
	c_float = 8
	c_double = 9
	c_pointer = 10
	c_void = 11
	
	ffi_uchar = ffi_prim_type(c_uchar)
	ffi_ushort = ffi_prim_type(c_ushort)
	ffi_uint = ffi_prim_type(c_uint)
	ffi_ulong = ffi_prim_type(c_ulong)
	ffi_char = ffi_prim_type(c_char)
	ffi_short = ffi_prim_type(c_short)
	ffi_int = ffi_prim_type(c_int)
	ffi_long = ffi_prim_type(c_long)
	ffi_float = ffi_prim_type(c_float)
	ffi_double = ffi_prim_type(c_double)
	ffi_pointer = ffi_prim_type(c_pointer)
	ffi_void = ffi_prim_type(c_void)

	ffi_result = null
end

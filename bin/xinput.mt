# xinput.mt -- interface to xinput using mint's cffi

var XINPUT_STATE

var _xinput_set_state_addr
var _xinput_get_state_addr

func xinput_load()
	var xinput = loadlib("xinput1_3.dll")
	_xinput_set_state_addr = getprocaddress(xinput, "XInputSetState")
	_xinput_get_state_addr = getprocaddress(xinput, "XInputGetState")

	var s = dc_new_struct(2)
		dc_struct_field(s, dc_sigchar_ulong, 1)
		dc_sub_struct(s, 7, 1)
			dc_struct_field(s, dc_sigchar_ushort, 1)
			dc_struct_field(s, dc_sigchar_uchar, 1)
			dc_struct_field(s, dc_sigchar_uchar, 1)
			dc_struct_field(s, dc_sigchar_short, 1)
			dc_struct_field(s, dc_sigchar_short, 1)
			dc_struct_field(s, dc_sigchar_short, 1)
			dc_struct_field(s, dc_sigchar_short, 1)
		dc_close_struct(s)
	dc_close_struct(s)
	
	XINPUT_STATE = cffi_struct(s, [c_ulong, c_ushort, c_uchar, c_uchar, c_short, c_short, c_short, c_short],
							   "dwPacketNumber", "Gamepad.
end

func xinput_get_state(index, state)
	dc_reset()
	dc_arg_long(index)
	dc_arg_pointer(state.ptr)
	write(state._desc.size)
	var result = dc_call_long(_xinput_get_state_addr)
end

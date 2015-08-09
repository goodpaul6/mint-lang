# xinput.mt -- interface to xinput using mint's cffi

var _xinput_state_type
var _xinput_gamepad_type
var _xinput_vibration_type

var _xinput_state_offsets
var _xinput_gamepad_offsets
var _xinput_vibration_offsets

var _xinput_set_state_addr
var _xinput_get_state_addr

func xinput_init()
	var xinput = loadlib("xinput1_3.dll")
	_xinput_set_state_addr = getprocaddress(xinput, "XInputSetState")
	_xinput_get_state_addr = getprocaddress(xinput, "XInputGetState")
	
	_xinput_gamepad_type = ffi_struct_type([ffi_ushort, ffi_uchar, ffi_uchar, ffi_short, ffi_short, ffi_short, ffi_short])
	_xinput_state_type = ffi_struct_type([ffi_ulong, _xinput_gamepad_type])
	_xinput_vibration_type = ffi_struct_type([ffi_ushort, ffi_ushort])

	_xinput_state_offsets = ffi_struct_type_offsets(_xinput_state_type, 2)
	_xinput_gamepad_offsets = ffi_struct_type_offsets(_xinput_gamepad_type, 7)
	_xinput_vibration_offsets = ffi_struct_type_offsets(_xinput_vibration_type, 2)
end

func xinput_state(index)
	return {
		ptr = malloc(ffi_type_size(_xinput_state_type)),
		
		index = index,

		wButtons = 0,
		bLeftTrigger = 0,
		bRightTrigger = 0,
		sThumbLX = 0,
		sThumbLY = 0,
		sThumbRX = 0,
		sThumbRY = 0,
		
		poll = _xinput_state_poll
	}
end

func _xinput_state_poll(self)
	write([self.index, self.ptr])
	if ffi_call(_xinput_get_state_addr, ffi_ulong, [ffi_ulong, ffi_pointer], [self.index, self.ptr])
		var s = self.ptr
		var so = _xinput_state_offsets
		var go = _xinput_gamepad_offsets
		
		var wButtons = getmem(s, so[1], c_ushort)
		var bLeftTrigger = getmem(s, so[1] + go[1], c_uchar) 
		var bRightTrigger = getmem(s, so[1] + go[2], c_uchar)
		var sThumbLX = getmem(s, so[1] + go[3], c_short)
		var sThumbLY = getmem(s, so[1] + go[4], c_short)
		var sThumbRX = getmem(s, so[1] + go[5], c_short)
		var sThumbRY = getmem(s, so[1] + go[6], c_short)
		
		self.wButtons = wButtons
		self.bLeftTrigger = bLeftTrigger
		self.bRightTrigger = bRightTrigger
		self.sThumbLX = sThumbLX
		self.sThumbLY = sThumbLY
		self.sThumbRX = sThumbRX
		self.sThumbRY = sThumbRY
	end
end

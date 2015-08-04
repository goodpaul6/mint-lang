# edge.mt -- testing cutting edge features in mint-lang

func _main()
	cffi_init()
	#~xinput_init()
	
	var state = xinput_state(0)
	while read() != "stop"
		state:poll()
		write(state.sThumbLX)
	end~#
	
	write([0, sizeof(c_pointer)])
	
	return 0
end

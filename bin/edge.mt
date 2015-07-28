# edge.mt -- testing cutting edge features in mint-lang

func main()
	init_cffi()
	xinput_load()

	var state = XINPUT_STATE()
	
	while read() != "stop"
		xinput_get_state(0, state)
		write(state["sThumbLX"])
	end
	
	state:release()

	return 0
end

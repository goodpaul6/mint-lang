# edge.mt -- testing cutting edge features of mint

extern type

func my_list(...)
	return {
		_raw = args,
		GETINDEX = my_get_index,
		SETINDEX = my_set_index
	}
end

func my_get_index(self, index)
	return self._raw[index]
end

func my_set_index(self, index, value)
	self._raw[index] = value
end

func map(f)
	return {
		buckets = array(1024),
		hash = f,
		SETINDEX = map_set_index,
		GETINDEX = map_get_index 
	}
end

func map_set_index(self, index, value)
	var hash = self.hash(index)
	
	var i = hash % len(self.buckets)
	var b = {
		next = self.buckets[i],
		key = index,
		value = value
	}

	self.buckets[i] = b
end

func map_get_index(self, index)
	var hash = self.hash(index)
	
	var bucket = self.buckets[hash % len(self.buckets)]
	while bucket.key != index
		bucket = bucket.next
		if bucket == null
			break
		end
		
		if bucket.key == index
			return bucket.value
		end
	end
	
	if bucket.key == index
		return bucket.value
	end
	
	return null
end

func inthash(i)
	return i
end

func _main()
	var imap = map(inthash)
	
	imap[0] = 0
	imap[1] = 10
	imap[2] = 20
	imap[3] = 30
	
	for var i = 0, i <= 3, i = i + 1
		write(imap[i])
	end
	
	
	return 0
end

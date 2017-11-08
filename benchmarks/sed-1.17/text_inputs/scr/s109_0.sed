#n

#reduce multiple blank lines to one; version using d command
	
/^$/{
	n
	/^\n$/{
		D
	}
}

p

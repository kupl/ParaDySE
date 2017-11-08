#n

#s/d with quit

/o/{
	s/o/()/gpw s177_0.wout
	n
	p
	/t/{
		s/t/TEA/
		q
	}
}

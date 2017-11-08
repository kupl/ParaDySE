#s/d with quit

/o/{
	s/o/\(\)/gw s321_0.wout
	n
	/t/{
		s/t/TEA/g
		q
	}
}

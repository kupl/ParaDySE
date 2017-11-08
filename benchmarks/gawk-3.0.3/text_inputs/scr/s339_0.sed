#s/D with quit

/o/{
	s/o/\(\)/gw s339_0.wout
	n
	/t/{
		s/t/TEA/g
		q
	}
	D
}

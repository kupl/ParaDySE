#n

#s/D with list, global/w flags, Next and Print

/UNIX$/{
        N
	l
        /\nSystem/{
        	s// Operating &/gw s200_0.wout
        	P
		D
	}
}


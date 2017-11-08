#n

#s/D with gpw next and q

/line\.$/{
	s/line/LINE/gp
	s/lines/LINES/gpw s213_0.wout
	n
	q
	D
}

# sub + quit + global + next + write

/\.\.@/ {
	n
	n
	s/dog/HAXED/g
	50q
}

s/HAXED/&/w s306_0.wout

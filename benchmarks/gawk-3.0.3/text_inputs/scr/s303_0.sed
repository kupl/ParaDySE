# sub + quit + global + next + write
/\.\.@/ {
	n
	n
	s/dog/HAXED/g
	50q
}

s/HAXED/&/w s303_0.wout

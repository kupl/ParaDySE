# sub + global + quit + write + Next
/\.\.@/ {
	N
	N
	N
	N
	N
	N
	s/dog/HAXED/g
	50q
}

s/HAXED/&/w s304_0.wout

# sub + global + next + write
/\.\.@/ {
	n
	n
	s/dog/HAXED/g
}

s/HAXED/&/w s305_0.wout

# sub + file write + Next + global
/\.\.@/ {
	N
	s/&/HAXED/g
}

s/\.\.@/&/w s302_0.wout


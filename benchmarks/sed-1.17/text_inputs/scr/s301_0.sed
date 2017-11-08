# sub + global + file write + next

/\.\.@/ {
	n
	s/example/HAXED/g
}

s/HAXED/&/w s301_0.wout

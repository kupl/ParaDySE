#n

#s/D gw, next, Print, and quit

/line\.$/{
        s/line/LINE/g
	P
        s/lines/LINES/gw s215_0.wout
        n
	D
	P
        q
}

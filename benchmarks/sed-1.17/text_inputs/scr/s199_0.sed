#n

#s/d with list

/\.\.@/{
        n
        l
        s/.*/anything/g
	P
	w s199_0.wout
	D
}

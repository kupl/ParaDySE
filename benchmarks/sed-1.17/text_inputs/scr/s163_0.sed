#n

#s/d with list

/\.\.@/{
        n
        l
        s/.*/anything/g
	P
	w s163_0.wout
}

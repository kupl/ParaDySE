#n

#s/d with list

/\.\.@/{
        N
        l
        s/.*/anything/g
	P
        w s164_0.wout
}

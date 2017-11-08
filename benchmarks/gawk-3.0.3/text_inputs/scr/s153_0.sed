#n

#s/d with insert

s/.*w.*/[found a 'w']/gpw s153_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        n
        p
	d
}


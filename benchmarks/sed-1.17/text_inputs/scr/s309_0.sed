
#s/d with insert

s/.*w.*/[found a 'w']/gpw s309_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        n
	d
}

s/^$/BLANKY\!/g

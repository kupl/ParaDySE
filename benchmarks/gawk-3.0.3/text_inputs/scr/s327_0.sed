
#s/D with insert

s/.*w.*/[found a 'w']/gpw s327_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        n
	D
}

s/^$/BLANKY\!/g

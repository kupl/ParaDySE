#n

#s/D with insert

s/.*w.*/[found a 'w']/gpw s189_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        n
        p
	D
}

s/^$/BLANKY\!/gp

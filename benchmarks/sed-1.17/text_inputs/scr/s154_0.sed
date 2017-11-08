#n

#s/d with insert

s/.*w.*/[found a 'w']/gpw s154_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        N
        p
        d
}

s/^$/BLANKY\!/gp

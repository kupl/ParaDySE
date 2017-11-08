#s/d with insert

s/.*w.*/[found a 'w']/gpw s310_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        N
        d
}

s/^$/BLANKY\!/g

#n

#s/d with insert

s/.*w.*/[found a 'w']/gpw s156_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        N
        P
        d
}

s/^$/BLANKY\!/gp

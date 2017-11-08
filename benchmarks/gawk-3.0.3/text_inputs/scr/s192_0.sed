#n

#s/d with insert

s/.*w.*/[found a 'w']/gpw s192_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        N
        P
        D
}

s/^$/BLANKY\!/gp

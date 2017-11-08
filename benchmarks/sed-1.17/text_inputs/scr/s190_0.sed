#n

#s/D with insert

s/.*w.*/[found a 'w']/gpw s190_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        N
        p
        D
}

s/^$/BLANKY\!/gp

#s/D with insert

s/.*w.*/[found a 'w']/gpw s328_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        N
        D
}

s/^$/BLANKY\!/g

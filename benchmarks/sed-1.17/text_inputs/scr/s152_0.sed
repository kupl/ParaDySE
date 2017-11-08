#n

#s/d with append

/\.\.@/{
        a\
        --HEADER ABOVE ME--
        N
        P
}

s/^$/BLANKY\!/gw s152_0.wout
N
P
/.*w.*/d

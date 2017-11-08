#n

#s/D with append

/\.\.@/{
        a\
        --HEADER ABOVE ME--
        N
        P
}

s/^$/BLANKY\!/gw s188_0.wout
N
P
/.*w.*/d

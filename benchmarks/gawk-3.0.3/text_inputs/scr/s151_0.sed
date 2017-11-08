#n

#s/d with append

/\.\.@/{
        a\
        --HEADER ABOVE ME--
        n
        P
}

s/^$/BLANKY\!/gw s151_0.wout
n
P
/.*w.*/d

#n

#s/D with append

/\.\.@/{
        a\
        --HEADER ABOVE ME--
        n
        P
}

s/^$/BLANKY\!/gw s187_0.wout
n
P
/.*w.*/D

#n

#s/d with append

/\.\.@/{
        a\
        --HEADER ABOVE ME--
        N
        p
}

s/^$/BLANKY\!/gpw s150_0.wout
N
/.*w.*/d

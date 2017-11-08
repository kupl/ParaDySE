#n

#s/d with append

/\.\.@/{
        a\
        --HEADER ABOVE ME--
        n
        p
}

s/^$/BLANKY\!/gpw s149_0.wout
n
/.*w.*/d



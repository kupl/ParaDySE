#n

#s/D with append

/\.\.@/{
        a\
        --HEADER ABOVE ME--
        N
        p
}

s/^$/BLANKY\!/gpw s186_0.wout
N
/.*w.*/D

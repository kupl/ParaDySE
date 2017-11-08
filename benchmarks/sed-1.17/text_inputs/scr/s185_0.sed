#n

#s/D with append

/\.\.@/{
        a\
        --HEADER ABOVE ME--
        n
        p
}

s/^$/BLANKY\!/gpw s185_0.wout
n
/.*w.*/D



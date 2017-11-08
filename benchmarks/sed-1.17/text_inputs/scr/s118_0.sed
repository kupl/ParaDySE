#n

#sub + insert

s/.*w.*/[found a 'w']/gpw s118_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        N
        p
}

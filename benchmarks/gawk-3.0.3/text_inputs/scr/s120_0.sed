#n

#sub + insert

s/.*w.*/[found a 'w']/gpw s120_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        N
        P
}

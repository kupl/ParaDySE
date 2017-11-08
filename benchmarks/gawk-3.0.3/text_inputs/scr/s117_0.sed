#n

#sub + insert

s/.*w.*/[found a 'w']/gpw s117_0.wout

/\.\.@/{
        i\
        --INSERTED ABOVE HEADER--
        n
        p
}

#n

#s/d with quit

/o/{
        s/o/()/gpw s180_0.wout
        N
        P
        /t/{
                s/t/TEA/
                q
        }
}

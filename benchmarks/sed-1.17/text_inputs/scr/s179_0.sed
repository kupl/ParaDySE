#n

#s/d with quit

/o/{
        s/o/()/gpw s179_0.wout
        n
        P
        /t/{
                s/t/TEA/
                q
        }
}

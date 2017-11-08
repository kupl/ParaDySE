#n

#s/D with transform, global/w flags, next and Print

/UNIX/{
        y/UNIX/unix/
}

/unix$/{
        n
        /[Ss]ystem/{
                s// operating &/gw s203_0.wout
                P
                D
        }
}

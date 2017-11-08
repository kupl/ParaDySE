#n

#s/D with transform, global/w flags, Next and print

/UNIX/{
        y/UNIX/unix/
}

/unix$/{
        N
        /\n System/{
                s// operating &/gw s202_0.wout
                p
                D
        }
}

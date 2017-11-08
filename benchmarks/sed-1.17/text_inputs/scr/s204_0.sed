#n

#s/D with transform, global/w flags, Next and Print

/UNIX/{
        y/UNIX/unix/
}

/unix$/{
        N
        /\n System/{
                s// operating &/gw s204_0.wout
                P
                D
        }
}

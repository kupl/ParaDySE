#n

#s/D with P, gw, r, Next

/UNIX$/{
        N
        /\n System/{
                s// operating &/gw s208_0.wout
                P
                D
        }
}

/\.\.@ COMPANY LIST/{
        N
        r ../inputs/company.list
}

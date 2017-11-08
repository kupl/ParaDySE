#n

#s/D with Print, gw, r, Next

/UNIX$/{
        n
        /\n System/{
                s// operating &/gw s207_0.wout
                P
                D
        }
}

/\.\.@ COMPANY LIST/{
        n
        r ../inputs/company.list
}

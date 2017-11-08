#n

#s/D with p, gw, r, Next

/UNIX$/{
        N
        /\n System/{
                s// operating &/gw s206_0.wout
                p
                D
        }
}

/\.\.@ COMPANY LIST/{
	N
	r ../inputs/company.list
}

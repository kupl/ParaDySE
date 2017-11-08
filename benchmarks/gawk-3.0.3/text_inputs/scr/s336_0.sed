#s/D with fileread

/^\.\.@ COMPANY LIST/r ../inputs/company.list
/^\.\.@ COMPANY LIST/{
        N
        s/*c*/CFOUND/gw s336_0.wout
	D
}


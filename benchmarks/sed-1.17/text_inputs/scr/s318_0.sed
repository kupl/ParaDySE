#s/d with fileread

/^\.\.@ COMPANY LIST/r ../inputs/company.list
/^\.\.@ COMPANY LIST/{
        N
        s/*c*/CFOUND/gw s318_0.wout
	d
}


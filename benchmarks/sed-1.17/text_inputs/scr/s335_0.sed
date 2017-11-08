#s/D with fileread

/^\.\.@ COMPANY LIST/r ../inputs/company.list
/^\.\.@ COMPANY LIST/{
        n
    	s/*c*/CFOUND/gw s335_0.wout
	D
}

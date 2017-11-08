#s/d with fileread

/^\.\.@ COMPANY LIST/r ../inputs/company.list
/^\.\.@ COMPANY LIST/{
        n
    	s/*c*/CFOUND/gw s317_0.wout
	d
}

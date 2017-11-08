#n

#s/d with fileread

/m$/n;n;p

/^\.\.@ COMPANY LIST/r ../inputs/company.list
/^\.\.@/{
        n
    	s/*c*/CFOUND/gpw s169_0.wout
	d
}

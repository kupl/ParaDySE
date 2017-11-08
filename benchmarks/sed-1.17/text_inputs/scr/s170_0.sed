#n

#s/d with fileread

/m$/n;n;p

/^\.\.@ COMPANY LIST/r ../inputs/company.list
/^\.\.@/{
        N
        s/*c*/CFOUND/gpw s170_0.wout
	d
}

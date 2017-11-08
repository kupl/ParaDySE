#n

#s/d with fileread

/m$/n;n;P

/^\.\.@ COMPANY LIST/r ../inputs/company.list
/^\.\.@/{
        n
        s/*c*/CFOUND/gpw s171_0.wout
	P
	d
}

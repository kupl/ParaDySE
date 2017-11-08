#n

#s/d with fileread

/m$/N;N;P

/^\.\.@ COMPANY LIST/r ../inputs/company.list
/^\.\.@/{
        N
        s/*c*/CFOUND/gpw s172_0.wout
        P
	d
}

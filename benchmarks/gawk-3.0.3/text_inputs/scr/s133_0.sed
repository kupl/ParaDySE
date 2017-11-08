#n

#sub and read

/\.\.@ COMPANY/{
	r ../inputs/company.list
	s/COMPANY LIST/COMPANY DIRECTORY/gpw s133_0.wout
}

/b/{
	n
	p
}

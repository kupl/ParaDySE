#n

#file read

/^\.\.@ COMPANY LIST/r ../inputs/company.list
/^\.\.@ COMPANY LIST/{
	n
	d
	p
}

# file read, global, Next

s/UNIX HAX/LINUX HAX/g

/\.\.@/ {
	N
	/Here are/ {
		r ../inputs/company.list
	}
}


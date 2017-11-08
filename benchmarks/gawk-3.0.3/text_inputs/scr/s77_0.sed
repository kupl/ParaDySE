#n

#multi-line delete (D) and append

/\.\.@/{
	n
	a\
	---APPENDAGE---
	p
}

/cat/{
	p
	D
}	

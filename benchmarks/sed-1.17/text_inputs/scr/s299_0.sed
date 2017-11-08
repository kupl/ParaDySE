#sub read

s/\.\.@ COMPANY LIST/CORPORATE ROSTER/gw s299_0.wout
/\.\.@ COMPANY/{
	r ../inputs/company.list
}

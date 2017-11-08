#n
                                                                                                                  
#sub and read
/\.\.@ COMPANY/{
        r ../inputs/company.list
        s/COMPANY LIST/COMPANY DIRECTORY/gpw s134_0.wout
}
 
/m/{
       	N
        p
}

#n
                                                                                                                              
#sub and read
                                                                                                                              
/\.\.@ COMPANY/{
        r ../inputs/company.list
        s/COMPANY LIST/COMPANY DIRECTORY/gpw s135_0.wout
}
                                                                                                                              
/r/{
        n
        P
}

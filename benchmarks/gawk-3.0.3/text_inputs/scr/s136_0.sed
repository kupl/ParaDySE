#n
                                                                                                                              
#sub and read
                                                                                                                              
/\.\.@ COMPANY/{
        r ../inputs/company.list
        s/COMPANY LIST/COMPANY DIRECTORY/gpw s136_0.wout
}
                                                                                                                              
/\.\.@/{
        N
        P
}
                                                                                                                              

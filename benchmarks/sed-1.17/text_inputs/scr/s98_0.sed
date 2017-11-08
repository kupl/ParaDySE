#n

#multi-line delete and read

/^\.\.@ COMPANY LIST/r ../inputs/company.list
/^\.\.@ COMPANY LIST/{
        N
        p
        D
}

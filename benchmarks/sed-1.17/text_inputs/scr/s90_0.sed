#n

#multi-line delete and list

/^\.\.@ ESCAPE/{
        N
        l
        p
        D
}

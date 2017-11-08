#n

#multi-line delete and list

/^\.\.@ ESCAPE/{
        n
        l
        P
        D
}

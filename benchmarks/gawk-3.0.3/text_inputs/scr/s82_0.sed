#n

#multi-line delete (D) and insert

/\.\.@/{
        i\
        ---INSERT---
        N
        p
        D
}

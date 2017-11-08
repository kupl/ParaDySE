#n

#multi-line delete (D) and insert

/\.\.@/{
        i\
        ---INSERT---
        n
        P
        D
}

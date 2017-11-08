#n

#s/D gw, Next, Print, and quit

/line\.$/{
        s/line/LINE/g
        P
        s/lines/LINES/gw s216_0.wout
        N
        D
        P
        q
}

#n

#s/D with gpw Next and q

/line\.$/{
        s/line/LINE/gp
        s/lines/LINES/gpw s214_0.wout
        N
        D
        q
}

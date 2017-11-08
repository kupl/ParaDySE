#n

#sub and w command (not flag)

/Northeast$/{
        s///g
        w s138_0.wout
}

/cat/{
        N
        p
}

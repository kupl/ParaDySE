#n

#s/D with gw, P, next

/UNIX$/{
        n
        /System/{
                s// operating &/g
		w s211_0.wout
                D
		P
        }
}

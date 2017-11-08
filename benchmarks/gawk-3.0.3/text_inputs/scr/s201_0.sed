#n

#s/D with transform, global/w flags, next and print

/UNIX/{
	y/UNIX/unix/
}

/unix$/{
        n
        /[Ss]ystem/{
                s// operating &/gw s201_0.wout
                p
                D
        }
}

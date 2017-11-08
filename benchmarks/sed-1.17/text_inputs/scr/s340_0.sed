#s/D with quit

/o/{
        s/o/()/gpw s340_0.wout
        N  
        /.*t.*/{
                s/t/TEA/
                q
        }
	D
}

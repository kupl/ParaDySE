#n

#reduce multiple blank lines to one; version using d command

/^$/{
        N
        /^\n$/{
                D
        }
}

p

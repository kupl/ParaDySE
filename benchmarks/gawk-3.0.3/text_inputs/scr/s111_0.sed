#n

#reduce multiple blank lines to one; version using D command

/^$/{
        n
        /^\n$/{
                D
        }
}

P

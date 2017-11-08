#very nested branch and test

/argv\./{
	/leia/b omg
        s/"\(.*\)" "\(.*\)" "\(.*\)"/1 2 3/
        t end
        s/"\(.*\)" "\(.*\)"/1 2/
        t end
        s/"\(.*\)"/1/
}

:omg
s//omg leia is here\!/
:end

#n

#s/d with transform (y)

s/ b.*/ BWORD/gpw s167_0.wout
s/^b.* /BWORD /gp
n
y/aeiou/AEIOU/
P
/^$/d

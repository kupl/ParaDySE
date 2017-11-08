#n

#sub and transform

s/ b.*/ BWORD/gpw s132_0.wout
s/^b.* /BWORD /gp
N
y/aeiou/AEIOU/
P

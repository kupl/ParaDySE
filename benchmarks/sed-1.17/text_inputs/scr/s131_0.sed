#n

#sub and transform

s/ b.*/ BWORD/gpw s131_0.wout
s/^b.* /BWORD /gp
n
y/aeiou/AEIOU/
P

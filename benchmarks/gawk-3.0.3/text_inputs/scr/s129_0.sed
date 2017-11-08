#n

#sub and transform

s/ b.*/ BWORD/gpw s129_0.wout
s/^b.* /BWORD /gp
n
y/aeiou/AEIOU/
p

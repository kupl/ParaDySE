#s/d with transform (y)

s/ b.*/ BWORD/gw s315_0.wout
s/^b.* /BWORD /g
n
y/aeiou/AEIOU/
/^$/d


#s/D with transform (y)

s/ b.*/ BWORD/gw s333_0.wout
s/^b.* /BWORD /g
n
y/aeiou/AEIOU/
/^$/D;D

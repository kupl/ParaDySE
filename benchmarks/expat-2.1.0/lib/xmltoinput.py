fin = open("example3.xml")
text = fin.read()
for c in text:
  print "%d" %(ord(c),)
fin.close()

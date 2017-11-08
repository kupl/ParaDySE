import sys

def main(basename):
	blocks = []
	lines = []
	for i in range(4):
		fname = basename+"_%02d.csv" %(i,)
		block, line = readfile(fname)
		blocks.append(block)
		lines.append(line)

	max_size = max( [len(t) for t in blocks] )

	for i in range(max_size):
		avg_block = sum([get_ith(t, i) for t in blocks]) / len(blocks)
		avg_line = sum([get_ith(t, i) for t in lines]) / len(lines)
		print "%d, %f, %f" %(i, avg_block, avg_line)
	
def readfile(fname):
	fin = open(fname)
	bc_list = []
	lc_list = []
	for line in fin:
		line = line.split(',')
		bc = float(line[4].strip())
		lc = float(line[5].strip())
		bc_list.append(bc)
		lc_list.append(lc)
	return (bc_list, lc_list)


def get_ith(l, i):
	if len(l) > i:
		return l[i]
	else:
		return l[-1]

if __name__ == '__main__':
	basename = sys.argv[1]
	main(basename)

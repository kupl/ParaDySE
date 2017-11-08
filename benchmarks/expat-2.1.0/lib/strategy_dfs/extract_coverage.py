#!/usr/local/bin/python

import subprocess
import sys
import re
import pdb
import glob

ITER = 10

def drive():
	files = glob.glob('output_cfg_*')
	results = list()
	maxs = list()
	iters = list()
	for f in sorted(files):
		cmd = 'grep Iteration %s' %(f,)
		pipe = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE)
		txt = pipe.stdout.read()
		nums = re.findall(' covered (\d+) ', txt)
		results.append(nums)
		maxs.append( str(max(int(x) for x in nums) )  )
		iters.append( str(len(nums)) )


		#print i, len(nums)
		#pdb.set_trace()
		#sys.exit(0)

	maxlen = max(len(result) for result in results)
	for i in range(1, maxlen):
		row = list()
		for j in range(ITER):
			num = get_cov(results[j], i)
			row.append(num)

		avg = sum(int(cov) for cov in row) / float(len([x for x in row if int(x) > 0]))
		print str(i) + ', ' + ', '.join(row) + ', ' +  str(avg)
		#rows.append(row)
	print 'max,', ', '.join(maxs)
	print 'iter,', ', '.join(iters)

	# from lists generate combined output with avg

def get_cov(result, idx):
	if len(result) > idx:
		return result[idx]
	else:
		return result[-1]
	



if __name__ == "__main__":
	drive()

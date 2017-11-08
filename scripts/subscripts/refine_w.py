import random
import os
import sys

weight_file1 = sys.argv[1]
weight_file2 = sys.argv[2]
n_iter = int(sys.argv[3])
trial = int(sys.argv[4])
def read_weights(w_file):
	with open(w_file, 'r') as f:
		lines = f.readlines()
		w = [float(l) for l in lines]
		#print w

	return w

def refine(weight1, weight2):
	weight = []
 	n=1
	for (w1, w2) in zip(weight1, weight2):
		if (w1 > 0) and (w2 > 0):
		#	print(str(n) + ':[ ' + str(min(w1,w2)) + ', 10 ]');
			w = random.uniform(min(w1, w2), 10)
		elif (w1 < 0) and (w2 < 0):
		#	print(str(n) + ':[ -10, ' + str(max(w1,w2)) + ' ]');
			w = random.uniform(-10, max(w1, w2))
		else:
		#	print(str(n) + '[ -10, 10 ]')
			w = random.uniform(-10, 10)

		weight.append(w)
		n=n+1
	return weight

def gen_weight_file(n_weights):
	w1 = read_weights(weight_file1)
	w2 = read_weights(weight_file2)
	for idx in range(1, n_weights+1):
		weights = refine(w1, w2)
		fname = str(trial+1) + "_weights/" + str(idx) + ".weight"
		with open(fname, 'w') as f:
			for w in weights:
				f.write(str(w) + "\n")

os.mkdir(str(trial+1)+"_weights")
gen_weight_file(n_iter)

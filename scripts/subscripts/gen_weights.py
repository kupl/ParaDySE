import random
import os
import sys

size = int(sys.argv[1])

def gen_random_weights(n_features):
	w = [str(random.uniform(-10, 10)) for _ in range(n_features)]
	return w

def gen_weight_file(n_weights):
	for idx in range(1, n_weights+1):
		weights = gen_random_weights(40)
		fname = "1_weights/" + str(idx) + ".weight"
		with open(fname, 'w') as f:
			for w in weights:
				f.write(str(w) + "\n")

os.mkdir("1_weights")
gen_weight_file(size)


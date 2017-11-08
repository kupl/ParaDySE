import random
import os
import sys

def gen_random_weights(n_features):
	w = [str(random.randint(10, 124)) for _ in range(n_features)]
	return w

def gen_weight_file(n_weights):
	for idx in range(1, n_weights+1):
		weights = gen_random_weights(4430)
		fname = "inputss/" + str(idx) + ".input"
		with open(fname, 'w') as f:
			for w in weights:
				f.write(str(w) + "\n")

os.mkdir("inputss")
gen_weight_file(10)


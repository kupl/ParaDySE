import os
import sys

os.mkdir("inputs")
for i in range(1, 1001):
	input = "inputs/" + str(i) + ".input"
	run_cmd = " ".join(
		['../../../bin/run_crest', "'./a.out_comb example2.xml 4430'", "strategy_cfg/input.1", '/dev/null', '40', "-cfg"])
	os.system(run_cmd)

	os.system("mv input " + input)



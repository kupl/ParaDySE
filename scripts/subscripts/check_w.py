import os
import sys
import random
import json
import argparse
import datetime
import shutil
import re


import subprocess
from multiprocessing import Process, Value

start_time = datetime.datetime.now()
__run_count = Value('i', 0)

configs = {
	'script_path': os.path.abspath(os.getcwd()),
	'crest_path': os.path.abspath('../bin/run_crest'),
	'n_exec': 4000,
	'date': datetime.datetime.now().strftime('%m'),
	'top_dir': os.path.abspath('../experiments/')
}

def load_pgm_config(config_file):
	with open(config_file, 'r') as f:
		parsed = json.load(f)

	return parsed

def run_all(pgm_config, n_iter, weights, trial):
	top_dir = "/".join([configs['top_dir'], configs['date']+"__check"+str(trial), pgm_config['pgm_name']])
	log_dir = top_dir + "/" + "__".join([pgm_config['pgm_name'],"check"+str(trial), "logs"])
	os.makedirs(log_dir)

	procs = []
	for w_idx in weights:
		procs.append(Process(target=running_function,
				args=(pgm_config, top_dir, log_dir, w_idx, n_iter, trial)))
	
	for p in procs:
		p.start()
	
def running_function(pgm_config, top_dir, log_dir, weight_idx, n_iter, trial):
	# Prepare directory copies for each weights
	instance_dir = "/".join([top_dir, str(weight_idx)])
	dir_cp_cmd = " ".join(["cp -r", pgm_config['pgm_dir'], instance_dir])
	os.system(dir_cp_cmd)

	os.chdir(instance_dir)
	os.chdir(pgm_config['exec_dir'])
	os.mkdir("logs")

	check_log = open(top_dir + "/" + "./" + "__".join([pgm_config['pgm_name'],"check"+str(trial), "total.log"]), 'a')
	for iter in range(1, n_iter+1):
		(run_cmd, log) = gen_run_cmd(pgm_config, weight_idx, iter)
		os.system(run_cmd)

		current_time = datetime.datetime.now()
		elapsed_time = str((current_time - start_time).total_seconds())

		grep_command = " ".join(["grep", '"It: 4000"', log])
		grep_line = (subprocess.Popen(grep_command, stdout=subprocess.PIPE,shell=True).communicate())[0]
		
		with __run_count.get_lock():
			__run_count.value = __run_count.value + 1

			log_to_write = ", ".join([elapsed_time.ljust(10), str(__run_count.value).ljust(10), grep_line]).strip() + '\n'
		if log_to_write != "":
			check_log.write(log_to_write)
			check_log.flush()

		shutil.move(log, log_dir)

def gen_run_cmd(pgm_config, weight_idx, iter):
	crest = configs['crest_path']
	pgm_name = pgm_config['pgm_name']
	exec_cmd = pgm_config['exec_cmd']
	n_exec = str(configs['n_exec'])

	if (pgm_config['pgm_name']).find('expat-') >= 0:
		input = "expat.input"
	if (pgm_config['pgm_name'] == 'grep-2.2'):
		input = "grep.input"
	if pgm_config['pgm_name'] == 'gawk-3.0.3':
		input = "gawk.input"
	if (pgm_config['pgm_name']).find('sed-') >= 0:
		input = "sed.input"
	if pgm_config['pgm_name'] == 'vim-5.7':
		input = "vim.input"
	if pgm_config['pgm_name'] == 'tree-1.6.0':
		input = "tree.input"
	if pgm_config['pgm_name'] == 'replace':
		input = "replace.input"
	if pgm_config['pgm_name'] == 'floppy':
		input = "floppy.input"
	if pgm_config['pgm_name'] == 'cdaudio':
		input = "cdaudio.input"
	if pgm_config['pgm_name'] == 'kbfiltr':
		input = "kbfiltr.input"

	log = "logs/" + "__".join([pgm_name+"check"+args.trial, str(weight_idx), "ours", str(iter)]) + ".log"
	weight = configs['script_path'] +"/"+ str(trial)+ "_weights/" + str(weight_idx) + ".weight"

	run_cmd = " ".join([crest, exec_cmd, input, log, n_exec, "-param", weight])
	print run_cmd

	return (run_cmd, log)

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	
	parser.add_argument("pgm_config")
	parser.add_argument("n_iter")
	parser.add_argument("trial")
	parser.add_argument("-l", type=lambda s:[int(item) for item in s.split(',')])

	args = parser.parse_args()
	pgm_config = load_pgm_config(args.pgm_config)
	n_iter = int(args.n_iter)
	trial = int(args.trial)
	weights = args.l

	run_all(pgm_config, n_iter, weights, trial)

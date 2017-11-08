from multiprocessing import Process

import signal
import subprocess
import os
import sys
import random
import json
import argparse
import datetime
import shutil
import re

start_time = datetime.datetime.now()
date = start_time.strftime('%m')

configs = {
	'script_path': os.path.abspath(os.getcwd()),
	'crest_path': os.path.abspath('../bin/run_crest'),
	'n_exec': 4000,
	'date': date,
	'top_dir': os.path.abspath('../experiments/')
}

def load_pgm_config(config_file):
	with open(config_file, 'r') as f:
		parsed = json.load(f)

	return parsed

def gen_run_cmd(pgm_config, idx):
	crest = configs['crest_path']
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


	log = "logs/" + "__".join([pgm_config['pgm_name']+"find"+args.trial, "ours", str(idx)]) + ".log"
	weight = configs['script_path'] +"/"+ args.trial + "_weights/" + str(idx) + ".weight"
	
	run_cmd = " ".join([crest, exec_cmd, input, log, n_exec,
		"-param", weight, "features", "dominator"])

	return (run_cmd, log)

def running_function(pgm_config, top_dir, log_dir, group_id, count, trial):
	group_dir = top_dir + "/" + str(group_id)
	os.system(" ".join(["cp -r", pgm_config['pgm_dir'], group_dir]))
	os.chdir(group_dir)
	os.chdir(pgm_config['exec_dir'])
	os.mkdir("logs")

	find_log = open(top_dir + "/"+ "__".join([pgm_config['pgm_name'],"find"+str(trial), "total.log"]), 'a')
	for idx in range((group_id - 1)*count+1, (group_id)*count+1):
		(run_cmd, log) = gen_run_cmd(pgm_config, idx)
		
		with open(os.devnull, 'wb') as devnull:
			os.system(run_cmd)

		current_time = datetime.datetime.now()
		elapsed_time = str((current_time - start_time).total_seconds())
		
		grep_command = " ".join(["grep", '"It: 4000"', log])
		grep_line = (subprocess.Popen(grep_command, stdout=subprocess.PIPE,shell=True).communicate())[0]
		log_to_write = ", ".join([elapsed_time.ljust(10), str(idx).ljust(10), grep_line]).strip() + '\n'
		if log_to_write != "":
			find_log.write(log_to_write)
			find_log.flush()

		shutil.move(log, log_dir)
	
def run_all(pgm_config, n_iter, n_groups, trial):
	top_dir = "/".join([configs['top_dir'], configs['date']+"__find"+str(trial), pgm_config['pgm_name']])
	log_dir = top_dir + "/" + "__".join([pgm_config['pgm_name'],"find"+str(trial), "logs"])
	os.makedirs(log_dir)
	pgm_dir = pgm_config["pgm_dir"]

	
	procs = []
	count = int(n_iter / n_groups)
	mod = int(n_iter % n_groups)
	
	for gid in range(1, n_groups+1):
		procs.append(Process(target=running_function, args=(pgm_config, top_dir, log_dir, gid, count, trial)))
	
	for p in procs:
		p.start()
	
if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	
	parser.add_argument("pgm_config")
	parser.add_argument("n_iter")
	parser.add_argument("n_parallel")
	parser.add_argument("trial")

	args = parser.parse_args()
	pgm_config = load_pgm_config(args.pgm_config)
	n_iter = int(args.n_iter)
	n_parallel = int(args.n_parallel)
	trial = int(args.trial)
	

	run_all(pgm_config, n_iter, n_parallel, trial)
	

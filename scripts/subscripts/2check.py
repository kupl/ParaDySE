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
	'n_exec': 4000,
	'date': date,
	'top_dir': os.path.abspath('../experiments/')
}

def load_pgm_config(config_file):
	with open(config_file, 'r') as f:
		parsed = json.load(f)

	return parsed

def check_10(pgm_config, trial):
	top_dir = "/".join([configs['top_dir'], configs['date']+"__find"+str(trial), pgm_config['pgm_name']])
	
	log_dir = top_dir + "/" + "__".join([pgm_config['pgm_name'],"find"+str(trial), "logs"])
	os.chdir(log_dir)
	grep_sort_cmd = " ".join(["grep -Rnw", '"It: 4000"', "|", "sort -k6n", "|", "tail -n 10", "> find_result"])
	print grep_sort_cmd
	os.system(grep_sort_cmd)
	
	top_list = []
	f = open(log_dir + "/find_result", 'r')
	lines = f.readlines()
	for line in lines:
		data = line.split("ours__")
		data2 = data[1].split(".log")
		top_list.append(data2[0])
	f.close()				
	
	scr_dir = configs['script_path'] 
	os.chdir(scr_dir)
	topw_list = ", ".join(top_list)
	check_cmd = " ".join(["python", "subscripts/check_w.py", args.pgm_config, "10", "-l", '"'+ topw_list +'"', str(trial)])
	print check_cmd
	os.system(check_cmd)

if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	
	parser.add_argument("pgm_config")
	parser.add_argument("trial")

	args = parser.parse_args()
	pgm_config = load_pgm_config(args.pgm_config)
	trial = int(args.trial)

	check_10(pgm_config, trial)

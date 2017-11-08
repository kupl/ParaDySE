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

def refine_2(pgm_config, n_iter, trial):
	top_dir = "/".join([configs['top_dir'], configs['date']+"__check"+str(trial), pgm_config['pgm_name']])
	log_dir = top_dir + "/" + "__".join([pgm_config['pgm_name'],"check"+str(trial), "logs"])
	os.chdir(log_dir)

	grep_sort_cmd = " ".join(["grep -Rnw", '"It: 4000"', "> check_result"])
	print grep_sort_cmd
	os.system(grep_sort_cmd)
	
	w_coverage = {};
	f = open(log_dir + "/check_result", 'r')
	lines = f.readlines()
	for line in lines:
		data = line.split("__")
		token = data[3].split()		
		coverage = token[5].split(",")
		if w_coverage.has_key(data[1]):
			w_coverage[(data[1])].append(int(coverage[0]))
		else:
			w_coverage[(data[1])] = [int(coverage[0]),]
	f.close()				
	
	avr_list =[];	
	for key in w_coverage.keys():
		if w_coverage[key]==0 or len(w_coverage[key])<5:
		#if w_coverage[key]==0 :
			return None
		else:				
			tup = (key, (sum(w_coverage[key], 0.0)/len(w_coverage[key])));			
			avr_list.append(tup) 
	
	sort_list = sorted(avr_list, key=lambda tup: tup[1]) #coverage sort
	scr_dir1 = configs['script_path'] 
	os.chdir(scr_dir1)
	print sort_list
	f = open(scr_dir1 + "/"+pgm_config['pgm_name']+"_topcheck_log", 'a')
	f.writelines(["%s \n" % sort_list[(len(sort_list)-1)][1] ])
	f.close()
	top1_w = (sort_list[len(sort_list)-1])[0] + ".weight"	
	top2_w = (sort_list[len(sort_list)-2])[0] + ".weight"	
	
	f = open(scr_dir1 + "/top2w_"+pgm_config['pgm_name']+"_log", 'a')
	f.write("top 1: "+top1_w + " Top 2: " + top2_w +"\n" )
	f.close()
		
	print "Top 1: ", top1_w, "Top 2: ", top2_w  
	#remove check folder
	os.chdir(top_dir)
	for sort_element in sort_list:
		w_name = sort_element[0]
		rm_cmd = " ".join(["rm -rf", w_name])
		os.system(rm_cmd)

	scr_dir = configs['script_path'] 
	os.chdir(scr_dir)
	refine_cmd = " ".join(["python", "subscripts/refine_w.py", args.trial+"_weights/"+top1_w, args.trial+"_weights/"+top2_w, args.n_iter, args.trial])
	print refine_cmd
	os.system(refine_cmd)


if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	
	parser.add_argument("pgm_config")
	parser.add_argument("n_iter")
	parser.add_argument("trial")

	args = parser.parse_args()
	pgm_config = load_pgm_config(args.pgm_config)
	n_iter = int(args.n_iter)
	trial = int(args.trial)

	refine_2(pgm_config, n_iter, trial)

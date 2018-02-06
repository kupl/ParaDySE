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
day = start_time.strftime('%m%d')

configs = {
	'script_path': os.path.abspath(os.getcwd()),
	'date': date,
	'day': day,
	'top_dir': os.path.abspath('../experiments/')
}

def load_pgm_config(config_file):
	with open(config_file, 'r') as f:
		parsed = json.load(f)

	return parsed
	
def run_all(pgm_config, n_iter, n_groups):
	#make final_log folder
	for k in range(1, 2):
		final_dir = "/".join([configs['top_dir'], str(k)+ pgm_config['pgm_name']+"__all__logs"])
		os.makedirs(final_dir)
		print final_dir
		#make timelog and w_folder
		all_dir = "/".join([final_dir, "all_logs"])
		os.makedirs(all_dir)
		time_dir = "/".join([final_dir, "time_logs"])
		os.makedirs(time_dir)
		print time_dir
		w_dir = "/".join([final_dir, "w_"+ pgm_config['pgm_name']])
		os.makedirs(w_dir)
		print w_dir
		
		scr_dir = configs['script_path'] 
		os.chdir(scr_dir)
		print scr_dir
		genw_cmd = " ".join(["python", "subscripts/gen_weights.py", args.n_iter])
		print genw_cmd
		os.system(genw_cmd)
		cpw_final_cmd = " ".join(["cp -r", "1_weights", w_dir])
		print cpw_final_cmd
		os.system(cpw_final_cmd)
		f = open(pgm_config['pgm_name']+"_topcheck_log", 'a')
		f.writelines(["0\n"])
		f.close()
	
		for i in range(1, 20):
			#find - check - refine	
			find_cmd = " ".join(["python", "subscripts/1find.py", args.pgm_config, args.n_iter, args.n_parallel, str(i)])	
			print find_cmd
			os.system(find_cmd)
			check_cmd = " ".join(["python", "subscripts/2check.py", args.pgm_config, str(i)])
			print check_cmd
			os.system(check_cmd)
			refine_cmd = " ".join(["python", "subscripts/3refine.py", args.pgm_config, args.n_iter, str(i)])
			print refine_cmd
			os.system(refine_cmd)
		
			#clean folder
			find_dir = "/".join([configs['top_dir'], configs['date']+"__find"+str(i), pgm_config['pgm_name']])
			os.chdir(find_dir)
			print find_dir
			for j in range(1, n_parallel+1):
				rm_cmd = " ".join(["rm -rf", str(j)])
				print rm_cmd
				os.system(rm_cmd)
	 	
			#copy_find_logs
			findtotal_cmd = "__".join([pgm_config['pgm_name'],"find"+str(i), "total.log"])	
			cp_findtotal_cmd = " ".join(["cp -r", findtotal_cmd, time_dir]) 
			print cp_findtotal_cmd
			os.system(cp_findtotal_cmd)
			findlogs_cmd = "__".join([pgm_config['pgm_name'],"find"+str(i), "logs/*.log"])  
			cp_findlogs_cmd = " ".join(["cp -r", findlogs_cmd, all_dir]) 
			print cp_findlogs_cmd
			os.system(cp_findlogs_cmd)
		
			#copy_check_logs
			check_dir = "/".join([configs['top_dir'], configs['date']+"__check"+str(i), pgm_config['pgm_name']])
			os.chdir(check_dir)
			print check_dir
			checktotal_cmd = "__".join([pgm_config['pgm_name'],"check"+str(i), "total.log"])	
			cp_checktotal_cmd = " ".join(["cp -r", checktotal_cmd, time_dir]) 
			print cp_checktotal_cmd
			os.system(cp_checktotal_cmd)
			checklogs_cmd = "__".join([pgm_config['pgm_name'],"check"+str(i), "logs/*.log"])  
			cp_checklogs_cmd = " ".join(["cp -r", checklogs_cmd, all_dir]) 
			print cp_checklogs_cmd
			os.system(cp_checklogs_cmd)
		
			#check saturation
			os.chdir(scr_dir)
			print scr_dir
			f = open(pgm_config['pgm_name']+"_topcheck_log", 'r')
			lines = f.readlines()
			if int(float(lines[len(lines)-1]))<=int(float(lines[len(lines)-2])):
				print "Saturation !!\n"
				f.close()
				break		
			f.close()	

			#copy_w
			cpw2_final_cmd=	" ".join(["cp -r", str(i+1)+"_weights", w_dir])
			print cpw2_final_cmd
			os.system(cpw2_final_cmd)

		os.chdir(scr_dir)
		#the best w's average coverage	
		f = open("top2w_"+ pgm_config['pgm_name']+"_log", 'r')
		lines = f.readlines()
		topw = []
		n = len(lines)
		topw = lines[len(lines)-2].split()
	
		cp_topw = " ".join(["cp", str(n-1)+"_weights/" + topw[2], pgm_config['pgm_dir']+pgm_config['exec_dir']+"/best.w"]) 
		mv_topw = " ".join(["cp", str(n-1)+"_weights/" + topw[2], final_dir+"/best.w"]) 
		os.system(cp_topw)
		os.system(mv_topw)
	
	#	average = " ".join(["python", "100.py",args.pgm_config, "20", "10", "1", "ours"])
	#	os.system(average)
		f.close()

		#cp information 
		for i in range(1, 10):
			rm_cmd = " ".join(["rm -rf", str(i)+"_weights"])
			print rm_cmd
			os.system(rm_cmd)
	
		cp_topcheck = " ".join(["mv", pgm_config['pgm_name']+"_topcheck_log", final_dir])
		os.system(cp_topcheck)
	
		cp_topw = " ".join(["mv", "top2w_"+ pgm_config['pgm_name']+"_log", final_dir])
		os.system(cp_topw)
		#rm inform(experiments)
		ex_dir = configs['top_dir']
		os.chdir(ex_dir)
		log_100 = "/".join([configs['day']+"__ours1", pgm_config['pgm_name'], "logs"])
		mv_100 = " ".join(["mv", log_100, final_dir])
		print mv_100	
		#os.system(mv_100)
		rm_folder = " ".join(["rm -rf", configs['date']+"*"])
		os.system(rm_folder)

		print "#############################################"
		print "Successfully Generate a Search Heuristic!!!!!"
		print "#############################################"
	
if __name__ == "__main__":
	parser = argparse.ArgumentParser()
	
	parser.add_argument("pgm_config")
	parser.add_argument("n_iter")
	parser.add_argument("n_parallel")

	args = parser.parse_args()
	pgm_config = load_pgm_config(args.pgm_config)
	n_iter = int(args.n_iter)
	n_parallel = int(args.n_parallel)
	
	run_all(pgm_config, n_iter, n_parallel)

# ParaDySE 

ParaDySE (Parametric Dynamic Symbolic Execution) is a tool 
that automatically generates search heuristics for concolic testing. 
The tool is implemented on top of [CREST][crest], 
a publicly available concolic testing tool for C.

## Install ParaDySE. 
You need to install [Ubuntu 16.04.3(64 bit)][ubuntu].
Then follow the steps:
```sh
$ sudo apt-get install ocaml #(if not installed) 
$ git clone https://github.com/kupl/ParaDySE.git 
$ cd ParaDySE/cil
$ ./configure
$ make
$ cd ../src
$ make
```

## Run a benchmark.
Please read **README\_ParaDySE** file located in each benchmark directory. 
In **README\_ParaDySE** file, we explain how to compile each benchmark and run ParaDySE.
For instance, we can compile grep-2.2 and run ParaDySE as follows:
```sh
$ cd ~/ParaDySE/benchmarks/grep-2.2 
# vi README_ParaDySE
$ ./configure
$ cd src
$ make
$ ../../../bin/run_crest './grep aaaaaaaaaa /dev/null' grep.input log 4000 -param grep.w
```
Each argument of the last command means:
-	**'./grep aaaaaaaaaa /dev/null'** : a subject program under test. 
-	**grep.input** : an initial input. 
-	**log** : a file which stores the result of testing.
-	**4000** : the number of executions of the program.
-	**-param** : search heuristic (e.g., -dfs, -cfg, -random, -cgs, -generational) 

In particular, our heuristic (param) additionally takes the parameter (e.g., gawk.w) as argument 
which is a 40-dimensional vector of real numbers. 
Note that the implementation of CGS(Context-Guided Search) and Generational search came from the author of [FSE'14 paper][FSE]. 

If you want to run another benchmark (e.g., sed-1.17), read the **README_ParaDySE** file in the directory:
```sh
$ cd ParaDySE/benchmarks/sed-1.17 
$ vi README_ParaDySE
```

## Automatically generate a search heuristic.
The script for automatically generating a search heuristic is run on an instrumented program. 
For instance, we can generate a search heuristic for **tree-1.6.0** as follows:
```sh
$ screen 
# Initially, each benchmark should be compiled with ParaDySE:
$ cd ParaDySE/benchmarks/tree-1.6.0
$ make
# Run a script
$ cd ~/ParaDySE/scripts
$ python fullauto.py pgm_config/tree.json 1000 20 
```

Each argument of the last command means:
-	**pgm_config/tree.json** : a json file to describe the benchmark.
-	**1000** : the number of parameters to evaluate in **Find Phase**
-	**20** : the number of cpu cores to use in parallel

If the script successfully ends, you can see the following command:
```sh
#############################################
Successfully Generate a Search Heuristic!!!!!
#############################################
```
Then, you can find the generated search heuristic (i.e. parameter values) as follows:
```sh
$ cd ~/ParaDySE/experiments/1tree-1.6.0__all__logs
$ vi best.w # (Automatically Generated Search Heuristic)
```
You can run tree-1.6.0 with the generated heuristic as follows: 
```sh
$ cd ~/ParaDySE/experiments/1tree-1.6.0__all__logs
$ cp best.w ~/ParaDySE/benchmarks/tree-1.6.0/best.w 
$ cd ~/ParaDySE/benchmarks/tree-1.6.0 
$ ../../bin/run_crest './tree aaaaaaaaaa aaaaaaaaaa' tree.input log 4000 -param best.w
```

we run **ParaDySE** on a linux machine with two Intel Xeon Processor E5-2630 and 192GB RAM. 
Time for obtaining the search heuristic for tree-1.6.0 is approximately 3 hours.  

[crest]: https://github.com/jburnim/crest
[ubuntu]: https://www.ubuntu.com/download/desktop
[FSE]: https://dl.acm.org/citation.cfm?id=2635872&CFID=1004243459&CFTOKEN=16632066

# ParaDySE 

ParaDySE (Parametric Dynamic Symbolic Execution) 
is a concolic testing tool that automatically generates an optimal 
search heuristic for a given subject program. 
The tool is implemented on top of [CREST][crest], 
a publicly available concolic test generation tool for C. 	

## Install ParaDySE. 
You need to install [Ubuntu 16.04.3(64 bit)][ubuntu].
Then follow the steps:
```sh
$ sudo apt-get install ocaml #(if not installed) 
$ git clone https://github.com/kupl/ParaDySE.git 
$ cd ParaDySE/cil
$ ./configure
$ make
$ cd ParaDySE/src
$ make
```
	
## Run ParaDySE. 
ParaDySE is run on an instrumented progam as:
```sh
$ bin/run_crest PROGRAM INTIAL_INPUT LOG NUM_ITERATIONS -STRATEGY
```
-	PROGRAM : a subject program under test. 
-	INITIAL\_INPUT : an initial input. 
-	LOG : a file which stores the result of testing.
-	NUM\_ITERATIONS : the number of executions of the program.
-	STRATEGY : dfs, cfg, random and so on.

In particular, our heuristic (param) additionally takes the parameter (e.g., gawk.w) as input 
which is a 40-dimensional vector of real numbers.

## Run benchmarks. 
We explain how to compile each benchmark and run ParaDySE in each directory.
```sh
$ cd ParaDySE/benchmarks
# read README_ParaDySE in each program directory.
```
For instance, we explain how to compile grep-2.2 and run ParaDySE as follows:
```sh
$ cd ParaDySE/benchmarks/grep-2.2 # vi REAEME_ParaDySE
# compile grep 2.2 for use with ParaDySE 
$ ./configure
$ cd src
$ make
# run ParaDySE
$ ../../../bin/run_crest './grep aaaaaaaaaa /dev/null' grep.input log 4000 -param grep.w
```

## Run a script.
The script for automatically generating a search heuristic is run on an instrumented program as:
```sh
$ screen
# if you don't install screen, install as follows: sudo apt-get install screen
$ cd ParaDySE/scripts
$ python fullauto.py pgm_config/(pgm_name) (the number of samples) (the number of cores)
# (ex) python fullauto.py pgm_config/gawk.json 1000 20 
```

[crest]: https://github.com/jburnim/crest
[ubuntu]: https://www.ubuntu.com/download/desktop

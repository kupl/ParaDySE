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
# (ex) bin/run_crest './gawk aaaaaaaaaa text_inputs/*' gawk.input log 4000 -param gawk.w
```
-	PROGRAM : a subject program under test. 
-	INITIAL\_INPUT : an initial input. 
-	LOG : a file which stores the result of testing.
-	NUM\_ITERATIONS : the number of executions of the program.
-	STRATEGY : dfs, cfg, random and so on.

In particular, our heuristic (param) additionally takes the parameter (e.g., gawk.w) as input 
which is a 40-dimensional vector of real numbers.
We also explain how to compile each benchmark and run ParaDySE in each directory.
```sh
$ cd ParaDySE/benchmarks
# read README_ParaDySE in each benchmark directory.
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

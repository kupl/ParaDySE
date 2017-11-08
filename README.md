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
$ cd ../src
$ make
```

## Run a benchmark.
Please read **README\_ParaDySE** file in each benchmark, explaining how to compile 
each benchmark and run ParaDySE.
 
For instance, we can compile grep-2.2 and run ParaDySE as follows:
```sh
$ cd ParaDySE/benchmarks/grep-2.2 
$ vi REAEME_ParaDySE
# compile grep 2.2 for use with ParaDySE 
$ ./configure
$ cd src
$ make
# run ParaDySE
$ ../../../bin/run_crest './grep aaaaaaaaaa /dev/null' grep.input log 4000 -param grep.w
```

-	PROGRAM ('./grep aaaaaaaaaa /dev/null') : a subject program under test. 
-	INITIAL\_INPUT (grep.input) : an initial input. 
-	LOG (log) : a file which stores the result of testing.
-	NUM\_ITERATIONS (4000): the number of executions of the program.
-	STRATEGY (param): dfs, cfg, random and so on.

In particular, our heuristic (param) additionally takes the parameter (e.g., gawk.w) as input 
which is a 40-dimensional vector of real numbers.

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

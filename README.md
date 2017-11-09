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
Please read **README\_ParaDySE** file located in each benchmark directory. 
In **README\_ParaDySE** file, we explain how to compile each benchmark and run ParaDySE.
For instance, we can compile grep-2.2 and run ParaDySE as follows:
```sh
$ cd ParaDySE/benchmarks/grep-2.2 
# vi README_ParaDySE
$ ./configure
$ cd src
$ make
$ ../../../bin/run_crest './grep aaaaaaaaaa /dev/null' grep.input log 4000 -param grep.w
```

We explain each argument of last command: 
-	**'./grep aaaaaaaaaa /dev/null'** : a subject program under test. 
-	**grep.input** : an initial input. 
-	**log** : a file which stores the result of testing.
-	**4000** : the number of executions of the program.
-	**-param** : search heuristic (e.g., -dfs, -cfg, -random) 

In particular, our heuristic (param) additionally takes the parameter (e.g., gawk.w) as argument 
which is a 40-dimensional vector of real numbers.

If you want to run another benchmark (e.g., sed-1.17), read **README_\ParaDySE** file in the directory:
```sh
$ cd ParaDySE/benchmarks/sed-1.17 
$ vi README_ParaDySE
```

## Automatically generate a search heuristic.
The script for automatically generating a search heuristic is run on an instrumented program. 
For instance, we can generate a search heuristic for **tree-1.6.0** as follows:
```sh
$ screen #(recommanded)
# Initially, each benchmark should be compiled with ParaDySE:
$ cd ParaDySE/benchmarks/tree-1.6.0
$ make
# Run a script
$ cd ~/ParaDySE/scripts
$ python fullauto.py pgm_config/tree.json 100 4 
```

We explain each argument of last command: 
-	**pgm_config/tree.json** : a json file to describe the benchmark: 
```
{
	"pgm_name": "tree-1.6.0",
	"pgm_dir": "../benchmarks/tree-1.6.0/",
	"exec_dir": "./",
	"exec_cmd": "'./tree aaaaaaaaaa aaaaaaaaaa'"
}
```
-	**100** : the number of parameters to evaluate in **Find Phase**
-	**4** : the number of cpu cores to use in parallel

[crest]: https://github.com/jburnim/crest
[ubuntu]: https://www.ubuntu.com/download/desktop

#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys

def parseCallGraphTxt(filename):
	succ = {}
	fin = open(filename)
	while 1:
		line = fin.readline()
		if not line:
			break
	
		cols = line.split()
		caller = cols[0]
		col2 = cols[1]
		
		# when the function is external skip it
		if col2 == "(external):":
			fin.readline()
			fin.readline()
			continue
	
		if caller in succ:
			print col1, "is already in dic"
			sys.exit(0)
		succ[caller] = []
	
		cols = fin.readline().strip().split(":")
		check = cols[0]
		children = cols[1]
		if check != "calls":
			print "Error1", calls
			sys.exit()
		for child in children.split():
			succ[caller].append(child)
	
	
		called = fin.readline()
		if called.strip().split(":")[0] != "is called by":
			print "Error2", called
			sys.exit()

	fin.close()
	return succ		

def printCallGraphToDot(graph):	
	for caller in graph.keys():
		for child in graph[caller]:
			print caller, "->", child


if __name__ == "__main__":
	g = parseCallGraphTxt("callgraph_withoutIndirect")
	printCallGraphToDot(g)
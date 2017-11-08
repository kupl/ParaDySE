#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
import callgraphtodot
import pdb

branch_cnt = dict() # key: fid,  value: number of branches in the function
fnameTofid = dict() 
fidTofname = dict()

def build_info():
	list1 = list()
	fin = open("cfg_func_map")
	for line in fin:
		(fname, fid) = line.strip().split()
		fid = int(fid)
		
		fnameTofid[fname] = fid
		fidTofname[fid] = fname
		
		list1.append(fname)
	fin.close()

	list2 = list()
	fin = open("branches")
	while True:
		line = fin.readline()

		if not line:
			break
	
		(i, paircnt)  = line.strip().split()
		paircnt = int(paircnt)
		list2.append(paircnt*2)
		for i in range(paircnt):
			fin.readline()
	fin.close()
	
	for (i, fname) in enumerate(list1):
		branch_cnt[fnameTofid[fname]] = list2[i]

	pass

def changeFnameToFid(t):
	g = dict()
	for (k, v) in t.items():
		k = fnameTofid[k]
		l = list()
		for item in v:
			if item not in fnameTofid:
				pass
				#print "UNKNOWN fname:", item
			else:
				l.append(fnameTofid[item])
		
		g[k] = l
	return g

def doDFS(k, g, dfs_set, covered):
	dfs_set.add(k)
	covered[k] = True
	
	for child in g[k]:
		if child not in covered:
			doDFS(child, g, dfs_set, covered)

	return dfs_set

def get_calls():
	cfg1 = 'cfg_base'
	cfg2 = 'cfg_fcall_edge'

	g1 = build_graph2('cfg_base')
	g2 = build_graph2('cfg_fcall_edge')
	
	calls = dict()
	
	for k in g1.keys():
		s1 = set(g1[k])
		s2 = set(g2[k])
		
		s = s2 - s1
		if len(s) > 0:
			#calls[k] = set(node for node in s if node in fidTofname)
			calls[k] = s
			for node in s:
				if node not in fidTofname:
					pdb.set_trace()
					
	
	return calls		

def calc_fscore():
	
	fscore = dict()
	
	build_info()
	
	temp = callgraphtodot.parseCallGraphTxt("callgraph_withoutIndirect")
	g = changeFnameToFid(temp)
	
	for k in g.keys():
		dfs_set = set()
		covered = dict()
		dfs_set = doDFS(k, g, dfs_set, covered)
		#print "fname:%s size of dfs set:%d" %(fidTofname[k], len(dfs_set))
		score = 0
		for f in dfs_set:
			score += branch_cnt[f]
		
		fscore[k] = score
		#print fidTofname[k], score

	return fscore




def calc_bscore(filename, fvalue=False):
	
	funname = get_funname()
	branches = get_branches()
	
	if fvalue:
		fscore = calc_fscore()
		calls = get_calls()
	else:
		fscore = dict()
		calls = dict()
	
	# graph; predecessor
	g = build_graph2(filename)
	
	unreachable = find_unreachable_nodes(g, funname.keys())
	
	for node in unreachable:
		del g[node]
	
	# reversed graph; successor
	r = reverse_graph(g)
	
	root_nodes = find_root_nodes(r, funname.keys())
	#_print_branches(branches)

	dominator = {}
	bscore = {}
	
	for root in sorted(root_nodes):
		dfs_nodes = []
		dfs_visited = {}
		# get reachable nodes from a root
		dfs(root, g, dfs_nodes, dfs_visited)
		
		# make a sub graph
		sub_g = {}
		for node in dfs_nodes:
			sub_g[node] = g[node]

		sub_r = reverse_graph(sub_g)
		
		dom = dominator2(root, sub_g, sub_r, dfs_nodes)
		for k in dom.keys():
			if k not in dominator:
				dominator[k] = dom[k]
			else:
				dominator[k] =  dominator[k] | dom[k]


		if fvalue:
			bvalue = calc_value(dom, dfs_nodes, branches, calls=calls, fvalue=fscore)
		else:
			bvalue = calc_value(dom, dfs_nodes, branches)
		for bid in bvalue.keys():
			if bid not in branches: continue
			
			if bid not in bscore:
				bscore[bid] = bvalue[bid]
			else:
				bscore[bid] = max(bscore[bid], bvalue[bid])

	print_dom(dominator, branches)
	_print_values(bscore)

	#print len(g_succ), len(g_pred)
	#print g_succ
	#print g_pred


def print_dom(dom, branches):
	fout = open('dominator', 'a')
	for k in sorted(dom.keys()):
		if k not in branches: continue
		l = [str(x) for x in dom[k] if x in branches]
		#l = filter(lambda x: x in branches, list(dom[k]))
		#l = map(lambda x: str(x), l)
		fout.write("%s %s\n" %(k, ' '.join(l)) )
	fout.close()

# functions for logging
def _print_values(bvalue):
	for node in sorted(bvalue.keys()):
		print node, bvalue[node]

def _print_branches(branches):
	for node in sorted(branches.keys()):
		print node, branches[node]

def _print_dominator(nodes, domin):
	for n in nodes:
		print n,":", sorted(domin[n])

def get_funname():
	fun = {}
	fin = open("cfg_func_map")
	for line in fin:
		cols = line.strip().split()
		fname = cols[0]
		fid = int(cols[1])
		fun[fid] = fname
	fin.close()
	
	return fun


def find_root_nodes(r, fids):
	# CFG 가 함수 콜 edge 를 포함하고 있을 경우 
	# 각 함수별로 CFG 가 나누어 지지 않고 큰 하나의 그래프로 여러 함수 CFG가 연결될 수 있다
	# 이를 고려해서 전체 그래프에서 루트노드만 찾아낸다	
	# 각 함수의 entry 노드가 predecessor 가 없을 경우 그 노드는 루트 노드이다

	roots = list() 
	for fid in fids:
		if fid in r and len(r[fid]) == 0:
			roots.append(fid)
			#print 'root node:', fid, fun_id[fid]

	return roots

def find_unreachable_nodes(g, roots):
	# 루트 노드들로부터 시작해서 dfs 통해 모든 reachable 한 노드를 구한뒤
	# key 들과 비교한다 
	
	dfs_visited = dict()
	reached_set = set()
	for root in roots:
		dfs_nodes = []
		dfs(root, g, dfs_nodes, dfs_visited)
		reached_set = reached_set | set(dfs_nodes)

	#print "Unreachable nodes:"
	#print set(g.keys()) - reached_set
	return set(g.keys()) - reached_set

def get_branches():
	branches = {}
	fin = open("branches")
	while 1:
		line = fin.readline()
		if not line:
			break
		cols = line.strip().split()
		fnum = cols[0]
		numbranch = int(cols[1])

		while numbranch > 0:
			line = fin.readline()
			cols = line.strip().split()
			b1 = int(cols[0])
			b2 = int(cols[1])
			if b1 not in branches:
				branches[b1] = b2
			if b2 not in branches:
				branches[b2] = b1
			numbranch -= 1

	fin.close()
	return branches

def dominator2(root, graph, rgraph, dfs_order):

	change = True
	dom = {}
	
	for node in dfs_order:
		if node == root:
			dom[node] = set([node])
		else:
			dom[node] = set(dfs_order)

	while change:
		change = False
		
		for node in dfs_order:
			if node == root: continue
			t = set(dfs_order)
			for p in rgraph[node]:
				t = t & dom[p]
			t.add(node)
			if len(t ^ dom[node]) > 0:
				change = True
				dom[node] = t

	return dom

"""
def dominator1(nodes, root):
	global succ
	global pred
	
	change = True
	domin = {}

	# initialize dominator list
	for n in nodes:
		if n == root:
			domin[n] = set([n])
		else:
			domin[n] = set(nodes)

	n_loop = 0
	while change == True:
		n_loop += 1
		change = False
		for n in nodes:
			if n == root: continue
			t = set(nodes)
			for p in pred[n]:
				if p not in nodes:
					# this node is a head of dead code
					# it can't be reached
					continue
				t.intersection_update(domin[p])
			t.add(n)
			if len(t.symmetric_difference(domin[n])) > 0:
				change = True
				domin[n] = t
				
	#print "dominator loop iteration:", n_loop
	return domin
	#_print_dominator(nodes, domin)
"""


def dfs(node, g, dfs_nodes, dfs_visited):
	
	#print "dfs_visited:", node
	dfs_visited[node] = True
	dfs_nodes.append(node)

	for child in g[node]:
		if child not in dfs_visited:
			dfs(child, g, dfs_nodes, dfs_visited)

def calc_value(domin, nodes, branches, calls=None, fvalue=None):

	base_value = {}
	for n in nodes:
		if n not in base_value:
			base_value[n] = 0
		
		# 이 노드가 branch node 일 경우 기본 점수 1점 추가  
		if n in branches:
			base_value[n] += 1
			
		if fvalue: # fvalue 를 포함하는 모드 
			# 이 노드가 다른 함수를 콜 할 경우 그 함수 점수를 포함한다  
			if n in calls:
				for f in calls[n]:
					base_value[n] += fvalue[f]

	values = {}
	for n in nodes:

		for d in domin[n]:
			if d in values:
				values[d] += base_value[n]
			else:
				values[d] = base_value[n]

	return values


def build_graph2(filename):
	fin = open(filename)
	g = dict()
	for line in fin:
		cols = line.strip().split()
		cols = [int(x) for x in cols]
		g[cols[0]] = cols[1:]
	fin.close()
	
	sanity_check(g)
	
	return g

def reverse_graph(g):
	r = dict()
	sanity_check(g)
	for k in g.keys():
		r[k] = list()
	
	for node in g.keys():
		for child in g[node]:
			r[child].append(node) 
	
	return r

def sanity_check(g):
	allnodes = set()
	for k in g.keys():
		allnodes = allnodes | set(g[k])
	
	if len(allnodes - set(g.keys())) > 0:
		print "SANITY problem"
		print set(g.keys()) ^ allnodes
		sys.exit(0)
		


if __name__ == "__main__":
	if (sys.argv[1] == 'cfg_base'):
		calc_bscore('cfg_base')

	elif (sys.argv[1] == 'cfg_fcall'):
		calc_bscore('cfg_fcall')

	elif (sys.argv[1] == 'cfg_fvalue'):
		# for cfg_base + fvalue
		calc_bscore('cfg_base', fvalue=True)

	else:
		print "Please specify the mode: [cfg_base, cfg_fcall, cfg_fvalue]"

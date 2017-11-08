#!/usr/local/bin/python
# -*- coding: utf-8 -*-

import argparse
import sys
import pdb
import logging

TARGET_F_NAME = 'HM_TARGET'

class FunctionInfo:
  def __init__(self, fid, fname, entry_node):
    self.fid = fid # Function ID which is a number increasing from 1.
    self.fname = fname
    self.entry_node = entry_node # ID of the entry node in CFG.
    self.branches = {}
    self.covered_branches = set([])

  def set_branches(self, branches):
    self.branches = branches


class ProgramInfo:
  def __init__(self):
    self.flist = []
    self.branches = {}
    self.bpair = {}
    self.btofid = {}

  def build(self, path_to_branches, path_to_cfg_func_map):
    # Fill in basic function information such as fid, fname, entry_node.
    fid = 1
    with open(path_to_cfg_func_map) as fin:
      for line in fin:
        cols = line.strip().split()
        fname = cols[0]
        entry_node = int(cols[1])
        finfo = FunctionInfo(fid, fname, entry_node)
        self.flist.append(finfo)
        fid += 1

    # Fill in branch information.
    with open(path_to_branches) as fin:
      while 1:
        line = fin.readline()
        if not line:
          break
        cols = line.strip().split()
        fid = int(cols[0])
        numbranch = int(cols[1])

        fun_branches = {}
        while numbranch > 0:
          line = fin.readline()
          cols = line.strip().split()
          b1 = int(cols[0])
          b2 = int(cols[1])

          assert b1 not in self.branches
          assert b2 not in self.branches
          self.branches[b1] = True
          self.branches[b2] = True
          fun_branches[b1] = True
          fun_branches[b2] = True
          self.btofid[b1] = fid
          self.btofid[b2] = fid

          assert b1 not in self.bpair
          assert b2 not in self.bpair
          self.bpair[b1] = b2
          self.bpair[b2] = b1

          numbranch -= 1
        self.flist[fid-1].set_branches(fun_branches)

  def print_info(self):
    for finfo in self.flist:
      print finfo.fid, finfo.fname, finfo.entry_node, len(finfo.branches)

  def get_entry_nodes(self):
    # Return a list of entry_node of functions.
    entry_nodes = []
    for finfo in self.flist:
      entry_nodes.append(finfo.entry_node)
    return entry_nodes

  def get_fname(self, entry_node):
    for finfo in self.flist:
      if finfo.entry_node == entry_node:
        return finfo.fname

  def get_num_branches_f(self, entry_node):
    for finfo in self.flist:
      if finfo.entry_node == entry_node:
        return len(finfo.branches)

def compact_cfg(G, RG, branches):
  # Edge based compaction.
  # If a node has one outgoing edge and the successor node has one incoming edge
  # then we can merge the two nodes into one.
  logging.info('Number of nodes before compaction:%d', len(G))

  CHANGED = True
  while CHANGED:
    CHANGED = False
    for (node, succs) in G.items():
      if len(succs) == 1:
        succ = succs[0]
        if len(RG[succ]) == 1 and succ not in branches:
          G[node] = G[succ]
          del G[succ]
          CHANGED = True
          break

  logging.info('Number of nodes after compaction:%d', len(G))


def print_graph_info(G):
  print "Number of nodes: %d" %(len(G.keys()),)


def num_func(path_to_cfg, path_to_branches, path_to_cfg_func_map):
  # Print the number of functions having branches.
  pinfo = ProgramInfo()
  pinfo.build(path_to_branches, path_to_cfg_func_map)
  print len([function for function in pinfo.flist if len(function.branches) > 0])


def num_br_func(path_to_cfg, path_to_branches, path_to_cfg_func_map):
  # For each function, print the number of branches.
  pinfo = ProgramInfo()
  pinfo.build(path_to_branches, path_to_cfg_func_map)
  for func in sorted(pinfo.flist, key=lambda f: f.fname):
    print func.fname, len(func.branches)


def print_branch(path_to_cfg, path_to_branches, path_to_cfg_func_map):
  # Print all the branch ids.
  pinfo = ProgramInfo()
  pinfo.build(path_to_branches, path_to_cfg_func_map)
  for bid in sorted(pinfo.branches.keys()):
    print bid,


def cov_br_func(path_to_cfg, path_to_branches, path_to_cfg_func_map, path_to_covered_branches):
  # For each function, print the number of branches.
  pinfo = ProgramInfo()
  pinfo.build(path_to_branches, path_to_cfg_func_map)
  fin = open(path_to_covered_branches)
  for br in fin:
    # get funid of br
    funid = pinfo.btofid[int(br.strip())]
    pinfo.flist[funid-1].covered_branches.add(br)

  for func in sorted(pinfo.flist, key=lambda f: f.fname):
    print func.fname, "%d/%d" %(len(func.covered_branches), len(func.branches))


def calc_dominator(path_to_cfg, path_to_branches, path_to_cfg_func_map):
  # Calculate dominator and write dominator and dominator tree.
  pinfo = ProgramInfo()
  pinfo.build(path_to_branches, path_to_cfg_func_map)
  logging.info("Number of branches: %d", len(pinfo.branches))
  # pinfo.print_info()
  
  # Graph; key:node, value:list of successors.
  G = build_graph(path_to_cfg)
  logging.info("Number of nodes in ICFG: %d", len(G.keys()))
  
  # Due to cil transformation, unreachable nodes could be introduced.
  # Remove them.
  unreachable_nodes = find_unreachable_nodes(G, pinfo.get_entry_nodes())
  for node in unreachable_nodes:
    del G[node]
  logging.info("Number of unreachable nodes in ICFG: %d", len(unreachable_nodes))
  
  # Reversed graph; key:node, value:list of predecessors.
  RG = build_reverse_graph(G)

  compact_cfg(G, RG, pinfo.branches)
  sanity_check_compaction(G, set(pinfo.branches.keys()) - set(unreachable_nodes))
  RG = build_reverse_graph(G) # Rebuild reverse graph after compaction.
  
  # Find root nodes of disconnected graphs in G.
  root_nodes = find_root_nodes(RG, pinfo.get_entry_nodes())
  logging.info("Number of root nodes: %d", len(root_nodes))
  for root_node in root_nodes:
    logging.info("Root node: %s (%d)", pinfo.get_fname(root_node),
        pinfo.get_num_branches_f(root_node))

  dominator = {}
  # DFS order dominator
  for root in sorted(root_nodes):
    dfs_nodes = []
    dfs_visited = {}
    # get reachable nodes from a root
    dfs(root, G, dfs_nodes, dfs_visited)
    
    # make a sub graph
    sub_g = {}
    for node in dfs_nodes:
      sub_g[node] = G[node]

    sub_r = build_reverse_graph(sub_g)
    
    dom = dominator2(root, sub_g, sub_r, dfs_nodes)
    for k in dom.keys():
      if k not in dominator:
        dominator[k] = dom[k]
      else:
        dominator[k] =  dominator[k] | dom[k]

  print_dom(dominator, pinfo.branches)
  print_dom_tree(RG, dominator, pinfo.branches)
  #make_target_file(RG, pinfo)


def make_target_file(G, pinfo):
  #Find entry_node of target function.
  for fun in pinfo.flist:
    if fun.fname == TARGET_F_NAME:
      entry_node = fun.entry_node

  target_branches = []

  for node in G[entry_node]:
    t_branch = first_reachable_branches(G, pinfo, node)
    target_branches.append(t_branch)

  logging.info('Number of targets in CFG: %d', len(G[entry_node]))

  with open('target', 'w') as fout:
    for t_branch in target_branches:
      for t in t_branch:
        fout.write('%d ' %(t,))
      fout.write('\n')
  fout.close()


def first_reachable_branches(G, pinfo, node):
  #Find all first reachable branch in G from node.

  # If node itself is a branch then return it.
  if node in pinfo.branches:
    return [node]

  visited = set([node])
  q = [node]
  branches = []
  while q:
    node = q.pop(0)
    for succ in G[node]:
      if succ not in visited:
        visited.add(succ)
        if succ in pinfo.branches:
          branches.append(succ)
        else:
          q.append(succ)

  return branches


def sanity_check_compaction(G, branches):
  # Branch nodes must survive after compaction.
  for br in branches:
    if br not in G:
      print "Branch missing after compaction:", br


def print_dom(dom, branches):
  # dom includes dominator information for all nodes in ICFG.
  # Print dom informatin for branch nodes only.
  fout = open('dominator', 'w')
  for node in sorted(dom.keys()):
    if node not in branches: continue
    l = [str(x) for x in dom[node] if x in branches]
    fout.write("%s %s\n" %(node, ' '.join(l)) )
  fout.close()


def print_dom_tree(RG, dom, branches):
  # dom includes dominator information for all nodes in ICFG.
  # Print dominator tree information for branch nodes only.
  # For each branch, find immediate dominator along a DFS path in RG.
  fout = open('dominator_tree', 'w')
  for node in sorted(dom.keys()):
    if node not in branches: continue # This is not branch node.
    if len(dom[node]) == 1: continue # This branch has no dominator.
    DFSStack = [node]
    DFSVisited = {}
    while DFSStack:
      current_node = DFSStack.pop(-1)
      DFSVisited[current_node] = True
      if (current_node != node and current_node in branches 
          and current_node in dom[node]): 
        # Found the immediate dominator.
        fout.write("%s %s\n" %(node, current_node) )
        break
      for pred in RG[current_node]:
        if pred not in DFSVisited:
          DFSStack.append(pred)
  fout.close()


# functions for logging
def _print_branches(branches):
  for node in sorted(branches.keys()):
    print node, branches[node]


def _print_dominator(nodes, domin):
  for n in nodes:
    print n,":", sorted(domin[n])


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


def find_unreachable_nodes(G, entry_nodes):
  # Do DFS search and find all visited nodes.
  dfs_visited = dict()
  for entry_node in entry_nodes:
    if entry_node not in dfs_visited:
      _dfs(entry_node, G, dfs_visited)

  all_nodes = G.keys()
  return set(all_nodes) - set(dfs_visited.keys())

def _dfs(node, G, dfs_visited):
  assert node not in dfs_visited
  dfs_visited[node] = True

  for child in G[node]:
    if child not in dfs_visited:
      _dfs(child, G, dfs_visited)


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


def dfs(node, G, dfs_nodes, dfs_visited):
  #print "dfs_visited:", node
  dfs_visited[node] = True
  dfs_nodes.append(node)

  for child in G[node]:
    if child not in dfs_visited:
      dfs(child, G, dfs_nodes, dfs_visited)


def build_graph(path_to_cfg):
  fin = open(path_to_cfg)
  G = dict()
  for line in fin:
    cols = line.strip().split()
    cols = [int(x) for x in cols]
    G[cols[0]] = cols[1:]
  fin.close()
  
  sanity_check(G)
  return G


def build_reverse_graph(G):
  r = dict()
  sanity_check(G)
  for k in G.keys():
    r[k] = list()
  
  for node in G.keys():
    for child in G[node]:
      r[child].append(node) 
  
  sanity_check(r)
  return r


def sanity_check(G):
  allchildnodes = set()
  for k in G.keys():
    allchildnodes = allchildnodes | set(G[k])
  
  if len(allchildnodes - set(G.keys())) > 0:
    print "SANITY problem: Some nodes are not key of Graph."
    print set(G.keys()) ^ allchildnodes
    sys.exit(0)
    

if __name__ == "__main__":
  parser = argparse.ArgumentParser(description='Calculate dominators.')
  parser.add_argument("--cfg", default="./cfg", help="path to cfg file")
  parser.add_argument("--branches", default="./branches", help="path to branch file")
  parser.add_argument("--cfg_func_map", default="./cfg_func_map", help="path to cfg_func_map file")
  parser.add_argument("--covered_branches", default="./covered_branches", help="path to covered branches file")
  parser.add_argument("--log", default='INFO', choices=['INFO', 'DEBUG'], help="logging level")
  # Operational mode. "dominator" mode calculates dominators which is the default.
  # "numfunc" mode calculates the number of functions having branches.
  parser.add_argument("--mode", default='dominator',
      choices=[
        # Calculate dominator and write dominator and dominator tree.
        'dominator',
        # Print the number of functions having branches.
        'numfunc',
        # For each function, print the number of branches.
        'num_br_func',
        # Print the number of covered branches for each function.
        'cov_br_func',
        # Print branch id.
        'print_branch',
        ],
      help="Operational mode")
  
  args = parser.parse_args()
  logger = logging.getLogger('root')
  FORMAT = "[%(funcName)-20s] %(message)s"
  if args.log == 'INFO':
    logging.basicConfig(level=logging.INFO, format=FORMAT)
  elif args.log == 'DEBUG':
    logging.basicConfig(level=logging.DEBUG, format=FORMAT)

  if args.mode == 'dominator':
    calc_dominator(args.cfg, args.branches, args.cfg_func_map)
  elif args.mode == 'numfunc':
    num_func(args.cfg, args.branches, args.cfg_func_map)
  elif args.mode == 'num_br_func':
    num_br_func(args.cfg, args.branches, args.cfg_func_map)
  elif args.mode == 'cov_br_func':
    cov_br_func(args.cfg, args.branches, args.cfg_func_map, args.covered_branches)
  elif args.mode == 'print_branch':
    print_branch(args.cfg, args.branches, args.cfg_func_map)
  else:
    print 'Please select proper mode.'

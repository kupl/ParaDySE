// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <fstream>
#include <functional>
#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <utility>
#include <iostream>
#include <sstream>
#include <string>
#include <numeric>
#include <unistd.h>

#include <time.h>

#include "base/yices_solver.h"
#include "run_crest/concolic_search.h"

using std::binary_function;
using std::ifstream;
using std::ios;
using std::min;
using std::max;
using std::numeric_limits;
using std::pair;
using std::queue;
using std::random_shuffle;
using std::stable_sort;
using std::remove_if;
using std::cerr;
using std::endl;
using std::istringstream;

namespace crest {

namespace {

typedef pair<size_t,int> ScoredBranch;

struct ScoredBranchComp
: public binary_function<ScoredBranch, ScoredBranch, bool>
{
  bool operator()(const ScoredBranch& a, const ScoredBranch& b) const {
    return (a.second < b.second);
  }
};

}  // namespace


////////////////////////////////////////////////////////////////////////
//// Search ////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

Search::Search(const string& program, const string& input_file,
    const string& log_file, int max_iterations, int max_time) :
  num_smt_unsat_(0),
  num_smt_try_(0),
  program_(program),
  input_file_(input_file),
	log_file_(log_file),
  max_iters_(max_iterations),
  max_time_(max_time),
  num_iters_(0) {

  start_time_ = time(NULL);

  { // Read in the set of branches.
    max_branch_ = 0;
    max_function_ = 0;
    branches_.reserve(100000);
    branch_count_.reserve(100000);
    branch_count_.push_back(0);

    ifstream in("branches");
    assert(in);
    function_id_t fid;
    int numBranches;
    while (in >> fid >> numBranches) {
      branch_count_.push_back(2 * numBranches);
      branch_id_t b1, b2;
      for (int i = 0; i < numBranches; i++) {
        assert(in >> b1 >> b2);
        branches_.push_back(b1);
        branches_.push_back(b2);
        max_branch_ = max(max_branch_, max(b1, b2));
	fid_branches_map_[fid].insert(b1);
	fid_branches_map_[fid].insert(b2);
      }
    }
    in.close();
    max_branch_ ++;
    max_function_ = branch_count_.size();
  }
 
  // Compute the paired-branch map.
  paired_branch_.resize(max_branch_);
  for (size_t i = 0; i < branches_.size(); i += 2) {
    paired_branch_[branches_[i]] = branches_[i+1];
    paired_branch_[branches_[i+1]] = branches_[i];
  }

  // Compute the branch-to-function map.
  branch_function_.resize(max_branch_);
  { size_t i = 0;
  for (function_id_t j = 0; j < branch_count_.size(); j++) {
    for (size_t k = 0; k < branch_count_[j]; k++) {
      branch_function_[branches_[i++]] = j;
    }
  }
  }

  // Initialize all branches to "uncovered" (and functions to "unreached").
  total_num_covered_ = num_covered_ = 0;
  reachable_functions_ = reachable_branches_ = 0;
  covered_.resize(max_branch_, false);
  total_covered_.resize(max_branch_, false);
  reached_.resize(max_function_, false);
  // Remember how many times each bid got UNSAT.
  bid_unsat_count_.resize(max_branch_, 0);

#if 0
  { // Read in any previous coverage (for faster debugging).
    ifstream in("coverage");
    branch_id_t bid;
    while (in >> bid) {
      covered_[bid] = true;
      num_covered_ ++;
      if (!reached_[branch_function_[bid]]) {
        reached_[branch_function_[bid]] = true;
        reachable_functions_ ++;
        reachable_branches_ += branch_count_[branch_function_[bid]];
      }
    }

    total_num_covered_ = 0;
    total_covered_ = covered_;
  }
#endif

  // set a directory to save generated inputs into files
  /*
  struct tm* timeinfo = localtime ( &start_time_ );
  strftime (save_dir_, 100, "geninput/%Y%m%d_%H%M%S", timeinfo);

  char mk [150];
  sprintf(mk, "mkdir %s", save_dir_);
  system(mk);
  */

  // Print out the initial coverage.
  fprintf(stderr, "Iteration 0 (0s): covered %u branches [%u reach funs, %u reach branches].\n",
      num_covered_, reachable_functions_, reachable_branches_);

  // Sort the branches.
  sort(branches_.begin(), branches_.end());
}


Search::~Search() { }


void Search::SaveInput(const vector<value_t>& input, const char* str)
{
  char filename[100];
  sprintf(filename, "geninput/%s/input.%d", save_dir_, num_iters_ );

  FILE* f = fopen(filename, "w");
  if (!f) {
    fprintf(stderr, "Failed to open %s.\n", filename);
    perror("Error: ");
    exit(-1);
  }

  for (size_t i = 0; i < input.size(); i++) {
    fprintf(f, "%lld\n", input[i]);
  }

  fclose(f);

}

void Search::WriteInputToFileOrDie(const string& file,
           const vector<value_t>& input) {
  FILE* f = fopen(file.c_str(), "w");
  if (!f) {
    fprintf(stderr, "Failed to open %s.\n", file.c_str());
    perror("Error: ");
    exit(-1);
  }

  for (size_t i = 0; i < input.size(); i++) {
   fprintf(f, "%lld\n", input[i]);
  }

  fclose(f);

  // copy generated input into geninput directory
  //char cp [150];
  //sprintf(cp, "cp input %s/input.%d", save_dir_, num_iters_);
  //system(cp);

}


void Search::WriteCoverageToFileOrDie(const string& file) {
  FILE* f = fopen(file.c_str(), "w");
  if (!f) {
    fprintf(stderr, "Failed to open %s.\n", file.c_str());
    perror("Error: ");
    exit(-1);
  }
  for (BranchIt i = branches_.begin(); i != branches_.end(); ++i) {
    if (total_covered_[*i]) {
      fprintf(f, "%d\n", *i);
    }
  }

  fclose(f);
}

void Search::WriteCoverageFunToFileOrDie(const string& file) {
  FILE* f = fopen(file.c_str(), "w");
  if (!f) {
    fprintf(stderr, "Failed to open %s.\n", file.c_str());
    perror("Error: ");
    exit(-1);
  }

  //for (vector<bool>::const_iterator i = reached_.begin(); i != reached_.end(); ++i) {
  for( size_t i = 0; i < reached_.size(); i++ ) {
    if (reached_[i]) {
      fprintf(f, "%lu\n", i);
    }
  }

  fclose(f);
}

void Search::LaunchProgram(const vector<value_t>& inputs) {

  WriteInputToFileOrDie("input", inputs);
    /*
  pid_t pid = fork();
  assert(pid != -1);

  if (!pid) {
    system(program_.c_str());
    exit(0);
  }
  */
  system(program_.c_str());

  }

void Search::LaunchProgram(const vector<value_t>& inputs, const string& out_file)
{
	WriteInputToFileOrDie(out_file, inputs);

  system(program_.c_str());
}

void Search::InitialInput(vector<value_t>& inputs) {
  if (input_file_ == "random") return;
  if (num_iters_ != 0) return;

  // Read initial input.
  std::ifstream in(input_file_.c_str());
  value_t val;
  while (in >> val) {
    inputs.push_back(val);
  }
  in.close();

}


void Search::RunProgram(const vector<value_t>& inputs, SymbolicExecution* ex) {
  if ((++num_iters_ > max_iters_) || ((time(NULL)-start_time_) > max_time_)) {
    // TODO(jburnim): Devise a better system for capping the iterations.
    // Print out final log information
    PrintFinalResult();
    exit(0);
  }

/*
  for (size_t i = 0; i < inputs.size(); i++) {
    fprintf(stderr, "%lld\n", inputs[i]);
  }
*/
  // Run the program.
  LaunchProgram(inputs);

  // Read the execution from the program.
  // Want to do this with sockets.  (Currently doing it with files.)
  ifstream in("szd_execution", ios::in | ios::binary);
  assert(in && ex->Parse(in));
  in.close();

  /*
  for (size_t i = 0; i < ex->path().branches().size(); i++) {
    fprintf(stderr, "%d ", ex->path().branches()[i]);
  }
  fprintf(stderr, "\n");
  */
}

void Search::RunProgram(const vector<value_t>& inputs,
			SymbolicExecution* ex, const string& out_file)
{
  if ((++num_iters_ > max_iters_) || ((time(NULL)-start_time_) > max_time_)) {
		PrintFinalResult();
		exit(0);
	}
	LaunchProgram(inputs, out_file);

  ifstream in("szd_execution", ios::in | ios::binary);
  assert(in && ex->Parse(in));
  in.close();

}


void Search::CheckTarget(const int tbid) {
  if (covered_[tbid]) {
    fprintf(stdout, "Iteration %d (%lds): Target Covered.\n",
        num_iters_, time(NULL)-start_time_);
    exit(0);
  }
}

bool Search::UpdateCoverage(const SymbolicExecution& ex) {
  return UpdateCoverage(ex, NULL);
}

bool Search::UpdateCoverage(const SymbolicExecution& ex,
    set<branch_id_t>* new_branches) {

	newly_covered_branches_.clear();
	FILE *f = fopen(log_file_.c_str(), "a");
	if (!f) {
		fprintf(stderr, "Failed to open %s. \n", log_file_.c_str());
		exit(-1);
	}

	latest_covered_fid_.clear();
  const unsigned int prev_covered_ = num_covered_;
  const vector<branch_id_t>& branches = ex.path().branches();
  for (BranchIt i = branches.begin(); i != branches.end(); ++i) {
    if ((*i > 0) && !covered_[*i]) {
      covered_[*i] = true;
      num_covered_++;
			newly_covered_branches_.insert(*i);
      if (new_branches) {
        new_branches->insert(*i);
      }
      if (!reached_[branch_function_[*i]]) {
        reached_[branch_function_[*i]] = true;
        reachable_functions_ ++;
        reachable_branches_ += branch_count_[branch_function_[*i]];
				latest_covered_fid_.insert(branch_function_[*i]);
      }
    }
    if ((*i > 0) && !total_covered_[*i]) {
      total_covered_[*i] = true;
      total_num_covered_++;
    }
  }

  //Gather Uncovered fun_id 
  uncovered_function_id_.clear();
  for (BranchIt i = branches_.begin(); i != branches_.end(); ++i) {
  	function_id_t fun_id = branch_function_[*i]; 
	if(!reached_[fun_id]){
   		uncovered_function_id_.insert(fun_id);
	}
  }
  //Calcuate uncovered_fid_bidsize and Sort
  unc_fid_bidsize_.clear();
  for(auto it = uncovered_function_id_.begin(); it != uncovered_function_id_.end(); ++it){
	int bid_size = fid_branches_map_[*it].size();
	unc_fid_bidsize_.push_back(pair<int, function_id_t>(bid_size, *it));
  }
  sort(unc_fid_bidsize_.begin(), unc_fid_bidsize_.end());
  if(unc_fid_bidsize_.size() > 0){ //Handing Exceptional Case. 
  	size_t top10 = unc_fid_bidsize_.size() /10 ;
  	size_t top20 = unc_fid_bidsize_.size() /5 ;
  	size_t top30 = unc_fid_bidsize_.size() /3 ;
    auto top_it = unc_fid_bidsize_.end();
    top10_threshold_ = (top_it-top10)->first;
    top20_threshold_ = (top_it-top20)->first;
    top30_threshold_ = (top_it-top30)->first;
  }
  else{
  	top10_threshold_ = 0;
    top20_threshold_ = 0;
    top30_threshold_ = 0;
  }		  
		   
  fprintf(stderr, "Iteration %d (%lds): covered %u branches [%u reach funs, %u reach branches].\n",
      num_iters_, time(NULL)-start_time_, total_num_covered_, reachable_functions_, reachable_branches_);

	// Write coverage to the log file
	fprintf(f, "It: %d, Time: %ld, covered: %u, reach_func: %u, reach_branch: %u\n",
		num_iters_, time(NULL)-start_time_, total_num_covered_, reachable_functions_, reachable_branches_);
	
	fclose(f);
		
  bool found_new_branch = (num_covered_ > prev_covered_);
  if (found_new_branch) {
    WriteCoverageToFileOrDie("coverage");
  }

  return found_new_branch;
}


void Search::RandomInput(const map<var_t,type_t>& vars, vector<value_t>* input) {
  input->resize(vars.size());

  for (map<var_t,type_t>::const_iterator it = vars.begin(); it != vars.end(); ++it) {
    unsigned long long val = 0;
    for (size_t j = 0; j < 8; j++)
      val = (val << 8) + (rand() / 256);


    switch (it->second) {
    case types::U_CHAR:
      input->at(it->first) = (unsigned char)val; break;
    case types::CHAR:
      input->at(it->first) = (char)val; break;
    case types::U_SHORT:
      input->at(it->first) = (unsigned short)val; break;
    case types::SHORT:
      input->at(it->first) = (short)val; break;
    case types::U_INT:
      input->at(it->first) = (unsigned int)val; break;
    case types::INT:
      input->at(it->first) = (int)val; break;
    case types::U_LONG:
      input->at(it->first) = (unsigned long)val; break;
    case types::LONG:
      input->at(it->first) = (long)val; break;
    case types::U_LONG_LONG:
      input->at(it->first) = (unsigned long long)val; break;
    case types::LONG_LONG:
      input->at(it->first) = (long long)val; break;
    }
  }
}

value_t Search::GetOneRandomInput(type_t t) {

  unsigned long long val = 0;
  /*
  for (size_t j = 0; j < 8; j++)
    val = (val << 8) + (rand() / 256);
  */
  val = rand();

  switch (t) {
  case types::U_CHAR:
    return (unsigned char)val; break;
  case types::CHAR:
    return (char)val; break;
  case types::U_SHORT:
    return (unsigned short)val; break;
  case types::SHORT:
    return (short)val; break;
  case types::U_INT:
    return (unsigned int)val; break;
  case types::INT:
    return (int)val; break;
  case types::U_LONG:
    return (unsigned long)val; break;
  case types::LONG:
    return (long)val; break;
  case types::U_LONG_LONG:
    return (unsigned long long)val; break;
  case types::LONG_LONG:
    return (long long)val; break;
  }
  // Control should not reach here.
  assert(false);
  return val;
}

bool Search::SolveAtBranchNew(const SymbolicExecution& ex, size_t branch_idx, 
    vector<value_t>* input) {
  bool solved = SolveAtBranch(ex, branch_idx, input);
  if (!solved)
    return solved;

  set<var_t> used_vars;
  const vector<SymbolicPred*>& constraints = ex.path().constraints();

  for (size_t i = 0; i <= branch_idx; i++) {
    constraints[i]->AppendVars(&used_vars);
  }

  set<value_t>::iterator it;
  for (var_t var = 0; var < input->size(); var++) {
    if (used_vars.count(var) == 0)
      (*input)[var] = GetOneRandomInput(ex.vars().find(var)->second);
  }
  return solved;
}

bool Search::SolveAtBranch(const SymbolicExecution& ex,
                           size_t branch_idx,
                           vector<value_t>* input) {

  num_smt_try_++;
  const vector<SymbolicPred*>& constraints = ex.path().constraints();

  // Optimization: If any of the previous constraints are idential to the
  // branch_idx-th constraint, immediately return false.
  for (int i = static_cast<int>(branch_idx) - 1; i >= 0; i--) {
    if (constraints[branch_idx]->Equal(*constraints[i])) {
      num_smt_unsat_++;
      return false;
    }
  }

  vector<const SymbolicPred*> cs(constraints.begin(),
         constraints.begin()+branch_idx+1);
  map<var_t,value_t> soln;
  constraints[branch_idx]->Negate();
  // fprintf(stderr, "Yices . . . ");
  bool success = YicesSolver::IncrementalSolve(ex.inputs(), ex.vars(), cs, &soln);
  // fprintf(stderr, "%d\n", success);
  constraints[branch_idx]->Negate();

  if (success) {
    // Merge the solution with the previous input to get the next
    // input.  (Could merge with random inputs, instead.)
    *input = ex.inputs();
    // RandomInput(ex.vars(), input);

    typedef map<var_t,value_t>::const_iterator SolnIt;
    for (SolnIt i = soln.begin(); i != soln.end(); ++i) {
      (*input)[i->first] = i->second;
    }
    return true;
  }
  num_smt_unsat_++;
  branch_id_t bid;
  bid = ex.path().branches()[ex.path().constraints_idx()[branch_idx]];
  bid_unsat_count_[paired_branch_[bid]]++;

  return false;
}

bool Search::SolveAtBranchOnly(const SymbolicExecution& ex,
                           size_t branch_idx,
                           vector<value_t>* input) {

  const vector<SymbolicPred*>& constraints = ex.path().constraints();

  vector<const SymbolicPred*> cs(constraints.begin()+branch_idx,
         constraints.begin()+branch_idx+1);
  map<var_t,value_t> soln;
  constraints[branch_idx]->Negate();
  // fprintf(stderr, "Yices . . . ");
  bool success = YicesSolver::IncrementalSolve(ex.inputs(), ex.vars(), cs, &soln);
  // fprintf(stderr, "%d\n", success);
  constraints[branch_idx]->Negate();

  if (success) {
    // Merge the solution with the previous input to get the next
    // input.  (Could merge with random inputs, instead.)
    *input = ex.inputs();
    // RandomInput(ex.vars(), input);

    typedef map<var_t,value_t>::const_iterator SolnIt;
    for (SolnIt i = soln.begin(); i != soln.end(); ++i) {
      (*input)[i->first] = i->second;
    }
    return true;
  }

  return false;
}

bool Search::CheckPrediction(const SymbolicExecution& old_ex,
           const SymbolicExecution& new_ex,
           size_t branch_idx) {

  if ((old_ex.path().branches().size() <= branch_idx)
      || (new_ex.path().branches().size() <= branch_idx)) {
    return false;
  }

   for (size_t j = 0; j < branch_idx; j++) {
     if  (new_ex.path().branches()[j] != old_ex.path().branches()[j]) {
       return false;
     }
   }
   return (new_ex.path().branches()[branch_idx]
           == paired_branch_[old_ex.path().branches()[branch_idx]]);
}

void Search::PrintFinalResult()
{
	FILE *f = fopen(log_file_.c_str(), "a");
	if (!f) {
		fprintf(stderr, "Failed to open %s, \n", log_file_.c_str());
		exit(-1);
	}

  fprintf(stderr, "NumSMTUNSAT:  %u / %u \n",
      num_smt_unsat_, num_smt_try_);

  fprintf(stderr, "Covered Branches: ");
  for (BranchIt i = branches_.begin(); i != branches_.end(); ++i) {
    if (total_covered_[*i]) {
      fprintf(stderr, "%d ", *i);
      // fprintf(stdout, "%d\n", *i);
    }
  }
  fprintf(stderr, "\n");

  fprintf(stderr, "Branch UNSAT Cnt: ");
  for (BranchIt i = branches_.begin(); i != branches_.end(); ++i) {
    if (bid_unsat_count_[*i] > 0)
      fprintf(stderr, "%d:%d ", *i, bid_unsat_count_[*i]);
  }
  fprintf(stderr, "\n");

	// Write the final result to the log file
	fprintf(f, "NumSMTUNSAT:  %u / %u \n",
      num_smt_unsat_, num_smt_try_);

  fprintf(f, "Covered Branches: ");
  for (BranchIt i = branches_.begin(); i != branches_.end(); ++i) {
    if (total_covered_[*i]) {
      fprintf(f, "%d ", *i);
    }
  }
  fprintf(f, "\n");

  fprintf(f, "Branch UNSAT Cnt: ");
  for (BranchIt i = branches_.begin(); i != branches_.end(); ++i) {
    if (bid_unsat_count_[*i] > 0)
      fprintf(f, "%d:%d ", *i, bid_unsat_count_[*i]);
  }
  fprintf(f, "\n");
	
	fclose(f);

  WriteCoverageFunToFileOrDie("covered_functions");
  WriteCoverageToFileOrDie("covered_branches");
}
int Search::GetNumIters()
{
	return num_iters_;
}
////////////////////////////////////////////////////////////////////////
//// BoundedDepthFirstSearch ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

BoundedDepthFirstSearch::BoundedDepthFirstSearch(
    const string& program, const string& input_file, const string& log_file,
		int max_iterations, int max_time, int max_depth)
  : Search(program, input_file, log_file, max_iterations, max_time), max_depth_(max_depth) { }

BoundedDepthFirstSearch::~BoundedDepthFirstSearch() { }

void BoundedDepthFirstSearch::Run() {
  while(true) {
    // Initial execution (on empty/random inputs).
    vector<value_t> input;
    InitialInput(input);

    SymbolicExecution ex;
    RunProgram(input, &ex);
    UpdateCoverage(ex);

    DFS(0, max_depth_, ex);
    // DFS(0, ex);
  }
}

  /*
void BoundedDepthFirstSearch::DFS(int depth, SymbolicExecution& prev_ex) {
  SymbolicExecution cur_ex;
  vector<value_t> input;

  const SymbolicPath& path = prev_ex.path();

  int last = min(max_depth_, static_cast<int>(path.constraints().size()) - 1);
  for (int i = last; i >= depth; i--) {
    // Solve constraints[0..i].
    if (!SolveAtBranch(prev_ex, i, &input)) {
      continue;
    }

    // Run on those constraints.
    RunProgram(input, &cur_ex);
    UpdateCoverage(cur_ex);

    // Check for prediction failure.
    size_t branch_idx = path.constraints_idx()[i];
    if (!CheckPrediction(prev_ex, cur_ex, branch_idx)) {
      fprintf(stderr, "Prediction failed!\n");
      continue;
    }

    // Recurse.
    DFS(i+1, cur_ex);
  }
}
  */


void BoundedDepthFirstSearch::DFS(size_t pos, int depth, SymbolicExecution& prev_ex) {
  SymbolicExecution cur_ex;
  vector<value_t> input;

  const SymbolicPath& path = prev_ex.path();
  for (size_t i = pos; (i < path.constraints().size()) && (depth > 0); i++) {
    // Solve constraints[0..i].
    if (!SolveAtBranch(prev_ex, i, &input)) {
      continue;
    }

    // Run on those constraints.
    RunProgram(input, &cur_ex);
    UpdateCoverage(cur_ex);

    // Check for prediction failure.
    size_t branch_idx = path.constraints_idx()[i];
    if (!CheckPrediction(prev_ex, cur_ex, branch_idx)) {
      fprintf(stderr, "Prediction failed!\n");
      continue;
    }

    // We successfully solved the branch, recurse.
    depth--;
    DFS(i+1, depth, cur_ex);
  }
}


////////////////////////////////////////////////////////////////////////
//// RandomInputSearch /////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

RandomInputSearch::RandomInputSearch(
    const string& program, const string& input_file,
    const string& log_file, int max_iterations, int max_time)
  : Search(program, input_file, log_file, max_iterations, max_time) { }

RandomInputSearch::~RandomInputSearch() { }

void RandomInputSearch::Run() {
  vector<value_t> input;
  InitialInput(input);

  RunProgram(input, &ex_);

  while (true) {
    RandomInput(ex_.vars(), &input);
    RunProgram(input, &ex_);
    UpdateCoverage(ex_);
  }
}

////////////////////////////////////////////////////////////////////////
//// RandomSearch //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

RandomSearch::RandomSearch(const string& program, const string& input_file,
    const string& log_file, int max_iterations, int max_time)
  : Search(program, input_file, log_file, max_iterations, max_time) { }

RandomSearch::~RandomSearch() { }

void RandomSearch::Run() {
  SymbolicExecution next_ex;

  while (true) {
    // Execution (on empty/random inputs).
    fprintf(stderr, "RESET\n");
    vector<value_t> next_input;
    InitialInput(next_input);

    RunProgram(next_input, &ex_);
    UpdateCoverage(ex_);

    // Do some iterations.
    int count = 0;
    while (count++ < 10000) {
      // fprintf(stderr, "Uncovered bounded DFS.\n");
      // SolveUncoveredBranches(0, 20, ex_);

      size_t idx;
      if (SolveRandomBranch(&next_input, &idx)) {
        
		RunProgram(next_input, &next_ex);
        bool found_new_branch = UpdateCoverage(next_ex);
        bool prediction_failed =
            !CheckPrediction(ex_, next_ex, ex_.path().constraints_idx()[idx]);

        if (found_new_branch) {
          count = 0;
          ex_.Swap(next_ex);
          if (prediction_failed)
            fprintf(stderr, "Prediction failed (but got lucky).\n");
        } else if (!prediction_failed) {
          ex_.Swap(next_ex);
        } else {
          fprintf(stderr, "Prediction failed.\n");
        }
      }
    }
  }
}

  /*
bool RandomSearch::SolveUncoveredBranches(SymbolicExecution& prev_ex) {
  SymbolicExecution cur_ex;
  vector<value_t> input;

  bool success = false;

  int cnt = 0;

  for (size_t i = 0; i < prev_ex.path().constraints().size(); i++) {

    size_t bid_idx = prev_ex.path().constraints_idx()[i];
    branch_id_t bid = prev_ex.path().branches()[bid_idx];
    if (covered_[paired_branch_[bid]])
      continue;

    if (!SolveAtBranch(prev_ex, i, &input)) {
      if (++cnt == 1000) {
  cnt = 0;
  fprintf(stderr, "Failed to solve at %u/%u.\n",
    i, prev_ex.path().constraints().size());
      }
      continue;
    }
    cnt = 0;

    RunProgram(input, &cur_ex);
    UpdateCoverage(cur_ex);
    if (!CheckPrediction(prev_ex, cur_ex, bid_idx)) {
      fprintf(stderr, "Prediction failed.\n");
      continue;
    }

    success = true;
    cur_ex.Swap(prev_ex);
  }

  return success;
}
  */

void RandomSearch::SolveUncoveredBranches(size_t i, int depth,
    const SymbolicExecution& prev_ex) {
  if (depth < 0)
    return;

  fprintf(stderr, "position: %zu/%zu (%d)\n",
      i, prev_ex.path().constraints().size(), depth);

  SymbolicExecution cur_ex;
  vector<value_t> input;

  int cnt = 0;

  for (size_t j = i; j < prev_ex.path().constraints().size(); j++) {
    size_t bid_idx = prev_ex.path().constraints_idx()[j];
    branch_id_t bid = prev_ex.path().branches()[bid_idx];
    if (covered_[paired_branch_[bid]])
      continue;

    if (!SolveAtBranch(prev_ex, j, &input)) {
      if (++cnt == 1000) {
        cnt = 0;
        fprintf(stderr, "Failed to solve at %zu/%zu.\n",
            j, prev_ex.path().constraints().size());
      }
      continue;
    }
    cnt = 0;

    RunProgram(input, &cur_ex);
    UpdateCoverage(cur_ex);
    if (!CheckPrediction(prev_ex, cur_ex, bid_idx)) {
      fprintf(stderr, "Prediction failed.\n");
      continue;
    }

    SolveUncoveredBranches(j+1, depth-1, cur_ex);
  }
}


bool RandomSearch::SolveRandomBranch(vector<value_t>* next_input, size_t* idx) {
  /*
  const SymbolicPath& p = ex_.path();
  vector<ScoredBranch> zero_branches, other_branches;
  zero_branches.reserve(p.constraints().size());
  other_branches.reserve(p.constraints().size());

  vector<size_t> idxs(p.constraints().size());
  for (size_t i = 0; i < idxs.size(); i++) {
    idxs[i] = i;
  }
  random_shuffle(idxs.begin(), idxs.end());

  vector<int> seen(max_branch_);
  for (vector<size_t>::const_iterator i = idxs.begin(); i != idxs.end(); ++i) {
    branch_id_t bid = p.branches()[p.constraints_idx()[*i]];
    if (!covered_[paired_branch_[bid]]) {
      zero_branches.push_back(make_pair(*i, seen[bid]));
    } else {
      other_branches.push_back(make_pair(*i, seen[bid]));
    }
    seen[bid] += 1;
  }

  stable_sort(zero_branches.begin(), zero_branches.end(), ScoredBranchComp());

  int tries = 1000;
  for (size_t i = 0; (i < zero_branches.size()) && (tries > 0); i++, tries--) {
    if (SolveAtBranch(ex_, zero_branches[i].first, next_input))
      return;
  }

  stable_sort(other_branches.begin(), other_branches.end(), ScoredBranchComp());
  for (size_t i = 0; (i < other_branches.size()) && (tries > 0); i++, tries--) {
    if (SolveAtBranch(ex_, other_branches[i].first, next_input))
      return;
  }
   */

  vector<size_t> idxs(ex_.path().constraints().size());
  for (size_t i = 0; i < idxs.size(); i++)
    idxs[i] = i;

  for (int tries = 0; tries < 1000; tries++) {
    // Pick a random index.
    if (idxs.size() == 0)
      break;
    size_t r = rand() % idxs.size();
    size_t i = idxs[r];
    swap(idxs[r], idxs.back());
    idxs.pop_back();

    if (SolveAtBranch(ex_, i, next_input)) {
      fprintf(stderr, "Solved %zu/%zu\n", i, idxs.size());
      *idx = i;
      return true;
    }
  }

  // We failed to solve a branch, so reset the input.
  fprintf(stderr, "FAIL\n");
  next_input->clear();
  return false;
}


////////////////////////////////////////////////////////////////////////
//// RandomSearch //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

UniformRandomSearch::UniformRandomSearch(const string& program,
           const string& input_file,
           const string& log_file,
           int max_iterations,
           int max_time,
           size_t max_depth)
  : Search(program, input_file, log_file, max_iterations, max_time), max_depth_(max_depth) { }

UniformRandomSearch::~UniformRandomSearch() { }

void UniformRandomSearch::Run() {
  // Initial execution (on empty/random inputs).
  vector<value_t> input;
  InitialInput(input);

  RunProgram(input, &prev_ex_);
  UpdateCoverage(prev_ex_);

  while (true) {
    fprintf(stderr, "RESET\n");

    // Uniform random path.
    DoUniformRandomPath();
  }
}

void UniformRandomSearch::DoUniformRandomPath() {
  vector<value_t> input;

  size_t i = 0;
  size_t depth = 0;
  fprintf(stderr, "%zu constraints.\n", prev_ex_.path().constraints().size());
  while ((i < prev_ex_.path().constraints().size()) && (depth < max_depth_)) {
    if (SolveAtBranch(prev_ex_, i, &input)) {
      fprintf(stderr, "Solved constraint %zu/%zu.\n",
          (i+1), prev_ex_.path().constraints().size());
      depth++;

      // With probability 0.5, force the i-th constraint.
      if (rand() % 2 == 0) {
        RunProgram(input, &cur_ex_);
        UpdateCoverage(cur_ex_);
        size_t branch_idx = prev_ex_.path().constraints_idx()[i];
        if (!CheckPrediction(prev_ex_, cur_ex_, branch_idx)) {
          fprintf(stderr, "prediction failed\n");
          depth--;
        } else {
          cur_ex_.Swap(prev_ex_);
        }
      }
    }

    i++;
  }
}


////////////////////////////////////////////////////////////////////////
//// HybridSearch //////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

HybridSearch::HybridSearch(const string& program, const string& input_file,
    const string& log_file, int max_iterations, int max_time, int step_size)
  : Search(program, input_file, log_file, max_iterations, max_time), step_size_(step_size) { }

HybridSearch::~HybridSearch() { }

void HybridSearch::Run() {
  SymbolicExecution ex;

  while (true) {
    // Execution on empty/random inputs.
    vector<value_t> input;
    InitialInput(input);
    RunProgram(input, &ex);
    UpdateCoverage(ex);

    // Local searches at increasingly deeper execution points.
    for (size_t pos = 0; pos < ex.path().constraints().size(); pos += step_size_) {
      RandomLocalSearch(&ex, pos, pos+step_size_);
    }
  }
}

void HybridSearch::RandomLocalSearch(SymbolicExecution *ex, size_t start, size_t end) {
  for (int iters = 0; iters < 100; iters++) {
    if (!RandomStep(ex, start, end))
      break;
  }
}

bool HybridSearch::RandomStep(SymbolicExecution *ex, size_t start, size_t end) {

  if (end > ex->path().constraints().size()) {
    end = ex->path().constraints().size();
  }
  assert(start < end);

  SymbolicExecution next_ex;
  vector<value_t> input;

  fprintf(stderr, "%zu-%zu\n", start, end);
  vector<size_t> idxs(end - start);
  for (size_t i = 0; i < idxs.size(); i++) {
    idxs[i] = start + i;
  }

  for (int tries = 0; tries < 1000; tries++) {
    // Pick a random index.
    if (idxs.size() == 0)
      break;
    size_t r = rand() % idxs.size();
    size_t i = idxs[r];
    swap(idxs[r], idxs.back());
    idxs.pop_back();

    if (SolveAtBranch(*ex, i, &input)) {
      RunProgram(input, &next_ex);
      UpdateCoverage(next_ex);
      if (CheckPrediction(*ex, next_ex, ex->path().constraints_idx()[i])) {
        ex->Swap(next_ex);
        return true;
      }
    }
  }

  return false;
}


////////////////////////////////////////////////////////////////////////
//// CfgBaselineSearch /////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

CfgBaselineSearch::CfgBaselineSearch(const string& program,
    const string& input_file, 
		const string& log_file,
		int max_iterations, int max_time)
  : Search(program, input_file, log_file, max_iterations, max_time) { }

CfgBaselineSearch::~CfgBaselineSearch() { }


void CfgBaselineSearch::Run() {
  SymbolicExecution ex;

  while (true) {
    // Execution on empty/random inputs.
    fprintf(stderr, "RESET\n");
    vector<value_t> input;
    InitialInput(input);

    RunProgram(input, &ex);
    UpdateCoverage(ex);

    while (DoSearch(5, 250, 0, ex)) {
      // As long as we keep finding new branches . . . .
      ex.Swap(success_ex_);
    }
  }
}


bool CfgBaselineSearch::DoSearch(int depth, int iters, int pos,
         const SymbolicExecution& prev_ex) {

  // For each symbolic branch/constraint in the execution path, we will
  // compute a heuristic score, and then attempt to force the branches
  // in order of increasing score.
  vector<ScoredBranch> scoredBranches(prev_ex.path().constraints().size() - pos);
  for (size_t i = 0; i < scoredBranches.size(); i++) {
    scoredBranches[i].first = i + pos;
  }

  { // Compute (and sort by) the scores.
    random_shuffle(scoredBranches.begin(), scoredBranches.end());
    map<branch_id_t,int> seen;
    for (size_t i = 0; i < scoredBranches.size(); i++) {
      size_t idx = scoredBranches[i].first;
      size_t branch_idx = prev_ex.path().constraints_idx()[idx];
      branch_id_t bid = paired_branch_[prev_ex.path().branches()[branch_idx]];
      if (covered_[bid]) {
  scoredBranches[i].second = 100000000 + seen[bid];
      } else {
  scoredBranches[i].second = seen[bid];
      }
      seen[bid] += 1;
    }
  }
  stable_sort(scoredBranches.begin(), scoredBranches.end(), ScoredBranchComp());

  // Solve.
  SymbolicExecution cur_ex;
  vector<value_t> input;
  for (size_t i = 0; i < scoredBranches.size(); i++) {
    if (iters <= 0) {
      return false;
    }

    if (!SolveAtBranch(prev_ex, scoredBranches[i].first, &input)) {
      continue;
    }

    RunProgram(input, &cur_ex);
    iters--;

    if (UpdateCoverage(cur_ex, NULL)) {
      success_ex_.Swap(cur_ex);
      return true;
    }
  }

  return false;
}


////////////////////////////////////////////////////////////////////////
//// CfgHeuristicSearch ////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

CfgHeuristicSearch::CfgHeuristicSearch(const string& program,
    const string& input_file, 
		const string& log_file, int max_iterations, int max_time)
  : Search(program, input_file, log_file, max_iterations, max_time),
    cfg_(max_branch_), cfg_rev_(max_branch_), dist_(max_branch_) {

  // Read in the CFG.
  ifstream in("cfg_branches", ios::in | ios::binary);
  assert(in);
  size_t num_branches;
  in.read((char*)&num_branches, sizeof(num_branches));
  assert(num_branches == branches_.size());
  for (size_t i = 0; i < num_branches; i++) {
    branch_id_t src;
    size_t len;
    in.read((char*)&src, sizeof(src));
    in.read((char*)&len, sizeof(len));
    cfg_[src].resize(len);
    in.read((char*)&cfg_[src].front(), len * sizeof(branch_id_t));
  }
  in.close();

  // Construct the reversed CFG.
  for (BranchIt i = branches_.begin(); i != branches_.end(); ++i) {
    for (BranchIt j = cfg_[*i].begin(); j != cfg_[*i].end(); ++j) {
      cfg_rev_[*j].push_back(*i);
    }
  }
}


CfgHeuristicSearch::~CfgHeuristicSearch() { }

void CfgHeuristicSearch::ReadCFG(const vector<branch_id_t>& branches,
		vector<vector<branch_id_t> >& cfg)
{
  ifstream in("cfg_branches", ios::in | ios::binary);
  assert(in);
  size_t num_branches;
  in.read((char*)&num_branches, sizeof(num_branches));
  assert(num_branches == branches.size());
  for (size_t i = 0; i < num_branches; i++) {
    branch_id_t src;
    size_t len;
    in.read((char*)&src, sizeof(src));
    in.read((char*)&len, sizeof(len));
    cfg[src].resize(len);
    in.read((char*)&cfg[src].front(), len * sizeof(branch_id_t));
  }
  in.close();
}

void CfgHeuristicSearch::Run() {
  set<branch_id_t> newly_covered_;
  SymbolicExecution ex;

  while (true) {
    covered_.assign(max_branch_, false);
    num_covered_ = 0;

    // Execution on empty/random inputs.
    fprintf(stderr, "RESET\n");
    vector<value_t> input;
    InitialInput(input);

    RunProgram(input, &ex);
    if (UpdateCoverage(ex)) {
      UpdateBranchDistances();
      PrintStats();
    }

    // while (DoSearch(3, 200, 0, kInfiniteDistance+10, ex)) {
    while (DoSearch(5, 30, 0, kInfiniteDistance, ex)) {
    // while (DoSearch(3, 10000, 0, kInfiniteDistance, ex)) {
      PrintStats();
      // As long as we keep finding new branches . . . .
      UpdateBranchDistances();
      ex.Swap(success_ex_);
    }
    PrintStats();
  }
}


void CfgHeuristicSearch::PrintStats() {
  return;
  fprintf(stderr, "Cfg solves: %u/%u (%u lucky [%u continued], %u on 0's, %u on others,"
  "%u unsats, %u prediction failures)\n",
    (num_inner_lucky_successes_ + num_inner_zero_successes_ + num_inner_nonzero_successes_ + num_top_solve_successes_),
    num_inner_solves_, num_inner_lucky_successes_, (num_inner_lucky_successes_ - num_inner_successes_pred_fail_),
    num_inner_zero_successes_, num_inner_nonzero_successes_,
    num_inner_unsats_, num_inner_pred_fails_);
  fprintf(stderr, "    (recursive successes: %u)\n", num_inner_recursive_successes_);
  fprintf(stderr, "Top-level SolveAlongCfg: %u/%u\n",
    num_top_solve_successes_, num_top_solves_);
  fprintf(stderr, "All SolveAlongCfg: %u/%u  (%u all concrete, %u no paths)\n",
    num_solve_successes_, num_solves_,
    num_solve_all_concrete_, num_solve_no_paths_);
  fprintf(stderr, "    (sat failures: %u/%u)  (prediction failures: %u) (recursions: %u)\n",
    num_solve_unsats_, num_solve_sat_attempts_,
    num_solve_pred_fails_, num_solve_recurses_);
}


void CfgHeuristicSearch::UpdateBranchDistances() {
  // We run a BFS backward, starting simultaneously at all uncovered vertices.
  queue<branch_id_t> Q;
  for (BranchIt i = branches_.begin(); i != branches_.end(); ++i) {
    if (!covered_[*i]) {
      dist_[*i] = 0;
      Q.push(*i);
    } else {
      dist_[*i] = kInfiniteDistance;
    }
  }

  while (!Q.empty()) {
    branch_id_t i = Q.front();
    size_t dist_i = dist_[i];
    Q.pop();

    for (BranchIt j = cfg_rev_[i].begin(); j != cfg_rev_[i].end(); ++j) {
      if (dist_i + 1 < dist_[*j]) {
  dist_[*j] = dist_i + 1;
  Q.push(*j);
      }
    }
  }
}


bool CfgHeuristicSearch::DoSearch(int depth,
          int iters,
          int pos,
          int maxDist,
          const SymbolicExecution& prev_ex) {

  /*fprintf(stderr, "DoSearch(%d, %d, %d, %zu)\n",
    depth, pos, maxDist, prev_ex.path().branches().size());
  */

  if (pos >= static_cast<int>(prev_ex.path().constraints().size()))
    return false;

  if (depth == 0)
    return false;


  // For each symbolic branch/constraint in the execution path, we will
  // compute a heuristic score, and then attempt to force the branches
  // in order of increasing score.
  vector<ScoredBranch> scoredBranches(prev_ex.path().constraints().size() - pos);
  for (size_t i = 0; i < scoredBranches.size(); i++) {
    scoredBranches[i].first = i + pos;
  }

  { // Compute (and sort by) the scores.
    random_shuffle(scoredBranches.begin(), scoredBranches.end());
    map<branch_id_t,int> seen;
    for (size_t i = 0; i < scoredBranches.size(); i++) {
      size_t idx = scoredBranches[i].first;
      size_t branch_idx = prev_ex.path().constraints_idx()[idx];
      branch_id_t bid = paired_branch_[prev_ex.path().branches()[branch_idx]];

      scoredBranches[i].second = dist_[bid] + seen[bid];
      seen[bid] += 1;

      /*
      if (dist_[bid] == 0) {
        scoredBranches[i].second = 0;
      } else {
        scoredBranches[i].second = dist_[bid] + seen[bid];
        seen[bid] += 1;
      }
      */
    }
  }
  stable_sort(scoredBranches.begin(), scoredBranches.end(), ScoredBranchComp());

  // Solve.
  SymbolicExecution cur_ex;
  vector<value_t> input;
  for (size_t i = 0; i < scoredBranches.size(); i++) {
	
	if ((iters <= 0) || (scoredBranches[i].second > maxDist))
      return false;

    num_inner_solves_ ++;

    if (!SolveAtBranch(prev_ex, scoredBranches[i].first, &input)) {
      num_inner_unsats_ ++;
      continue;
    }
   	RunProgram(input, &cur_ex);
    iters--;

    size_t b_idx = prev_ex.path().constraints_idx()[scoredBranches[i].first];
    branch_id_t bid = paired_branch_[prev_ex.path().branches()[b_idx]];
    set<branch_id_t> new_branches;
    bool found_new_branch = UpdateCoverage(cur_ex, &new_branches);
    bool prediction_failed = !CheckPrediction(prev_ex, cur_ex, b_idx);

    if (found_new_branch && prediction_failed) {
      /*
      fprintf(stderr, "Prediction failed.\n");
      fprintf(stderr, "Found new branch by forcing at "
                "distance %zu (%d) [lucky, pred failed].\n",
        dist_[bid], scoredBranches[i].second);
      */

      // We got lucky, and can't really compute any further stats
      // because prediction failed.
      num_inner_lucky_successes_ ++;
      num_inner_successes_pred_fail_ ++;
      success_ex_.Swap(cur_ex);
      return true;
    }

    if (found_new_branch && !prediction_failed) {
      /*
      fprintf(stderr, "Found new branch by forcing at distance %zu (%d).\n",
        dist_[bid], scoredBranches[i].second);
      */
      size_t min_dist = MinCflDistance(b_idx, cur_ex, new_branches);
      // Check if we were lucky.
      if (FindAlongCfg(b_idx, dist_[bid], cur_ex, new_branches)) {
  assert(min_dist <= dist_[bid]);
  // A legitimate find -- return success.
  if (dist_[bid] == 0) {
    num_inner_zero_successes_ ++;
  } else {
    num_inner_nonzero_successes_ ++;
  }
  success_ex_.Swap(cur_ex);
  return true;
      } else {
  // We got lucky, but as long as there were no prediction failures,
  // we'll finish the CFG search to see if that works, too.
  assert(min_dist > dist_[bid]);
  assert(dist_[bid] != 0);
  num_inner_lucky_successes_ ++;
      }
    }

    if (prediction_failed) {
      //fprintf(stderr, "Prediction failed.\n");
      if (!found_new_branch) {
  num_inner_pred_fails_ ++;
  continue;
      }
    }

    // If we reached here, then scoredBranches[i].second is greater than 0.
    num_top_solves_ ++;
    if ((dist_[bid] > 0) &&
        SolveAlongCfg(b_idx, scoredBranches[i].second-1, cur_ex)) {
      num_top_solve_successes_ ++;
      PrintStats();
      return true;
    }

    if (found_new_branch) {
      success_ex_.Swap(cur_ex);
      return true;
    }

    /*
    if (DoSearch(depth-1, 5, scoredBranches[i].first+1, scoredBranches[i].second-1, cur_ex)) {
      num_inner_recursive_successes_ ++;
      return true;
    }
    */
  }
  return false;
}


size_t CfgHeuristicSearch::MinCflDistance
(size_t i, const SymbolicExecution& ex, const set<branch_id_t>& bs) {

  const vector<branch_id_t>& p = ex.path().branches();

  if (i >= p.size())
    return numeric_limits<size_t>::max();

  if (bs.find(p[i]) != bs.end())
    return 0;

  vector<size_t> stack;
  size_t min_dist = numeric_limits<size_t>::max();
  size_t cur_dist = 1;

  //fprintf(stderr, "Found uncovered branches at distances:");
  for (BranchIt j = p.begin()+i+1; j != p.end(); ++j) {
    if (bs.find(*j) != bs.end()) {
      min_dist = min(min_dist, cur_dist);
      //fprintf(stderr, " %zu", cur_dist);
    }

    if (*j >= 0) {
      cur_dist++;
    } else if (*j == kCallId) {
      stack.push_back(cur_dist);
    } else if (*j == kReturnId) {
      if (stack.size() == 0)
  break;
      cur_dist = stack.back();
      stack.pop_back();
    } else {
      //fprintf(stderr, "\nBad branch id: %d\n", *j);
      exit(1);
    }
  }

  //fprintf(stderr, "\n");
  return min_dist;
}

bool CfgHeuristicSearch::FindAlongCfg(size_t i, unsigned int dist,
              const SymbolicExecution& ex,
              const set<branch_id_t>& bs) {

  const vector<branch_id_t>& path = ex.path().branches();

  if (i >= path.size())
    return false;

  branch_id_t bid = path[i];
  if (bs.find(bid) != bs.end())
    return true;

  if (dist == 0)
    return false;

  // Compute the indices of all branches on the path that immediately
  // follow the current branch (corresponding to the i-th constraint)
  // in the CFG. For example, consider the path:
  //     * ( ( ( 1 2 ) 4 ) ( 5 ( 6 7 ) ) 8 ) 9
  // where '*' is the current branch.  The branches immediately
  // following '*' are : 1, 4, 5, 8, and 9.
  vector<size_t> idxs;
  { size_t pos = i + 1;
    CollectNextBranches(path, &pos, &idxs);
  }

  for (vector<size_t>::const_iterator j = idxs.begin(); j != idxs.end(); ++j) {
    if (FindAlongCfg(*j, dist-1, ex, bs))
      return true;
  }

  return false;
}


bool CfgHeuristicSearch::SolveAlongCfg(size_t i, unsigned int max_dist,
    const SymbolicExecution& prev_ex) {
  num_solves_ ++;

  //fprintf(stderr, "SolveAlongCfg(%zu,%u)\n", i, max_dist);
  SymbolicExecution cur_ex;
  vector<value_t> input;
  const vector<branch_id_t>& path = prev_ex.path().branches();

  // Compute the indices of all branches on the path that immediately
  // follow the current branch (corresponding to the i-th constraint)
  // in the CFG. For example, consider the path:
  //     * ( ( ( 1 2 ) 4 ) ( 5 ( 6 7 ) ) 8 ) 9
  // where '*' is the current branch.  The branches immediately
  // following '*' are : 1, 4, 5, 8, and 9.
  bool found_path = false;
  vector<size_t> idxs;
  { size_t pos = i + 1;
  CollectNextBranches(path, &pos, &idxs);
  // fprintf(stderr, "Branches following %d:", path[i]);
  for (size_t j = 0; j < idxs.size(); j++) {
    // fprintf(stderr, " %d(%u,%u,%u)", path[idxs[j]], idxs[j],
    //      dist_[path[idxs[j]]], dist_[paired_branch_[path[idxs[j]]]]);
    if ((dist_[path[idxs[j]]] <= max_dist)
        || (dist_[paired_branch_[path[idxs[j]]]] <= max_dist))
      found_path = true;
  }
  //fprintf(stderr, "\n");
  }

  if (!found_path) {
    num_solve_no_paths_ ++;
    return false;
  }

  bool all_concrete = true;
  num_solve_all_concrete_ ++;

  // We will iterate through these indices in some order (random?
  // increasing order of distance? decreasing?), and try to force and
  // recurse along each one with distance no greater than max_dist.
  random_shuffle(idxs.begin(), idxs.end());
  for (vector<size_t>::const_iterator j = idxs.begin(); j != idxs.end(); ++j) {
    // Skip if distance is wrong.
    if ((dist_[path[*j]] > max_dist)
        && (dist_[paired_branch_[path[*j]]] > max_dist)) {
      continue;
    }

    if (dist_[path[*j]] <= max_dist) {
      // No need to force, this branch is on a shortest path.
      num_solve_recurses_ ++;
      if (SolveAlongCfg(*j, max_dist-1, prev_ex)) {
        num_solve_successes_ ++;
        return true;
      }
    }

    // Find the constraint corresponding to branch idxs[*j].
    vector<size_t>::const_iterator idx =
        lower_bound(prev_ex.path().constraints_idx().begin(),
            prev_ex.path().constraints_idx().end(), *j);
    if ((idx == prev_ex.path().constraints_idx().end()) || (*idx != *j)) {
      continue;  // Branch is concrete.
    }
    size_t c_idx = idx - prev_ex.path().constraints_idx().begin();

    if (all_concrete) {
      all_concrete = false;
      num_solve_all_concrete_ --;
    }

    if(dist_[paired_branch_[path[*j]]] <= max_dist) {
      num_solve_sat_attempts_ ++;
      // The paired branch is along a shortest path, so force.
      if (!SolveAtBranch(prev_ex, c_idx, &input)) {
        num_solve_unsats_ ++;
        continue;
      }
      RunProgram(input, &cur_ex);
      if (UpdateCoverage(cur_ex)) {
        num_solve_successes_ ++;
        success_ex_.Swap(cur_ex);
        return true;
      }
      if (!CheckPrediction(prev_ex, cur_ex, *j)) {
        num_solve_pred_fails_ ++;
        continue;
      }

      // Recurse.
      num_solve_recurses_ ++;
      if (SolveAlongCfg(*j, max_dist-1, cur_ex)) {
        num_solve_successes_ ++;
        return true;
      }
    }
  }

  return false;
}

void CfgHeuristicSearch::SkipUntilReturn(const vector<branch_id_t> path, size_t* pos) {
  while ((*pos < path.size()) && (path[*pos] != kReturnId)) {
    if (path[*pos] == kCallId) {
      (*pos)++;
      SkipUntilReturn(path, pos);
      if (*pos >= path.size())
  return;
      assert(path[*pos] == kReturnId);
    }
    (*pos)++;
  }
}

void CfgHeuristicSearch::CollectNextBranches
(const vector<branch_id_t>& path, size_t* pos, vector<size_t>* idxs) {
  // fprintf(stderr, "Collect(%u,%u,%u)\n", path.size(), *pos, idxs->size());

  // Eat an arbitrary sequence of call-returns, collecting inside each one.
  while ((*pos < path.size()) && (path[*pos] == kCallId)) {
    (*pos)++;
    CollectNextBranches(path, pos, idxs);
    SkipUntilReturn(path, pos);
    if (*pos >= path.size())
      return;
    assert(path[*pos] == kReturnId);
    (*pos)++;
  }

  // If the sequence of calls is followed by a branch, add it.
  if ((*pos < path.size()) && (path[*pos] >= 0)) {
    idxs->push_back(*pos);
    (*pos)++;
    return;
  }

  // Alternatively, if the sequence is followed by a return, collect the branches
  // immediately following the return.
  /*
  if ((*pos < path.size()) && (path[*pos] == kReturnId)) {
    (*pos)++;
    CollectNextBranches(path, pos, idxs);
  }
  */
}


bool CfgHeuristicSearch::DoBoundedBFS(int i, int depth, const SymbolicExecution& prev_ex) {
  if (depth <= 0)
    return false;

  /*
  fprintf(stderr, "%d (%d: %d) (%d: %d)\n", depth,
          i-1, prev_ex.path().branches()[prev_ex.path().constraints_idx()[i-1]],
          i, prev_ex.path().branches()[prev_ex.path().constraints_idx()[i]]);
         */ 

  SymbolicExecution cur_ex;
  vector<value_t> input;
  const vector<SymbolicPred*>& constraints = prev_ex.path().constraints();
  for (size_t j = static_cast<size_t>(i); j < constraints.size(); j++) {
    if (!SolveAtBranch(prev_ex, j, &input)) {
      continue;
    }

    RunProgram(input, &cur_ex);
    iters_left_--;
    if (UpdateCoverage(cur_ex)) {
      success_ex_.Swap(cur_ex);
      return true;
    }

    if (!CheckPrediction(prev_ex, cur_ex, prev_ex.path().constraints_idx()[j])) {
      fprintf(stderr, "Prediction failed!\n");
      continue;
    }

    return (DoBoundedBFS(j+1, depth-1, cur_ex)
      || DoBoundedBFS(j+1, depth-1, prev_ex));
  }

  return false;
}

////////////////////////////////////////////////////////////////////////
//// ContextGuidedSearch(CGS) ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/* 
  The implementation of CGS came from the author of FSE'14 paper.
*/ 
void CGSSymbolicExecution::InitTried() { 
  tried_.resize(path().constraints_idx().size(), false);
  return;
}

void CGSSymbolicExecution::SetTried(int cidx) { 
  tried_[cidx] = true;
  return;
}

bool CGSSymbolicExecution::Tried(int cidx) {
  return tried_[cidx];
}

vector<branch_id_t> CGSSymbolicExecution::GetContext(int cidx, size_t k, map< branch_id_t, set<branch_id_t> > &dominator) {
	vector<branch_id_t> context;
	const vector<branch_id_t>& branches = path().branches();
	const vector<size_t>& cidxs = path().constraints_idx();

	int bidx = cidxs[cidx];
	branch_id_t bid = branches[bidx];
  	set<branch_id_t> dom = dominator[bid];

  	context.push_back(bid);
  	bidx--;

  	while (bidx >= 0) {
    	if (context.size() >= k) 
     		break;
    	if (dom.count(branches[bidx]) == 0)
      		context.push_back(branches[bidx]);
    	bidx--;
  	}
  	return context;
/*  original CGS
  vector<branch_id_t> context;
  vector<branch_id_t> branches = path().branches();

  int bidx = path().constraints_idx()[cidx];
  branch_id_t bid = branches[bidx];
  set<branch_id_t> dom = dominator[bid];

  context.push_back(bid);
  bidx--;

  while (bidx >= 0) {
    if (context.size() >= k) 
      break;
    if (dom.count(branches[bidx]) == 0)
      context.push_back(branches[bidx]);
    bidx--;
  }
  return context;
*/
}

ContextGuidedSearch::ContextGuidedSearch(
    const string& program,
		const string& input_file,
		const string& log_file,
		int max_iterations, int max_time, int max_k,
		const string& dom_file)
  : Search(program, input_file, log_file, max_iterations, max_time), max_k_(max_k) {
    ex_tree_.reserve(max_iterations);

    // Read in dominator and save. (if it is given)
		if (!dom_file.empty())
			ReadDominator(dom_file, dominator_);
}


ContextGuidedSearch::~ContextGuidedSearch() { }

void ContextGuidedSearch::ReadDominator(const string& dom_file,
		map<branch_id_t, set<branch_id_t> >& dominator)
{	
    ifstream in(dom_file.c_str());
    assert(in);

	dominator.clear();
    string line;
    while (getline(in, line)) {
      istringstream line_in(line);
      branch_id_t src;
      line_in >> src;
      line_in.get();

      set<branch_id_t> dom;
      while(line_in) {
        branch_id_t dsc;
        line_in >> dsc;
        dom.insert(dsc);
        line_in.get();
      }
      assert(dominator.count(src) == 0);
      dominator[src] = dom;
    }
    in.close();
}


void ContextGuidedSearch::Run() {
  while (true) {
    DoCGS();
    fprintf(stderr, "CGS: RESET\n");
  }
  
}

void ContextGuidedSearch::DoCGS() {
  // Initial execution (on empty/random inputs).
  vector<value_t> input;
  InitialInput(input);
  ex_tree_.clear();
  // Clear context cache or not?
  // While testing grep with optional arguments, do not clear.
  context_cache_.clear();

  int ex_no = 0;
  size_t k = 1;

  CGSSymbolicExecution* prev_ex;
  CGSSymbolicExecution* ex = new CGSSymbolicExecution();
  RunProgram(input, ex);
  ex->InitTried();
  ex->ex_no_ = ex_no++;
  ex->div_bidx_ = -1;
  ex_tree_.push_back(ex);
  UpdateCoverage(*ex);

  map<branch_id_t, size_t> bid_history;
  while (k <= max_k_) {
    int depth = 0;
    while (depth < GetMaxDepthT()) {
      vector<int> exs_at_depth;
      GetExAtDepth(exs_at_depth, depth);
     // fprintf(stderr, "CGS: Context: %d Depth: %d Candidates:%d\n", k, depth, exs_at_depth.size());
      std::random_shuffle(exs_at_depth.begin(), exs_at_depth.end());
      for (vector<int>::iterator it = exs_at_depth.begin(); it != exs_at_depth.end(); ++it) {
        prev_ex = ex_tree_[*it];
        if (prev_ex->Tried(depth)) continue;
        vector<branch_id_t> context = prev_ex->GetContext(depth, k, dominator_);
        if (context_cache_.count(context) > 0) continue;

        prev_ex->SetTried(depth);
        context_cache_.insert(context);
        if (!SolveAtBranchNew(*prev_ex, depth, &input)) {
          continue;
        }
        size_t branch_idx = prev_ex->path().constraints_idx()[depth];
        size_t bbid = prev_ex->path().branches()[branch_idx];
       
	printf(" ex_no, bid, %d, %lu\n", prev_ex->ex_no_, bbid);
	
	ex = new CGSSymbolicExecution();
        
	RunProgram(input, ex);
        UpdateCoverage(*ex);
	if (!CheckPrediction(*prev_ex, *ex, branch_idx)) continue;
   	
	ex->InitTried();
        ex->ex_no_ = ex_no++;
        ex->div_bidx_ = branch_idx;
        ex_tree_.push_back(ex);
      }
      depth++;
    }

    k++;
  }
  
}


int ContextGuidedSearch::GetMaxDepthT() {
  int max = 0;
  int depth;
  vector<CGSSymbolicExecution*>::iterator it;
  CGSSymbolicExecution* ex;
  for(it = ex_tree_.begin(); it != ex_tree_.end(); ++it) {
    ex = *it;
    depth = ex->path().constraints().size();
    if (depth > max) max = depth;
    
  }
  return max;
}

void ContextGuidedSearch::GetExAtDepth(vector<int>& executions, int depth) {
  vector<CGSSymbolicExecution*>::iterator it;
  CGSSymbolicExecution* ex;
  vector<size_t> cidxs;
  for(it = ex_tree_.begin(); it != ex_tree_.end(); ++it) {
    ex = *it;
    if (ex->Tried(depth)) 
      continue;
    cidxs = ex->path().constraints_idx();
    if (((long)cidxs.size() > depth) && ((long)cidxs[depth] > ex->div_bidx_)) {
      executions.push_back(ex->ex_no_);
    }
    
  }
  return;
}
////////////////////////////////////////////////////////////////////////
//// Generational Search ///////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
/* 
  The implementation of Generational Search also came from the author of FSE'14 paper.
*/ 

GenerationalSearch::GenerationalSearch(const string& program,
    const string& input_file, 
		const string& log_file,
		int max_iterations, int max_time)
  : Search(program, input_file, log_file, max_iterations, max_time) {}

GenerationalSearch::~GenerationalSearch() {}


void GenerationalSearch::Run() {

  while(true) {
    DoGenerational();
    fprintf(stderr, "Generational: RESET\n");
  }

}

void GenerationalSearch::DoGenerational() {
  vector<value_t> input;
  InitialInput(input);
  vector<GenTarget> targets;

  SymbolicExecution* ex = new SymbolicExecution();
  RunProgram(input, ex);
  UpdateCoverage(*ex);
  GenTarget t = {ex, 0, 0};
  targets.push_back(t);

  while (!targets.empty()) {
    stable_sort(targets.begin(), targets.end(), GenTargetComp);
    GenTarget target = targets.back();
    targets.pop_back();
    for (size_t cidx = target.cidx; cidx < (target.ex)->path().constraints().size(); cidx++) {
      if (!SolveAtBranchNew(*(target.ex), cidx, &input)) {
        continue;
      }
      SymbolicExecution* new_ex = new SymbolicExecution();
      RunProgram(input, new_ex);
      set<branch_id_t> new_branches;
      UpdateCoverage(*new_ex, &new_branches);
      if (CheckPrediction(*(target.ex), *new_ex, (target.ex)->path().constraints_idx()[cidx])){
        GenTarget t = {new_ex, cidx+1, new_branches.size()};
        targets.push_back(t);
      }
    }

  }
  
}

bool GenerationalSearch::GenTargetComp(const struct GenTarget &a, const struct GenTarget &b){
  return a.score < b.score;
}


////////////////////////////////////////////////////////////////////////
//// Testing Search ////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
TestingSearch::TestingSearch(const string& program, 
		const string& out_file, int max_iterations, int max_time)
	: Search(program, out_file, "/dev/null", max_iterations, max_time), out_file_(out_file) { }

TestingSearch::~TestingSearch() { }

void TestingSearch::Run() {
	vector<value_t> input;
	InitialInput(input);
	SymbolicExecution ex;
	RunProgram(input, &ex, out_file_);

	for (size_t i = 0; i < ex.path().constraints().size(); ++i) {
		if (!SolveAtBranch(ex, i, &input))
			continue;

		RunProgram(input, &ex, out_file_);

		fprintf(stderr, "###Testing search result:\n");
		fprintf(stderr, "Solved branch_ID: %d\n", ex.path().branches()[ex.path().constraints_idx()[i]]);
		fprintf(stderr, "Path: \n\t");
		for (size_t idx = 0; idx < ex.path().constraints_idx().size(); ++idx) {
			fprintf(stderr, "%d ", ex.path().branches()[ex.path().constraints_idx()[idx]]);
		} fprintf(stderr, "\n");

		return;
	}
}

////////////////////////////////////////////////////////////////////////
//// Parameterized Search //////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

ParameterizedSearch::ParameterizedSearch(const string& prog_name,
		const string& input_file, const string& log_file,
		int max_iterations, int max_time,
		const string& weight_file)
	: Search(prog_name, input_file, log_file, max_iterations, max_time),
		cfg_(max_branch_) {

		const string& dom_file = "dominator";
		const string& static_feature_file = "features";
		const string& w_file = weight_file;
		
		// Read CFG & dominator
		CfgHeuristicSearch::ReadCFG(branches_, cfg_);
		ContextGuidedSearch::ReadDominator(dom_file, dominator_);

		// Read static feature map
		ifstream fin(static_feature_file.c_str());
		assert(fin);
		string line;
		while (getline(fin, line)) {
			istringstream line_in(line);
			branch_id_t bid;
			vector<int> fv;

			line_in >> bid;
			line_in.ignore().ignore();
			while (line_in) {
				int pred;
				line_in >> pred;
				fv.push_back(pred);
			} fv.pop_back();

			static_feature_map_.insert(make_pair(bid, fv));
		}

		// Read weight
		double weight;
		ifstream win(w_file.c_str());
		assert(win);
		while (win >> weight) {
			weights_.push_back(weight);
		}
	
 		dy_features_.push_back(new IsFrontBranch());
		dy_features_.push_back(new IsEndBranch());
		dy_features_.push_back(new IsMostFrequentlyAppearedInPath());
		dy_features_.push_back(new IsLeastFrequentlyAppearedInPath());
		dy_features_.push_back(new IsFreshBranch(newly_covered_branches_));
		dy_features_.push_back(new IsFreshPartOfPath());
		dy_features_.push_back(new IsVisitedContext(1, dominator_));
		dy_features_.push_back(new IsVisitedContext(2, dominator_));
		dy_features_.push_back(new IsVisitedContext(3, dominator_));
		dy_features_.push_back(new IsVisitedContext(4, dominator_));
		dy_features_.push_back(new IsVisitedContext(5, dominator_));
		dy_features_.push_back(new IsFrequentlySelectedPair(10));
		dy_features_.push_back(new IsFrequentlySelectedPair(20));
		dy_features_.push_back(new IsFrequentlySelectedPair(30));
		dy_features_.push_back(new IsNearNegatedBefore());
		dy_features_.push_back(new IsFrequentlyUnsatBranch(bid_unsat_count_, paired_branch_, false));
		dy_features_.push_back(new IsFrequentlyUnsatBranch(bid_unsat_count_, paired_branch_, true));
		dy_features_.push_back(new IsPairedBranchUncovered(cfg_, total_covered_, paired_branch_, branches_, 0));
		dy_features_.push_back(new IsPairedBranchUncovered(cfg_, total_covered_, paired_branch_, branches_, 1));
		dy_features_.push_back(new IsRecentlySelectedBranch(10));
		dy_features_.push_back(new IsRecentlySelectedBranch(20));
		dy_features_.push_back(new IsRecentlySelectedBranch(30));
		dy_features_.push_back(new IsInMostUncoveredFunc(total_covered_, branch_function_, fid_branches_map_));
		dy_features_.push_back(new IsNearUncovered_Function(cfg_, total_covered_, paired_branch_, branches_, 2, fid_branches_map_, uncovered_function_id_, top10_threshold_));
		dy_features_.push_back(new IsNearUncovered_Function(cfg_, total_covered_, paired_branch_, branches_, 2, fid_branches_map_, uncovered_function_id_, top20_threshold_));
		dy_features_.push_back(new IsNearUncovered_Function(cfg_, total_covered_, paired_branch_, branches_, 2, fid_branches_map_, uncovered_function_id_, top30_threshold_));
		dy_features_.push_back(new IsNearUncovered_Function(cfg_, total_covered_, paired_branch_, branches_, 2, fid_branches_map_, uncovered_function_id_, 10));
		dy_features_.push_back(new IsInLatestCoveredFunc(branch_function_, fid_branches_map_, latest_covered_fid_));
		assert(weights_.size() == dy_features_.size() + 12);
}
		
ParameterizedSearch::~ParameterizedSearch() {}

void ParameterizedSearch::Run()
{

	while (true) {
			fprintf(stderr, "First run\n");
			SymbolicExecution initial_ex;
			vector<value_t> input;
			InitialInput(input);
			RunProgram(input, &initial_ex);
			UpdateCoverage(initial_ex);
			DoSearch(initial_ex);
			while (DoSearch(new_ex_)) {}
	}

}

bool ParameterizedSearch::DoSearch(SymbolicExecution& ex)
{
	const auto& cidxs = ex.path().constraints_idx();
	const auto& branches = ex.path().branches();

	const auto& fv_map = ExtractFeatures(ex);
	auto candidates = ComputeScores(fv_map);
	
	int trial = 0;
	while (!candidates.empty()) {
		double score = candidates.back().first;
		size_t cidx = candidates.back().second;
		candidates.pop_back();
		trial++;

		branch_id_t bid = branches[cidxs[cidx]];

		if (bid_unsat_counter_[bid] > 70)
			continue;
               
		vector<value_t> new_input;
		if(!SolveAtBranch(ex, cidx, &new_input)) {
			bid_unsat_counter_[bid]++;
			continue;
		}

		if(inputs_tried_.count(new_input) > 0) {
			continue;
		}
		inputs_tried_.insert(new_input);
		
		fprintf(stderr, "CIDX: %lu, BID: %d, SCORE: %lf TRIAL: %d\n", cidx, bid, score, trial);

  		SymbolicExecution cur_ex;
		RunProgram(new_input, &cur_ex);
		UpdateCoverage(cur_ex);
		bool prediction_failure = CheckPrediction(ex, cur_ex, cidxs[cidx]);
		UpdateFeatureState(GenExecStatus(&ex, cidx, bid, prediction_failure));
	    new_ex_.Swap(cur_ex);	
		return true;
	}
	return false;
}

void ParameterizedSearch::UpdateFeatureState(const ExecStatus& ex_stat)
{
	for (auto it = dy_features_.begin(); it != dy_features_.end(); ++it) {
		(*it)->UpdateFeatureState(ex_stat);
	}
}
map< size_t, vector<int> >
ParameterizedSearch::ExtractFeatures(const SymbolicExecution& ex)
{
	map< size_t, vector<int> > fvmap;
	const auto& cidxs = ex.path().constraints_idx();
	const auto& branches = ex.path().branches();

	for (auto fit = dy_features_.begin(); fit != dy_features_.end(); ++fit) {
		if ((*fit)->IsReadyToCompute())
			(*fit)->ComputeFeature(ex);
	}

	for (size_t cidx = 0; cidx < cidxs.size(); ++cidx) {
		branch_id_t bid = branches[cidxs[cidx]];
		/* Static features */
		vector<int> fvector = static_feature_map_.find(bid)->second;

		/* Dynamic features */
		for (auto fit = dy_features_.begin(); fit != dy_features_.end(); ++fit)
			fvector.push_back((*fit)->Predicate(cidx));

		fvmap.insert(pair< size_t, vector<int> >(cidx, fvector));
	}

	return fvmap;
}


vector< pair<double, size_t> >
ParameterizedSearch::ComputeScores(const map< size_t, vector<int> >& fvmap)
{
	vector< pair<double, size_t> > scored_idxs;
	
	for (auto it = fvmap.begin(); it != fvmap.end(); ++it) {
		size_t cidx = it->first;
		vector<int> fv = it->second;
	        

		double score = inner_product(fv.begin(), fv.end(), weights_.begin(), 0.0);
		scored_idxs.push_back(pair<double, size_t>(score, cidx));
	}
	std::sort(scored_idxs.begin(), scored_idxs.end());
	
		return scored_idxs;
}


}

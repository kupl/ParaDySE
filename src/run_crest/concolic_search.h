
//// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#ifndef RUN_CREST_CONCOLIC_SEARCH_H__
#define RUN_CREST_CONCOLIC_SEARCH_H__

#include <map>
#include <vector>
//#include <ext/hash_map>
//#include <ext/hash_set>
#include <time.h>

/*
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
*/

#include "base/basic_types.h"
#include "base/symbolic_execution.h"
#include "run_crest/feature.h"

using std::map;
using std::vector;
using std::set;
using std::pair;
//using __gnu_cxx::hash_map;
//using __gnu_cxx::hash_set;

namespace crest {

class DynamicFeature;

class Search {
 public:
 	// For the testing search
  Search(const string& program, const string& input_file,
      const string& log_file, int max_iterations, int max_time);
  virtual ~Search();

  virtual void Run() = 0;

 protected:
  vector<branch_id_t> branches_;
  vector<branch_id_t> paired_branch_;
  vector<function_id_t> branch_function_;
	map<function_id_t, set<branch_id_t> > fid_branches_map_;
	set<function_id_t> latest_covered_fid_;
  vector<bool> covered_;
  vector<bool> total_covered_;
	set<branch_id_t> newly_covered_branches_;
 	set<function_id_t> uncovered_function_id_;
	int threshold_;
	int top10_threshold_;
	int top20_threshold_;
	int top30_threshold_;
	double total;
  branch_id_t max_branch_;
  unsigned int num_covered_;
  unsigned int total_num_covered_;
  vector<pair<int, function_id_t> > fid_bidsize_;  
  vector<pair<int, function_id_t> > unc_fid_bidsize_;  
  unsigned int init_coverage_;
  map<branch_id_t, size_t> bid_history;  
  map<int, size_t> b_unsat_count;
  //CGS
  map<int, size_t> selected_ex_no;
  size_t ex_idx;
  vector<bool> reached_;
  vector<unsigned int> branch_count_;
  function_id_t max_function_;
  unsigned int reachable_functions_;
  unsigned int reachable_branches_;

  unsigned int num_smt_unsat_;
  unsigned int num_smt_try_;
  vector<int> bid_unsat_count_;

  int num_iters_;
  time_t start_time_;

  typedef vector<branch_id_t>::const_iterator BranchIt;

  bool SolveAtBranch(const SymbolicExecution& ex,
		     size_t branch_idx,
		     vector<value_t>* input);

  bool SolveAtBranchOnly(const SymbolicExecution& ex,
		     size_t branch_idx,
		     vector<value_t>* input);

  bool SolveAtBranchNew(const SymbolicExecution& ex,
		     size_t branch_idx,
		     vector<value_t>* input);

  bool CheckPrediction(const SymbolicExecution& old_ex,
		       const SymbolicExecution& new_ex,
		       size_t branch_idx);

  void RunProgram(const vector<value_t>& inputs, SymbolicExecution* ex);
  void RunProgram(const vector<value_t>& inputs, 
					SymbolicExecution* ex, const string& out_file);
  bool UpdateCoverage(const SymbolicExecution& ex);
  bool UpdateCoverage(const SymbolicExecution& ex,
		      set<branch_id_t>* new_branches);

  void CheckTarget(const int tbid);
  void RandomInput(const map<var_t,type_t>& vars, vector<value_t>* input);
  value_t GetOneRandomInput(type_t type);

  void InitialInput(vector<value_t>& inputs);
  void PrintFinalResult();

  void SaveInput(const vector<value_t>& input, const char* str);
  int GetNumIters();

 private:
  const string program_;
  const string input_file_; // initial input file
  const string log_file_; // log file
  const int max_iters_; 
  const int max_time_;  // maximum tims in seconds.

  char save_dir_[100];					// directory to save generated inputs


  /*
  struct sockaddr_un sock_;
  int sockd_;
  */

  void WriteInputToFileOrDie(const string& file, const vector<value_t>& input);
  void WriteCoverageToFileOrDie(const string& file);
  void WriteCoverageFunToFileOrDie(const string& file);
  void LaunchProgram(const vector<value_t>& inputs);
  void LaunchProgram(const vector<value_t>& inputs, const string& out_file);
};


class BoundedDepthFirstSearch : public Search {
 public:
  explicit BoundedDepthFirstSearch(const string& program,
  		 	 	 const string& input_file,
  		 	 	 const string& log_file,
				   int max_iterations,
           int max_time,
				   int max_depth);
  virtual ~BoundedDepthFirstSearch();

  virtual void Run();

 private:
  int max_depth_;

  void DFS(size_t pos, int depth, SymbolicExecution& prev_ex);
};


/*
class OldDepthFirstSearch : public Search {
 public:
  explicit OldDepthFirstSearch(const string& program,
			       int max_iterations);
  virtual ~OldDepthFirstSearch();

  virtual void Run();

 private:

  void DFS(size_t pos, SymbolicExecution& prev_ex);
};
*/

class RandomInputSearch : public Search {
 public:
  RandomInputSearch(const string& program, const string& input_file,
      const string& log_file, int max_iterations, int max_time);
  virtual ~RandomInputSearch();

  virtual void Run();
  
 private:
  SymbolicExecution ex_;
};

class RandomSearch : public Search {
 public:
  RandomSearch(const string& program, const string& input_file,
      const string& log_file, int max_iterations, int max_time);
  virtual ~RandomSearch();

  virtual void Run();

 private:
  SymbolicExecution ex_;

  void SolveUncoveredBranches(size_t i, int depth,
                              const SymbolicExecution& prev_ex);

  bool SolveRandomBranch(vector<value_t>* next_input, size_t* idx);
};


class UniformRandomSearch : public Search {
 public:
  UniformRandomSearch(const string& program, const string& input_file,
      const string& log_file, int max_iterations, int max_time, size_t max_depth);
  virtual ~UniformRandomSearch();

  virtual void Run();

 private:
  SymbolicExecution prev_ex_;
  SymbolicExecution cur_ex_;

  size_t max_depth_;

  void DoUniformRandomPath();
};


// Search looks like:
//  (1) Do random inputs for some amount of time.
//  (2) Do a local search repeatedly in some area, then continue the random search.
class HybridSearch : public Search {
 public:
  HybridSearch(const string& program, const string& input_file,
      const string& log_file, int max_iterations, int max_time, int step_size);
  virtual ~HybridSearch();

  virtual void Run();

 private:
  void RandomLocalSearch(SymbolicExecution* ex, size_t start, size_t end);
  bool RandomStep(SymbolicExecution* ex, size_t start, size_t end);

  int step_size_;
};


class CfgBaselineSearch : public Search {
 public:
  CfgBaselineSearch(const string& program, const string& input_file,
      const string& log_file, int max_iterations, int max_time);
  virtual ~CfgBaselineSearch();

  virtual void Run();

 private:
  SymbolicExecution success_ex_;

  bool DoSearch(int depth, int iters, int pos, const SymbolicExecution& prev_ex);
};


class CfgHeuristicSearch : public Search {
 public:
  CfgHeuristicSearch(const string& program, const string& input_file,
      const string& log_file, int max_iterations, int max_time);
  virtual ~CfgHeuristicSearch();

  virtual void Run();

	static void ReadCFG(const vector<branch_id_t>& branches, 
			vector<vector<branch_id_t> >& cfg);

 private:
  typedef vector<branch_id_t> nbhr_list_t;
  vector<nbhr_list_t> cfg_;
  vector<nbhr_list_t> cfg_rev_;
  vector<size_t> dist_;

  static const size_t kInfiniteDistance = 10000;

  int iters_left_;

  SymbolicExecution success_ex_;

  // Stats.
  unsigned num_inner_solves_;
  unsigned num_inner_successes_pred_fail_;
  unsigned num_inner_lucky_successes_;
  unsigned num_inner_zero_successes_;
  unsigned num_inner_nonzero_successes_;
  unsigned num_inner_recursive_successes_;
  unsigned num_inner_unsats_;
  unsigned num_inner_pred_fails_;

  unsigned num_top_solves_;
  unsigned num_top_solve_successes_;

  unsigned num_solves_;
  unsigned num_solve_successes_;
  unsigned num_solve_sat_attempts_;
  unsigned num_solve_unsats_;
  unsigned num_solve_recurses_;
  unsigned num_solve_pred_fails_;
  unsigned num_solve_all_concrete_;
  unsigned num_solve_no_paths_;

  void UpdateBranchDistances();
  void PrintStats();
  bool DoSearch(int depth, int iters, int pos, int maxDist, const SymbolicExecution& prev_ex);
  bool DoBoundedBFS(int i, int depth, const SymbolicExecution& prev_ex);
  void SkipUntilReturn(const vector<branch_id_t> path, size_t* pos);

  bool FindAlongCfg(size_t i, unsigned int dist,
		    const SymbolicExecution& ex,
		    const set<branch_id_t>& bs);

  bool SolveAlongCfg(size_t i, unsigned int max_dist,
		     const SymbolicExecution& prev_ex);

  void CollectNextBranches(const vector<branch_id_t>& path,
			   size_t* pos, vector<size_t>* idxs);

  size_t MinCflDistance(size_t i,
			const SymbolicExecution& ex,
			const set<branch_id_t>& bs);

};


class CGSSymbolicExecution: public SymbolicExecution {
  public:
    int ex_no_;
    int div_bidx_;
    void InitTried();
    void SetTried(int cidx);
    bool Tried(int cidx);
    vector<branch_id_t> GetContext(int cidx, size_t k, map< branch_id_t, set<branch_id_t> > &dominator);
  private:
    vector<bool> tried_;
};

class ContextGuidedSearch : public Search {
 public:
  explicit ContextGuidedSearch(const string& program,
  		 	       const string& input_file,
			       const string& log_file,
			       int max_iterations,
                               int max_time,
			       int max_depth,
			       const string& dom_file);
  virtual ~ContextGuidedSearch();

  virtual void Run();
  void DoCGS();
	static void ReadDominator(const string& dom_file,
			map<branch_id_t, set<branch_id_t> >& dominator);

 private:
  size_t max_k_;
  vector<CGSSymbolicExecution*> ex_tree_;
  set< vector<branch_id_t> > context_cache_;
  map< branch_id_t, set<branch_id_t> > dominator_;
  
  int GetMaxDepthT(); // Return the maximum depth of T.
  void GetExAtDepth(vector<int>& executions, int depth);

};

struct GenTarget {
  SymbolicExecution* ex;
  size_t cidx;
  size_t score;
};

class GenerationalSearch : public Search {
 public:
  explicit GenerationalSearch(const string& program,
  		 	 	 const string& input_file,
					 const string& log_file,
				   int max_iterations, int max_time);
  virtual ~GenerationalSearch();
  virtual void Run();

 private:
  void DoGenerational();
  static bool GenTargetComp(const struct GenTarget &a, const struct GenTarget &b);
 
};

class TestingSearch : public Search {
 public:
	explicit TestingSearch(const string& program, 
		const string& out_file,
		int max_iterations,
		int max_time);
	virtual ~TestingSearch();
	virtual void Run();

 private:
	const string out_file_;
};
		
class ParameterizedSearch : public Search {
 public:
	explicit ParameterizedSearch(const string& program,
			const string& input_file,
			const string& log_file,
			int max_iterations, int max_time,
			const string& weight_file);
	virtual ~ParameterizedSearch();
	virtual void Run();
 private:
  	SymbolicExecution new_ex_;
	vector<double> weights_;
	vector<DynamicFeature*> dy_features_;
	vector<vector<branch_id_t> > cfg_;
	map<branch_id_t, set<branch_id_t> > dominator_;
	map<branch_id_t, vector<int> > static_feature_map_;
	set<vector<value_t> > inputs_tried_;
	map<branch_id_t, int> bid_unsat_counter_;

	typedef map<size_t, vector<int> > fv_map_t;

	bool DoSearch(SymbolicExecution& ex);
	void UpdateFeatureState(const ExecStatus& ex_stat);
	fv_map_t ExtractFeatures(const SymbolicExecution& ex);
	vector<pair<double, size_t> > ComputeScores(const fv_map_t& fv_map);
};



}  // namespace crest

#endif  // RUN_CREST_CONCOLIC_SEARCH_H__

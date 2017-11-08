#ifndef RUN_CREST_FEATURE_H__
#define RUN_CREST_FEATURE_H__

#include <assert.h>
#include <map>
#include <vector>
#include <set>
#include <deque>
#include <algorithm>
#include <utility>

#include "base/basic_types.h"
#include "base/symbolic_execution.h"


using std::map;
using std::vector;
using std::set;
using std::pair;

namespace crest {

typedef struct exec_status {
	SymbolicExecution* ex_;
	size_t neg_cidx_;
	branch_id_t neg_bid_;
	bool prediction_failure_;
} ExecStatus;

ExecStatus GenExecStatus(SymbolicExecution* ex,
		size_t neg_cidx, branch_id_t neg_bid, bool prediction_failure);

class DynamicFeature {
 public:
	DynamicFeature();
	~DynamicFeature();
	virtual void UpdateFeatureState(const ExecStatus& ex_stat);
	virtual void UpdateFeatureStateImpl(const ExecStatus& ex_stat);
	virtual void ComputeFeature(const SymbolicExecution& ex) = 0;
	bool Predicate(size_t cidx);
	bool IsReadyToCompute() { return ready_; }

	int feature_idx_;

 protected:
	bool ready_;
	vector<bool> checked_cidxs_;
	static int feature_counter_;
};
	
class IsFrontBranch : public DynamicFeature {
 public:
	IsFrontBranch();
	~IsFrontBranch();
	virtual void ComputeFeature(const SymbolicExecution& ex);
};

class IsEndBranch : public DynamicFeature {
 public:
	IsEndBranch();
	~IsEndBranch();
	virtual void ComputeFeature(const SymbolicExecution& ex);
};

class IsMostFrequentlyAppearedInPath : public DynamicFeature {
 public:
	IsMostFrequentlyAppearedInPath();
	~IsMostFrequentlyAppearedInPath();
	virtual void ComputeFeature(const SymbolicExecution& ex);
};

class IsLeastFrequentlyAppearedInPath : public DynamicFeature {
 public:
	IsLeastFrequentlyAppearedInPath();
	~IsLeastFrequentlyAppearedInPath();
	virtual void ComputeFeature(const SymbolicExecution& ex);
};

class IsFreshBranch : public DynamicFeature {
 public:
	IsFreshBranch(const set<branch_id_t>& newly_covered);
	~IsFreshBranch();
	virtual void ComputeFeature(const SymbolicExecution& ex);

 private:
	const set<branch_id_t>& newly_covered_;
};

class IsFreshPartOfPath : public DynamicFeature {
 public:
	IsFreshPartOfPath();
	~IsFreshPartOfPath();
	virtual void ComputeFeature(const SymbolicExecution& ex);
	virtual void UpdateFeatureStateImpl(const ExecStatus& ex_stat);

 private:
	SymbolicExecution* prev_ex_;
	size_t negated_cidx_;
};

class IsVisitedContext : public DynamicFeature {
 public:
	IsVisitedContext(size_t k, 
			map<branch_id_t, set<branch_id_t> >&	dominator);
	~IsVisitedContext();
	virtual void UpdateFeatureStateImpl(const ExecStatus& ex_stat);
	virtual void ComputeFeature(const SymbolicExecution& ex);

 private:
	typedef vector<branch_id_t> ctx_t;
	size_t k_;
	map<branch_id_t, set<branch_id_t> >& dominator_;
	set<ctx_t> ctx_cache_;

	ctx_t GetContext(const SymbolicExecution& ex, size_t cidx);
};

class IsFrequentlySelectedPair : public DynamicFeature {
 public:
	IsFrequentlySelectedPair(size_t k);
	~IsFrequentlySelectedPair();
	virtual void UpdateFeatureStateImpl(const ExecStatus& ex_stat);
	virtual void ComputeFeature(const SymbolicExecution& ex);

 private:
	const size_t k_;
	map<size_t, size_t> cidx_counter_;
	map<branch_id_t, size_t> bid_counter_;
};

class IsNearNegatedBefore : public DynamicFeature {
 public:
	IsNearNegatedBefore();
	~IsNearNegatedBefore();
	virtual void UpdateFeatureStateImpl(const ExecStatus& ex_stat);
	virtual void ComputeFeature(const SymbolicExecution& ex);

 private:
	size_t negated_cidx_;
};

class IsFrequentlyUnsatBranch : public DynamicFeature {
 public:
	IsFrequentlyUnsatBranch(
			const vector<int>& bid_unsat_count,
			const vector<branch_id_t>& paired_branches,
			bool look_pair);
	~IsFrequentlyUnsatBranch();
	virtual void ComputeFeature(const SymbolicExecution& ex);

 private:
	const vector<int>& bid_unsat_count_;	
	const vector<branch_id_t>& paired_branches_;
	bool look_pair_;
	
};

class IsPairedBranchUncovered : public DynamicFeature {
 public:
	IsPairedBranchUncovered(
			const vector<vector<branch_id_t> >& cfg,
			const vector<bool>& total_covered,
			const vector<int>& paired_branches,
			const vector<branch_id_t>& branches,
			const int depth);
	~IsPairedBranchUncovered();
	virtual void ComputeFeature(const SymbolicExecution& ex);

 private:
	const vector<bool>& total_covered_;
	map<branch_id_t, set<branch_id_t> > branches_upto_depth_;
};

class IsRecentlySelectedBranch : public DynamicFeature {
 public:
	IsRecentlySelectedBranch(const int k);
	~IsRecentlySelectedBranch();
	virtual void UpdateFeatureStateImpl(const ExecStatus& ex_stat);
	virtual void ComputeFeature(const SymbolicExecution& ex);

 private:
	const int k_;
	std::deque<branch_id_t> bid_history_;
};

class IsInMostUncoveredFunc : public DynamicFeature {
 public:
	IsInMostUncoveredFunc(
			const vector<bool>& total_covered,
			const vector<unsigned int>& branch_function,
			map<unsigned int, set<branch_id_t> >& fid_branches_map);
	~IsInMostUncoveredFunc();
	virtual void UpdateFeatureStateImpl(const ExecStatus& ex_stat);
	virtual void ComputeFeature(const SymbolicExecution& ex);

 private:
	const vector<bool>& total_covered_;
	const vector<unsigned int>& branch_function_;
	map<unsigned int, set<branch_id_t> >& fid_branches_map_;
};

class IsNearUncovered_Function : public DynamicFeature {
 public:
	IsNearUncovered_Function(
			const vector<vector<branch_id_t> >& cfg,
			const vector<bool>& total_covered,
			const vector<int>& paired_branches,
			const vector<branch_id_t>& branches,
			const int depth,
			map<unsigned int, set<branch_id_t> >& fid_branches_map,
			set<function_id_t>& uncovered_function_id,
			const int threshold);
			
	~IsNearUncovered_Function();
	virtual void ComputeFeature(const SymbolicExecution& ex);

 private:
	const vector<bool>& total_covered_;
	map<branch_id_t, set<branch_id_t> > branches_upto_depth_;
	map<unsigned int, set<branch_id_t> >& fid_branches_map_;
	set<function_id_t>& uncovered_function_id_;
	set<branch_id_t> branches_in_uncovered_func;    
	const vector<int>& paired_branches_;
	int threshold_;
};

class IsInLatestCoveredFunc : public DynamicFeature {
 public:
	IsInLatestCoveredFunc(
			const vector<unsigned int>& branch_function,
			map<unsigned int, set<branch_id_t> >& fid_branches_map,
			const set<unsigned int>& latest_covered_fid);
	~IsInLatestCoveredFunc();
	virtual void UpdateFeatureStateImpl(const ExecStatus& ex_stat);
	virtual void ComputeFeature(const SymbolicExecution& ex);

 private:
	const vector<unsigned int>& branch_function_;
	map<unsigned int, set<branch_id_t> >& fid_branches_map_;
	const set<unsigned int>& latest_covered_fid_;
	map<unsigned int, int> fid_to_exec_counter_;
	set<unsigned int> latest_fids_;
};

}
#endif // RUN_CREST_FEATURE_H__

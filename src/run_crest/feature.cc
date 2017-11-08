#include "run_crest/feature.h"


namespace crest {

int DynamicFeature::feature_counter_ = 12;

ExecStatus GenExecStatus(SymbolicExecution* ex,
		size_t neg_cidx, branch_id_t neg_bid, bool prediction_failure)
{
	ExecStatus ex_stat;
	ex_stat.ex_ = ex;
	ex_stat.neg_cidx_ = neg_cidx;
	ex_stat.neg_bid_ = neg_bid;
	ex_stat.prediction_failure_ = prediction_failure;

	return ex_stat;
}

DynamicFeature::DynamicFeature()
	: feature_idx_(feature_counter_), ready_(false) {
		DynamicFeature::feature_counter_++;
}

DynamicFeature::~DynamicFeature() {}

void DynamicFeature::UpdateFeatureState(const ExecStatus& ex_stat)
{
	ready_ = true;
	checked_cidxs_.clear();

	UpdateFeatureStateImpl(ex_stat);
}

void DynamicFeature::UpdateFeatureStateImpl(const ExecStatus& ex) {}

bool DynamicFeature::Predicate(size_t cidx)
{
	if (ready_) {
		return checked_cidxs_[cidx];
	}
	else
		return false;
}

IsFrontBranch::IsFrontBranch()
	: DynamicFeature() {}

IsFrontBranch::~IsFrontBranch() {}

void IsFrontBranch::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& cidxs = ex.path().constraints_idx();
	size_t top_size = cidxs.size() * 0.1;
	
	for (size_t cidx = 0; cidx < cidxs.size(); ++cidx) {
		if (cidx < top_size)
			checked_cidxs_.push_back(true);
		else
			checked_cidxs_.push_back(false);
	}
}

IsEndBranch::IsEndBranch()
	: DynamicFeature() {}

IsEndBranch::~IsEndBranch() {}

void IsEndBranch::ComputeFeature(const SymbolicExecution& ex) {
	const auto& cidxs = ex.path().constraints_idx();
	size_t bottom_size = cidxs.size() * 0.9;
	
	for (size_t cidx = 0; cidx < cidxs.size(); ++cidx) {
		if (cidx > bottom_size)
			checked_cidxs_.push_back(true);
		else
			checked_cidxs_.push_back(false);
	}
}

IsMostFrequentlyAppearedInPath::IsMostFrequentlyAppearedInPath()
	: DynamicFeature() {}

IsMostFrequentlyAppearedInPath::~IsMostFrequentlyAppearedInPath() {}

void IsMostFrequentlyAppearedInPath::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& cidxs = ex.path().constraints_idx();
	const auto& branches = ex.path().branches();

	map<branch_id_t, size_t> branch_counter;
	for (size_t cidx = 0; cidx < cidxs.size(); ++cidx) {
		branch_id_t bid = branches[cidx];
		branch_counter[bid]++;
	}
		
	int max_count = std::max_element(branch_counter.begin(), branch_counter.end(),
				[](const pair<branch_id_t, size_t>& p1, const pair<branch_id_t, size_t>& p2)
				{ return p1.second < p2.second; })->second;

	for (size_t cidx = 0; cidx < cidxs.size(); ++cidx) {
		branch_id_t bid = branches[cidxs[cidx]];
		if (branch_counter[bid] == max_count)
			checked_cidxs_.push_back(true);
		else
			checked_cidxs_.push_back(false);
	}
}
		
IsLeastFrequentlyAppearedInPath::IsLeastFrequentlyAppearedInPath()
	: DynamicFeature() {}

IsLeastFrequentlyAppearedInPath::~IsLeastFrequentlyAppearedInPath() {}

void IsLeastFrequentlyAppearedInPath::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& cidxs = ex.path().constraints_idx();
	const auto& branches = ex.path().branches();

	map<branch_id_t, size_t> branch_counter;
	for (size_t cidx = 0; cidx < cidxs.size(); ++cidx) {
		branch_id_t bid = branches[cidx];
		branch_counter[bid]++;
	}
		
	int min_count = std::min_element(branch_counter.begin(), branch_counter.end(),
				[](const pair<branch_id_t, size_t>& p1, const pair<branch_id_t, size_t>& p2)
				{ return p1.second > p2.second; })->second;

	for (size_t cidx = 0; cidx < cidxs.size(); ++cidx) {
		branch_id_t bid = branches[cidxs[cidx]];
		if (branch_counter[bid] == min_count)
			checked_cidxs_.push_back(true);
		else
			checked_cidxs_.push_back(false);
	}
}


IsFreshBranch::IsFreshBranch(const set<branch_id_t>& newly_covered)
	: DynamicFeature(), newly_covered_(newly_covered) {}

IsFreshBranch::~IsFreshBranch() {}

void IsFreshBranch::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& cidxs = ex.path().constraints_idx();
	const auto& branches = ex.path().branches();

	for (auto const& bidx: cidxs) {
		branch_id_t bid = branches[bidx];
		if (newly_covered_.count(bid) > 0)
			checked_cidxs_.push_back(true);
		else
			checked_cidxs_.push_back(false);
	}
}

IsFreshPartOfPath::IsFreshPartOfPath()
	: DynamicFeature() {}

IsFreshPartOfPath::~IsFreshPartOfPath() {}

void IsFreshPartOfPath::UpdateFeatureStateImpl(const ExecStatus& ex_stat)
{
	if (ex_stat.prediction_failure_) {
		ready_ = false;

		return;
	}

	negated_cidx_ = ex_stat.neg_cidx_;
}

void IsFreshPartOfPath::ComputeFeature(const SymbolicExecution& ex) {
	const vector<size_t>& cidxs = ex.path().constraints_idx();

	for (size_t cidx = 0; cidx < cidxs.size(); ++cidx) {
		if (cidx < negated_cidx_) {
			checked_cidxs_.push_back(false);
		} else {
			checked_cidxs_.push_back(true);
		}
	}
}


IsVisitedContext::IsVisitedContext(size_t k,
		map<branch_id_t, set<branch_id_t> >& dominator)
	: DynamicFeature(), k_(k), dominator_(dominator) {}

IsVisitedContext::~IsVisitedContext() {}

void IsVisitedContext::UpdateFeatureStateImpl(const ExecStatus& ex_stat)
{
	auto context = GetContext(*(ex_stat.ex_), ex_stat.neg_cidx_);
	ctx_cache_.insert(context);
}

void IsVisitedContext::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& cidxs = ex.path().constraints_idx();

	for (size_t cidx = 0; cidx < cidxs.size(); ++cidx) {
		auto ctx = GetContext(ex, cidx);

		if (ctx_cache_.count(ctx) > 0)
			checked_cidxs_.push_back(true);
		else
			checked_cidxs_.push_back(false);
	}
}

vector<branch_id_t>
IsVisitedContext::GetContext(const SymbolicExecution& ex, size_t cidx)
{
	vector<branch_id_t> context;
	const vector<branch_id_t>& branches = ex.path().branches();
	const vector<size_t>& cidxs = ex.path().constraints_idx();

  int bidx = cidxs[cidx];
  branch_id_t bid = branches[bidx];
  auto const& dom = dominator_.find(bid)->second;

  context.push_back(bid);
  bidx--;

  while (bidx >= 0) {
    if (context.size() >= k_) 
      break;
    if (dom.count(branches[bidx]) == 0)
      context.push_back(branches[bidx]);
    bidx--;
  }
  return context;
}

IsFrequentlySelectedPair::IsFrequentlySelectedPair(size_t k)
	: DynamicFeature(), k_(k) {}

IsFrequentlySelectedPair::~IsFrequentlySelectedPair() {}

void IsFrequentlySelectedPair::UpdateFeatureStateImpl(const ExecStatus& ex_stat)
{
	cidx_counter_[ex_stat.neg_cidx_]++;
	bid_counter_[ex_stat.neg_bid_]++;
}

void IsFrequentlySelectedPair::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& cidxs = ex.path().constraints_idx();
	const auto& branches = ex.path().branches();
	
	for (size_t cidx = 0; cidx < cidxs.size(); ++cidx) {
		branch_id_t bid = branches[cidxs[cidx]];
		if ((cidx_counter_[cidx] > k_) && (bid_counter_[bid] > k_))
			checked_cidxs_.push_back(true);
		else
			checked_cidxs_.push_back(false);
	}
}

IsNearNegatedBefore::IsNearNegatedBefore()
	: DynamicFeature() {}

IsNearNegatedBefore::~IsNearNegatedBefore() {}

void IsNearNegatedBefore::UpdateFeatureStateImpl(const ExecStatus& ex_stat) {
	negated_cidx_ = ex_stat.neg_cidx_;
}

void IsNearNegatedBefore::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& cidxs = ex.path().constraints_idx();
	double ratio = 0.2;
	int lower = negated_cidx_ - (ex.path().constraints_idx().size() * ratio);
	int upper = negated_cidx_ + (ex.path().constraints_idx().size() * ratio);
	
	for (int cidx = 0; cidx < cidxs.size(); ++cidx) { 
		if (cidx >= lower || cidx <= upper)
			checked_cidxs_.push_back(true);
		else
			checked_cidxs_.push_back(false);
	}
}


IsFrequentlyUnsatBranch::IsFrequentlyUnsatBranch(
		const vector<int>& bid_unsat_count,
		const vector<branch_id_t>& paired_branches, bool look_pair)
	: DynamicFeature(), bid_unsat_count_(bid_unsat_count),
		paired_branches_(paired_branches), look_pair_(look_pair) {}

IsFrequentlyUnsatBranch::~IsFrequentlyUnsatBranch() {}

void IsFrequentlyUnsatBranch::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& cidxs = ex.path().constraints_idx();
	const auto& branches = ex.path().branches();
	
	for (auto cit = cidxs.begin(); cit != cidxs.end(); ++cit) {
		branch_id_t bid = branches[*cit];
		int unsat_count;
		if (look_pair_)
			unsat_count = bid_unsat_count_[paired_branches_[bid]];
		else
			unsat_count = bid_unsat_count_[bid];

		if (unsat_count > 10)
			checked_cidxs_.push_back(true);
		else
			checked_cidxs_.push_back(false);
	}
}


IsPairedBranchUncovered::IsPairedBranchUncovered(
		const vector<vector<branch_id_t> >& cfg,
		const vector<bool>& total_covered,
		const vector<int>& paired_branches,
		const vector<branch_id_t>& branches,
		const int depth)
	: DynamicFeature(),
		total_covered_(total_covered) {

	for (auto const &br: branches) {
		branch_id_t bid = br;
		set<branch_id_t> paired;
		paired.insert(paired_branches[bid]);

		branches_upto_depth_[bid] = paired;
	}

	for (auto &kv: branches_upto_depth_) {
		int current_depth = 0;

		auto key = kv.first;
		auto value = kv.second;
		set<branch_id_t> collected(value);
		vector<branch_id_t> to_collect(value.begin(), value.end());

		while (current_depth++ < depth) {
			vector<branch_id_t> next_branches;
			for (auto const &branch: to_collect) {
				for (auto const &next_branch: cfg[branch]) {
					next_branches.push_back(next_branch);
					collected.insert(next_branch);
				}
			}
			to_collect.swap(next_branches);
		}

		branches_upto_depth_[key] = collected;
	}
}


IsPairedBranchUncovered::~IsPairedBranchUncovered() {}

void IsPairedBranchUncovered::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& branches = ex.path().branches();
	const auto& cidxs = ex.path().constraints_idx();

	map<branch_id_t, bool> branch_to_predicate;

	set<branch_id_t> branches_in_path;

	for (const auto& cidx: cidxs) {
		branch_id_t bid = branches[cidx];
		branches_in_path.insert(bid);
	}

	for (const auto& bid: branches_in_path) {
		const auto& bids = branches_upto_depth_[bid];
		bool check_uncovered_all = true;
		for (const auto& br: bids) {
			if (total_covered_[br]) {
				check_uncovered_all = false;
				break;
			}
		}
		branch_to_predicate[bid] = check_uncovered_all;
	}

	for (const auto& cidx: cidxs) {
		branch_id_t bid = branches[cidx];
		if (branch_to_predicate[bid])
			checked_cidxs_.push_back(true);
		else
			checked_cidxs_.push_back(false);
	}
}

IsRecentlySelectedBranch::IsRecentlySelectedBranch(const int k)
	: DynamicFeature(), k_(k) {}

IsRecentlySelectedBranch::~IsRecentlySelectedBranch() {}

void IsRecentlySelectedBranch::UpdateFeatureStateImpl(const ExecStatus& ex_stat)
{
	branch_id_t bid = ex_stat.neg_bid_;
	int history_size = bid_history_.size();
	assert(history_size <= k_);
	if (history_size >= k_) {
		bid_history_.pop_front();
	}

	bid_history_.push_back(bid);
}

void IsRecentlySelectedBranch::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& branches = ex.path().branches();
	const auto& cidxs = ex.path().constraints_idx();
	map<branch_id_t, bool> bid_to_predicate;
	set<branch_id_t> bids_in_path;
	set<branch_id_t> bids_in_history(bid_history_.begin(), bid_history_.end());

	for (auto const& cidx: cidxs) {
		branch_id_t bid = branches[cidx];
		bids_in_path.insert(bid);
	}
	
	for (auto const& bid: bids_in_path) {
		if (bids_in_history.count(bid)) 
			bid_to_predicate[bid] = true;
		else
			bid_to_predicate[bid] = false;
	}

	for (auto const& cidx: cidxs) {
		branch_id_t bid = branches[cidx];
		if (bid_to_predicate[bid]) {
			checked_cidxs_.push_back(true);
		} else {
			checked_cidxs_.push_back(false);
		}
	}
}

IsInMostUncoveredFunc::IsInMostUncoveredFunc(
		const vector<bool>& total_covered,
		const vector<unsigned int>& branch_function,
		map<unsigned int, set<branch_id_t> >& fid_branches_map)
	: DynamicFeature(), total_covered_(total_covered),
		branch_function_(branch_function), fid_branches_map_(fid_branches_map) {}

IsInMostUncoveredFunc::~IsInMostUncoveredFunc() {}

void IsInMostUncoveredFunc::UpdateFeatureStateImpl(const ExecStatus& ex_stat) {}

	
void IsInMostUncoveredFunc::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& cidxs = ex.path().constraints_idx();
	const auto& branches = ex.path().branches();

	set<branch_id_t> covered_bids;
	set<int> fids_in_path;

	for (const auto& bidx: cidxs) {
		branch_id_t bid = branches[bidx];
		if (total_covered_[bid]) {
			covered_bids.insert(bid);

			int fid = branch_function_[bid];
			fids_in_path.insert(fid);
		}
	}
	
	int largest = 0;
	int most_uncovered_fid = 0;
	for (const auto& fid: fids_in_path) {
		set<branch_id_t> bids_in_func = fid_branches_map_[fid];
		vector<branch_id_t> uncovered(bids_in_func.size());
		auto sit = std::set_difference(bids_in_func.begin(), bids_in_func.end(),
				covered_bids.begin(), covered_bids.end(), uncovered.begin());
		uncovered.resize(sit - uncovered.begin());

		int num_uncovered = uncovered.size();
		if (num_uncovered >= largest) {
			most_uncovered_fid = fid;
		}
	}

	for (auto const& bidx: cidxs) {
		branch_id_t bid = branches[bidx];
		int fid = branch_function_[bid];

		if (fid == most_uncovered_fid)
			checked_cidxs_.push_back(true);
		else
			checked_cidxs_.push_back(false);
	}
}

IsNearUncovered_Function::IsNearUncovered_Function(
		const vector<vector<branch_id_t> >& cfg,
		const vector<bool>& total_covered,
		const vector<int>& paired_branches,
		const vector<branch_id_t>& branches,
		const int depth,
		map<unsigned int, set<branch_id_t> >& fid_branches_map,
		set<function_id_t>& uncovered_function_id,
		int threshold)
	: DynamicFeature(), total_covered_(total_covered), paired_branches_(paired_branches),  
	                    fid_branches_map_(fid_branches_map), uncovered_function_id_(uncovered_function_id), threshold_(threshold){

	for (auto const &br: branches) {
		branch_id_t bid = br;
		set<branch_id_t> paired;
		paired.insert(paired_branches[bid]);

		branches_upto_depth_[bid] = paired;
	}

	for (auto &kv: branches_upto_depth_) {
		int current_depth = 0;

		auto key = kv.first;
		auto value = kv.second;
		set<branch_id_t> collected(value);
		vector<branch_id_t> to_collect(value.begin(), value.end());

		while (current_depth++ < depth) {
			vector<branch_id_t> next_branches;
			for (auto const &branch: to_collect) {
				for (auto const &next_branch: cfg[branch]) {
					next_branches.push_back(next_branch);
					collected.insert(next_branch);
				}
			}
			to_collect.swap(next_branches);
		}

		branches_upto_depth_[key] = collected;
	}
	}


IsNearUncovered_Function::~IsNearUncovered_Function() {}

void IsNearUncovered_Function::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& branches = ex.path().branches();
	const auto& cidxs = ex.path().constraints_idx();

	map<branch_id_t, bool> branch_to_predicate;

	set<branch_id_t> branches_in_path;
	branches_in_uncovered_func.clear();
	for(const auto& fun_id: uncovered_function_id_){
		set<branch_id_t> bids_in_func = fid_branches_map_[fun_id];
		if(bids_in_func.size()>=threshold_){
        		set<branch_id_t>::iterator it = bids_in_func.begin();
			branches_in_uncovered_func.insert(*it);
			branches_in_uncovered_func.insert(paired_branches_[*it]);
		}
	}
	for (const auto& cidx: cidxs) {
		branch_id_t bid = branches[cidx];
		branches_in_path.insert(bid);
	}
	for (const auto& bid: branches_in_path) {
		const auto& bids = branches_upto_depth_[bid];
		bool check_uncovered_all = false;
		for (const auto& br: bids) {
			if (branches_in_uncovered_func.count(br)>0) {
				check_uncovered_all = true;
				break;
			}
		}
		branch_to_predicate[bid] = check_uncovered_all;
	}
	for (const auto& cidx: cidxs) {
		branch_id_t bid = branches[cidx];
		if (branch_to_predicate[bid])
			checked_cidxs_.push_back(true);
		else
			checked_cidxs_.push_back(false);
	}
}

IsInLatestCoveredFunc::IsInLatestCoveredFunc(
		const vector<unsigned int>& branch_function,
		map<unsigned int, set<branch_id_t> >& fid_branches_map,
		const set<unsigned int>& latest_covered_fid)
	: DynamicFeature(), branch_function_(branch_function),
		fid_branches_map_(fid_branches_map), latest_covered_fid_(latest_covered_fid) {}

IsInLatestCoveredFunc::~IsInLatestCoveredFunc() {}

void IsInLatestCoveredFunc::UpdateFeatureStateImpl(const ExecStatus& ex_stat)
{
	int count = 10;

	for (const auto& fid: latest_covered_fid_) {
		fid_to_exec_counter_[fid] = count;
	}

	for (auto& kv: fid_to_exec_counter_) {
		unsigned int fid = kv.first;
		int count = kv.second;

		if ((fid_branches_map_[fid]).size() < 10)
			continue;

		if (count > 0) {
			latest_fids_.insert(fid);
			fid_to_exec_counter_[fid]--;
		} else {
			latest_fids_.erase(fid);
			fid_to_exec_counter_.erase(fid);
		}
	}
}

void IsInLatestCoveredFunc::ComputeFeature(const SymbolicExecution& ex)
{
	const auto& cidxs = ex.path().constraints_idx();
	const auto& branches = ex.path().branches();

	for (auto const& bidx: cidxs) {
		branch_id_t bid = branches[bidx];
		unsigned int fid = branch_function_[bid];

		if (latest_fids_.count(fid) > 0) {
			checked_cidxs_.push_back(true);
		} else {
			checked_cidxs_.push_back(false);
		}
	}
}


}

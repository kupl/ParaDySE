// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <sys/time.h>

#include "run_crest/concolic_search.h"

int main(int argc, char* argv[]) {
  if (argc < 4) {
    fprintf(stderr,
            "Syntax: run_crest <program> "
						"<input file>"
						"<log file> "
            "<number of iterations or number of seconds e.g.) 1000 or 3600s> "
            "-<strategy> [strategy options]\n");
    fprintf(stderr,
            "  Strategies include: "
            "dfs, cfg, random, uniform_random, random_input, cgs, carfast, generational\n");
    return 1;
  }

	string prog = argv[1];
	string input_file = argv[2];
	string log_file = argv[3];
	string testing_budget = argv[4];

  int num_iters, num_time;
  string::reverse_iterator rit = testing_budget.rbegin();
  // If testing_budget ends with 's', it means testing budget is time.
  // Otherwise, testing budget is iteration.
  if (*rit == 's') {
    *rit = NULL;
    num_time = atoi(testing_budget.c_str());
    num_iters = INT_MAX;
  } else {
    num_iters = atoi(argv[4]);
    num_time = INT_MAX;
  }
  string search_type = argv[5];

  // Initialize the random number generator.
  struct timeval tv;
  gettimeofday(&tv, NULL);
  srand((tv.tv_sec * 1000000) + tv.tv_usec);

  crest::Search* strategy;
  if (search_type == "-random") {
    strategy = new crest::RandomSearch(prog, input_file, log_file, num_iters, num_time);
  } else if (search_type == "-random_input") {
    strategy = new crest::RandomInputSearch(prog, input_file, log_file, num_iters, num_time);
  } else if (search_type == "-dfs") {
    if (argc == 6) {
      strategy = new crest::BoundedDepthFirstSearch(prog, input_file, log_file, num_iters, num_time, 1000000);
    } else {
      strategy = new crest::BoundedDepthFirstSearch(prog, input_file, log_file, num_iters, num_time, atoi(argv[6]));
    }
  } else if (search_type == "-cfg") {
    strategy = new crest::CfgHeuristicSearch(prog, input_file, log_file, num_iters, num_time);
  } else if (search_type == "-cfg_baseline") {
    strategy = new crest::CfgBaselineSearch(prog, input_file, log_file, num_iters, num_time);
  } else if (search_type == "-hybrid") {
    strategy = new crest::HybridSearch(prog, input_file, log_file, num_iters, num_time, 100);
  } else if (search_type == "-uniform_random") {
    if (argc == 5) {
      strategy = new crest::UniformRandomSearch(prog, input_file, log_file, num_iters, num_time, 100000000);
    } else {
      strategy = new crest::UniformRandomSearch(prog, input_file, log_file, num_iters, num_time, atoi(argv[6]));
    }
  } else if (search_type == "-testing") {
		string out_file = argv[2];
    strategy = new crest::TestingSearch(prog, out_file, num_iters, num_time);
  } else if (search_type == "-generational") {
    strategy = new crest::GenerationalSearch(prog, input_file, log_file, num_iters, num_time);
  } else if (search_type == "-cgs") {
    assert (argc == 7 || argc == 8);
		if (argc == 7) {
			strategy = new crest::ContextGuidedSearch(prog, input_file, log_file, num_iters, num_time, atoi(argv[6]), "");
		} else { 
			strategy = new crest::ContextGuidedSearch(prog, input_file, log_file, num_iters, num_time, atoi(argv[6]), argv[7]);
		}
  } else if (search_type == "-param") {
			const string& weight_file = argv[6];
			strategy = new crest::ParameterizedSearch(prog, input_file, log_file, num_iters, num_time, weight_file);
	} else {
    fprintf(stderr, "Unknown search strategy: %s\n", search_type.c_str());
    return 1;
  }

  strategy->Run();

  delete strategy;
  return 0;
}



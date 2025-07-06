#ifndef BEECOLONY
#define BEECOLONY

#include "Instance.h"
#include "Solution.h"
#include "Evaluator.h"
#include "Greedy.h"
#include "IteratedGreedy.h"

class BeeColony {
private:
    Instance& instance;
    int pop_size;
    int limit;
    int max_cycles;
    double destruction_rate;

    vector<Solution> population;
    vector<int> trial_counters;
    vector<double> fitness;
    vector<double> costs;
    Solution best_solution;
    double best_cost;

    double evaluate(Solution& sol);

public:
    BeeColony(Instance& inst);

    void solve(int pop_size, int limit, int max_cycles, double destruction_rate);

    Solution getBestSolution();
};

#endif

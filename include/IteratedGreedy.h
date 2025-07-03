#ifndef ITERATEDGREEDY
#define ITERATEDGREEDY

#include "Instance.h"
#include "Solution.h"
#include "Evaluator.h"
#include "Greedy.h"

class IteratedGreedy
{
private:
    vector<string> select_events(const Solution& solution, const Instance& instance, int num_events);

    pair<Solution, vector<string>> destroy(Solution solution, int destruction_rate, const Instance &instance);

    Solution rebuild(Solution solution, vector<string> &destroyed, Instance &instance);

public:
    void remove_allocations(string event_id, Solution &solution, const Instance &instance);

    Solution solve(Instance &instance, int max_iters, float destruction_percentage);
};


#endif
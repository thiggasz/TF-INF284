#ifndef GREEDY
#define GREEDY

#include "Instance.h"
#include "Solution.h"

class Greedy
{
public:
    Solution generate_greedy(const Instance &instance, int max_attempts = 100);

    void greedy_event_allocation(string event_id, Solution &solution, Instance &instance, bool record_bad = false);
};

#endif
#ifndef GREEDY
#define GREEDY

#include "Instance.h"
#include "Solution.h"

class Greedy
{
public:
    Solution generate_greedy(const Instance &instance);

    void generate_greedy(vector<string> destroyed_events, Solution &solution, Instance &instance);
};

#endif
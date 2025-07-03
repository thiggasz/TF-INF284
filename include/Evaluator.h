#ifndef EVALUATOR
#define EVALUATOR

#include "Instance.h"
#include "Solution.h"

class Evaluator
{
private:
    void check_hard_constraints(const Instance &instance, const Solution &solution);
    void check_soft_constraints(const Instance &instance, const Solution &solution);

public:
    int hard_violations = 0;
    int soft_violations = 0;
    int total_cost = 0;

    void evaluate(const Instance &instance, const Solution &solution);
    void print_report() const;
};

#endif

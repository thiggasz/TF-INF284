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
    int pop_size; // Número de fontes de alimento (tamanho da população)
    int limit; // Limite de tentativas antes do abandono
    int max_cycles; // Número máximo de ciclos
    double destruction_rate; // Taxa de destruição para perturbação

    vector<Solution> population;
    vector<int> trial_counters;
    vector<double> fitness;
    vector<double> costs;
    Solution best_solution;
    double best_cost;

    // Função para avaliar uma solução
    double evaluate(Solution& sol);

    // Função para destruir eventos aleatoriamente
    pair<Solution, vector<string>> destroy_random(Solution solution, int num_events);

    // Função para perturbar uma solução
    Solution perturb_solution(Solution sol);

public:
    BeeColony(Instance& inst);

    void solve(int pop_size, int limit, int max_cycles, double destruction_rate);

    Solution getBestSolution();
};

#endif

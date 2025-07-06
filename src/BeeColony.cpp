#include "../include/BeeColony.h"

#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>

double BeeColony::evaluate(Solution &sol)
{
    Evaluator evaluator;
    evaluator.evaluate(instance, sol);

    return evaluator.hard_violations * 1000 + evaluator.total_cost;
}

BeeColony::BeeColony(Instance &inst) : instance(inst) {}

void BeeColony::solve(int pop_size, int limit, int max_cycles, double destruction_rate)
{
    this->pop_size = pop_size;
    this->limit = limit;
    this->max_cycles = max_cycles;
    this->destruction_rate = destruction_rate;

    population.resize(pop_size);
    trial_counters.assign(pop_size, 0);
    costs.resize(pop_size);
    fitness.resize(pop_size);
    best_cost = 1e9;

    Greedy greedy;

    for (int i = 0; i < pop_size; i++)
    {
        population[i] = greedy.generate_greedy(instance);
        costs[i] = evaluate(population[i]);

        if (costs[i] < best_cost)
        {
            best_cost = costs[i];
            best_solution = population[i];
        }
    }

    for (int cycle = 0; cycle < max_cycles; cycle++)
    {
        // Abelhas operárias
        for (int i = 0; i < pop_size; i++)
        {
            IteratedGreedy iterated_greedy;
            Solution new_solution = iterated_greedy.solve(instance, 1, destruction_rate);

            double new_cost = evaluate(new_solution);

            if (new_cost < costs[i])
            {
                population[i] = new_solution;
                costs[i] = new_cost;
                trial_counters[i] = 0;

                if (new_cost < best_cost)
                {
                    best_cost = new_cost;
                    best_solution = new_solution;
                }
            }
            else
                trial_counters[i]++;
        }

        double total_fitness = 0.0;
        for (int i = 0; i < pop_size; i++)
        {
            fitness[i] = 1.0 / (1.0 + costs[i]);
            total_fitness += fitness[i];
        }

        vector<double> probabilities(pop_size);
        for (int i = 0; i < pop_size; i++)
            probabilities[i] = fitness[i] / total_fitness;

        // Abelhas observadoras
        for (int i = 0; i < pop_size; i++)
        {
            double r = (double)rand() / RAND_MAX;
            double sum_prob = 0.0;
            int selected_idx = 0;

            for (int j = 0; j < pop_size; j++)
            {
                sum_prob += probabilities[j];
                if (r <= sum_prob)
                {
                    selected_idx = j;
                    break;
                }
            }

            IteratedGreedy iterated_greedy;
            Solution new_solution = iterated_greedy.solve(instance, 1, destruction_rate);

            double new_cost = evaluate(new_solution);

            if (new_cost < costs[selected_idx])
            {
                population[selected_idx] = new_solution;
                costs[selected_idx] = new_cost;
                trial_counters[selected_idx] = 0;

                if (new_cost < best_cost)
                {
                    best_cost = new_cost;
                    best_solution = new_solution;
                }
            }
            else
                trial_counters[selected_idx]++;
        }

        Greedy greedy;

        // Abelhas exploradoras
        for (int i = 0; i < pop_size; i++)
        {
            if (trial_counters[i] >= limit)
            {
                population[i] = greedy.generate_greedy(instance);
                costs[i] = evaluate(population[i]);
                trial_counters[i] = 0;

                if (costs[i] < best_cost)
                {
                    best_cost = costs[i];
                    best_solution = population[i];
                }
            }
        }

        if (cycle % 10 == 0)
            cout << "Ciclo " << cycle << ": Melhor custo = " << best_cost << endl;
    }
}

Solution BeeColony::getBestSolution()
{
    return best_solution;
}

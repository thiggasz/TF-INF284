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

pair<Solution, vector<string>> BeeColony::destroy_random(Solution solution, int num_events)
{
    vector<string> all_event_ids;
    for (const auto &e : instance.events)
    {
        all_event_ids.push_back(e.id);
    }

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    shuffle(all_event_ids.begin(), all_event_ids.end(), mt19937(seed));

    vector<string> selected;
    int n = min(num_events, (int)all_event_ids.size());
    for (int i = 0; i < n; i++)
    {
        selected.push_back(all_event_ids[i]);
    }

    IteratedGreedy iterated_greedy;

    for (const auto &event_id : selected)
    {
        iterated_greedy.remove_allocations(event_id, solution, instance);
    }

    return {solution, selected};
}

Solution BeeColony::perturb_solution(Solution sol)
{
    int destruction_rate = max(1, (int)(instance.events.size() * this->destruction_rate));
    auto [new_sol, destroyed] = destroy_random(sol, destruction_rate);

    sort(destroyed.begin(), destroyed.end(), [this](const string &a, const string &b)
         {
        const EventInfo& eventA = instance.events[instance.event_index.at(a)];
        const EventInfo& eventB = instance.events[instance.event_index.at(b)];
        return eventA.total_duration > eventB.total_duration; });

    Greedy greedy;
    IteratedGreedy ig;

    for (const string &event_id : destroyed)
    {
        if (instance.event_index.find(event_id) == instance.event_index.end())
            continue;

        int attempts = 0;
        int max_attempts = 5;
        int remaining = instance.events[instance.event_index.at(event_id)].total_duration;

        while (remaining > 0 && attempts < max_attempts)
        {
            greedy.greedy_event_allocation(event_id, new_sol, instance, false);
            int allocated_after = new_sol.allocated_duration[event_id];
            remaining = instance.events[instance.event_index.at(event_id)].total_duration - allocated_after;

            if (remaining > 0)
            {
                ig.remove_allocations(event_id, new_sol, instance);
                remaining = instance.events[instance.event_index.at(event_id)].total_duration;
            }
            attempts++;
        }
    }

    return new_sol;
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
        // Fase das abelhas operárias
        for (int i = 0; i < pop_size; i++)
        {
            Solution new_solution = perturb_solution(population[i]);
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
            {
                trial_counters[i]++;
            }
        }

        double total_fitness = 0.0;
        for (int i = 0; i < pop_size; i++)
        {
            fitness[i] = 1.0 / (1.0 + costs[i]);
            total_fitness += fitness[i];
        }

        vector<double> probabilities(pop_size);
        for (int i = 0; i < pop_size; i++)
        {
            probabilities[i] = fitness[i] / total_fitness;
        }

        // Fase das abelhas observadoras
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

            Solution new_solution = perturb_solution(population[selected_idx]);
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
            {
                trial_counters[selected_idx]++;
            }
        }

        Greedy greedy;

        // Fase das abelhas exploradoras
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
        {
            // cout << "Ciclo " << cycle << ": Melhor custo = " << best_cost << endl;
        }
    }
}

Solution BeeColony::getBestSolution()
{
    return best_solution;
}

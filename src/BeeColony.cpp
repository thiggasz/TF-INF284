#include "../include/BeeColony.h"

#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>

// Função para avaliar uma solução
double BeeColony::evaluate(Solution& sol) {
    Evaluator evaluator;
    evaluator.evaluate(instance, sol);

    return evaluator.hard_violations * 1000 + evaluator.total_cost;
}

// Função para destruir eventos aleatoriamente
pair<Solution, vector<string>> BeeColony::destroy_random(Solution solution, int num_events) {
    vector<string> all_event_ids;
    for (const auto& e : instance.events) {
        all_event_ids.push_back(e.id);
    }
    
    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    shuffle(all_event_ids.begin(), all_event_ids.end(), mt19937(seed));
    
    vector<string> selected;
    int n = min(num_events, (int)all_event_ids.size());
    for (int i = 0; i < n; i++) {
        selected.push_back(all_event_ids[i]);
    }

    IteratedGreedy iterated_greedy;

    for (const auto& event_id : selected) {
        iterated_greedy.remove_allocations(event_id, solution, instance);
    }

    return {solution, selected};
}

// Função para perturbar uma solução
Solution BeeColony::perturb_solution(Solution sol) {
    int destruction_rate = max(1, (int)(instance.events.size() * this->destruction_rate));
    auto [new_sol, destroyed] = destroy_random(sol, destruction_rate);

    Greedy greedy;
    
    for (const string& event_id : destroyed) {
        greedy.greedy_event_allocation(event_id, new_sol, instance, false);
    }
    
    return new_sol;
}


BeeColony::BeeColony(Instance& inst) : instance(inst) {}

void BeeColony::solve(int pop_size, int limit, int max_cycles, double destruction_rate) {
    this->pop_size = pop_size;
    this->limit = limit;
    this->max_cycles = max_cycles;
    this->destruction_rate = destruction_rate;

    // Inicialização
    population.resize(pop_size);
    trial_counters.assign(pop_size, 0);
    costs.resize(pop_size);
    fitness.resize(pop_size);
    best_cost = 1e9;

    Greedy greedy;

    // Gerar população inicial
    for (int i = 0; i < pop_size; i++) {
        population[i] = greedy.generate_greedy(instance, 100);
        costs[i] = evaluate(population[i]);
        
        if (costs[i] < best_cost) {
            best_cost = costs[i];
            best_solution = population[i];
        }
    }

    // Loop principal
    for (int cycle = 0; cycle < max_cycles; cycle++) {
        // Fase das abelhas empregadas
        for (int i = 0; i < pop_size; i++) {
            Solution new_solution = perturb_solution(population[i]);
            double new_cost = evaluate(new_solution);

            // Seleção gananciosa
            if (new_cost < costs[i]) {
                population[i] = new_solution;
                costs[i] = new_cost;
                trial_counters[i] = 0;
                
                if (new_cost < best_cost) {
                    best_cost = new_cost;
                    best_solution = new_solution;
                }
            } else {
                trial_counters[i]++;
            }
        }

        // Calcular probabilidades para as onlookers
        double total_fitness = 0.0;
        for (int i = 0; i < pop_size; i++) {
            fitness[i] = 1.0 / (1.0 + costs[i]);
            total_fitness += fitness[i];
        }
        
        vector<double> probabilities(pop_size);
        for (int i = 0; i < pop_size; i++) {
            probabilities[i] = fitness[i] / total_fitness;
        }

        // Fase das abelhas observadoras
        for (int i = 0; i < pop_size; i++) {
            // Seleção por roleta
            double r = (double)rand() / RAND_MAX;
            double sum_prob = 0.0;
            int selected_idx = 0;
            
            for (int j = 0; j < pop_size; j++) {
                sum_prob += probabilities[j];
                if (r <= sum_prob) {
                    selected_idx = j;
                    break;
                }
            }

            Solution new_solution = perturb_solution(population[selected_idx]);
            double new_cost = evaluate(new_solution);

            if (new_cost < costs[selected_idx]) {
                population[selected_idx] = new_solution;
                costs[selected_idx] = new_cost;
                trial_counters[selected_idx] = 0;
                
                if (new_cost < best_cost) {
                    best_cost = new_cost;
                    best_solution = new_solution;
                }
            } else {
                trial_counters[selected_idx]++;
            }
        }

        Greedy greedy;

        // Fase das abelhas exploradoras
        for (int i = 0; i < pop_size; i++) {
            if (trial_counters[i] >= limit) {
                // Substituir solução abandonada
                population[i] = greedy.generate_greedy(instance, 100);
                costs[i] = evaluate(population[i]);
                trial_counters[i] = 0;
                
                if (costs[i] < best_cost) {
                    best_cost = costs[i];
                    best_solution = population[i];
                }
            }
        }

        // Log de progresso
        if (cycle % 10 == 0) {
            cout << "Ciclo " << cycle << ": Melhor custo = " << best_cost << endl;
        }
    }
}

Solution BeeColony::getBestSolution() {
    return best_solution;
}

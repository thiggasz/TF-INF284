#include "../include/IteratedGreedy.h"

#include <chrono>
#include <random>
#include <algorithm>

vector<string> IteratedGreedy::select_events(const Solution& solution, const Instance& instance, int num_events) {
    vector<pair<string, int>> event_costs;
    for (const auto& e : solution.event_allocations) {
        const string& event_id = e.first;
        int cost = 0;

        if (solution.allocated_duration.at(event_id) < instance.events[instance.event_index.at(event_id)].total_duration) {
            cost += 1000;
        }

        if (solution.event_day_counts.find(event_id) != solution.event_day_counts.end()) {
            for (const auto& day_count : solution.event_day_counts.at(event_id)) {
                if (day_count.second > 1) {
                    cost += 50; 
                }
            }
        }

        if (instance.course_split_constraints.find(instance.events[instance.event_index.at(event_id)].course_id) != instance.course_split_constraints.end()) {
            auto constraint = instance.course_split_constraints.at(instance.events[instance.event_index.at(event_id)].course_id);
            int min_double = constraint.first;
            int max_double = constraint.second;
            int actual_double = (solution.event_double_lessons.find(event_id) != solution.event_double_lessons.end()) ? solution.event_double_lessons.at(event_id) : 0;
            if (actual_double < min_double) cost += (min_double - actual_double) * 10;
            if (actual_double > max_double) cost += (actual_double - max_double) * 10;
        }

        event_costs.push_back({event_id, cost});
    }

    sort(event_costs.begin(), event_costs.end(), [](const pair<string, int>& a, const pair<string, int>& b) {
        return a.second > b.second;
    });

    vector<string> selected;
    int n = min(num_events, (int)event_costs.size());
    for (int i = 0; i < n; i++) {
        selected.push_back(event_costs[i].first);
    }
    return selected;
}

void IteratedGreedy::remove_allocations(string event_id, Solution &solution, const Instance &instance) {
    if (instance.event_index.find(event_id) == instance.event_index.end()) return;
    
    const EventInfo &event = instance.events.at(instance.event_index.at(event_id));

    auto it = solution.allocations.begin();
    while (it != solution.allocations.end()) {
        if (it->event_id == event_id) {
            solution.teacher_occupation[it->time_id].erase(event.teacher_id);
            solution.class_occupation[it->time_id].erase(event.class_id);

            const TimeInfo &t = instance.times.at(instance.time_index.at(it->time_id));
            solution.event_day_counts[event_id][t.day]--;
            if (solution.event_day_counts[event_id][t.day] == 0) {
                solution.event_day_counts[event_id].erase(t.day);
            }

            if (it->duration == 2) {
                solution.event_double_lessons[event_id]--;
            }

            it = solution.allocations.erase(it);
        } else {
            ++it;
        }
    }

    solution.event_allocations.erase(event_id);
    solution.allocated_duration.erase(event_id);

    set<string> days_to_remove;
    for (const auto& day : solution.teacher_schedule[event.teacher_id]) {
        bool has_other_allocations = false;
        for (const Allocation &alloc : solution.allocations) {
            if (alloc.time_id == "UNALLOCATED") 
                continue; 
            const EventInfo &e = instance.events.at(instance.event_index.at(alloc.event_id));
            if (e.teacher_id == event.teacher_id) {
                const TimeInfo &t = instance.times.at(instance.time_index.at(alloc.time_id));
                if (t.day == day) {
                    has_other_allocations = true;
                    break;
                }
            }
        }
        if (!has_other_allocations) {
            days_to_remove.insert(day);
        }
    }
    for (const auto& day : days_to_remove) {
        solution.teacher_schedule[event.teacher_id].erase(day);
    }
    if (solution.teacher_schedule[event.teacher_id].empty()) {
        solution.teacher_schedule.erase(event.teacher_id);
    }
    
    days_to_remove.clear();
    for (const auto& day : solution.class_schedule[event.class_id]) {
        bool has_other_allocations = false;
        for (const Allocation &alloc : solution.allocations) {
            if (alloc.time_id == "UNALLOCATED") 
                continue; 
            const EventInfo &e = instance.events.at(instance.event_index.at(alloc.event_id));
            if (e.class_id == event.class_id) {
                const TimeInfo &t = instance.times.at(instance.time_index.at(alloc.time_id));
                if (t.day == day) {
                    has_other_allocations = true;
                    break;
                }
            }
        }
        if (!has_other_allocations) {
            days_to_remove.insert(day);
        }
    }
    for (const auto& day : days_to_remove) {
        solution.class_schedule[event.class_id].erase(day);
    }
    if (solution.class_schedule[event.class_id].empty()) {
        solution.class_schedule.erase(event.class_id);
    }

    if (solution.event_day_counts.find(event_id) != solution.event_day_counts.end() && 
        solution.event_day_counts[event_id].empty()) {
        solution.event_day_counts.erase(event_id);
    }
    if (solution.event_double_lessons.find(event_id) != solution.event_double_lessons.end() && 
        solution.event_double_lessons[event_id] == 0) {
        solution.event_double_lessons.erase(event_id);
    }
}

pair<Solution, vector<string>> IteratedGreedy::destroy(Solution solution, int destruction_rate, const Instance &instance) {
    vector<string> events_to_destroy = select_events(solution, instance, destruction_rate);

    for (const auto &event_id : events_to_destroy) {
        remove_allocations(event_id, solution, instance);
    }

    return {solution, events_to_destroy};
}

Solution IteratedGreedy::rebuild(Solution solution, vector<string> &destroyed, Instance &instance) {
    sort(destroyed.begin(), destroyed.end(), [&](const string &a, const string &b) {
        return instance.events[instance.event_index.at(a)].total_duration > 
               instance.events[instance.event_index.at(b)].total_duration;
    });

    Greedy greedy;

    for (const string &e : destroyed) {
        greedy.greedy_event_allocation(e, solution, instance, true); // Ativa o registro de horários problemáticos
    }
    return solution;
}

Solution IteratedGreedy::solve(Instance &instance, int max_iters, float  destruction_percentage ) {
    int total_events = instance.events.size();
    int destruction_rate = max(1, static_cast<int>(total_events * destruction_percentage));

    Greedy greedy;

    Solution best_solution = greedy.generate_greedy(instance, 100);
    Solution current_solution = best_solution;
    
    Evaluator evaluator;
    evaluator.evaluate(instance, best_solution);
    int best_cost = evaluator.hard_violations * 1000 + evaluator.total_cost; // Corrigido
    
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> dis(0.0, 1.0);
    
    double initial_temp = 1000.0;
    double cooling_rate = 0.95;
    double temperature = initial_temp;

    for (int i = 0; i < max_iters; i++) {
        auto [partial_solution, destroyed] = destroy(current_solution, destruction_rate, instance);
        Solution new_solution = rebuild(partial_solution, destroyed, instance);
        evaluator.evaluate(instance, new_solution);
        int new_cost = evaluator.hard_violations * 1000 + evaluator.total_cost;
        
        if (new_cost < best_cost) {
            best_solution = new_solution;
            best_cost = new_cost;
            current_solution = new_solution;
        } else {
            double delta = new_cost - best_cost;
            double acceptance_prob = exp(-delta / temperature);
            
            if (dis(gen) < acceptance_prob) {
                current_solution = new_solution;
            }
        }
        
        // Resfriamento
        temperature *= cooling_rate;
        
        // Reinício aleatório periódico
        if (i % 50 == 0 && i > 0) {
            current_solution = greedy.generate_greedy(instance, 50);
        }
    }
    return best_solution;
}

#include "../include/Greedy.h"

#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>

Solution Greedy::generate_greedy(const Instance &instance, int max_attempts)
{
    Solution sol;
    int attempt = 0;
    bool complete_solution = false;

    unordered_map<string, int> day_order = {{"Mo", 1}, {"Tu", 2}, {"We", 3}, {"Th", 4}, {"Fr", 5}};

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    mt19937 rng(seed);

    while (attempt < max_attempts && !complete_solution)
    {
        attempt++;
        sol = Solution(); // Reset
        complete_solution = true;

        for (const auto &t : instance.times) {
            sol.teacher_occupation[t.id] = unordered_set<string>();
            sol.class_occupation[t.id] = unordered_set<string>();
        }

        unordered_map<string, set<string>> teacher_used;
        unordered_map<string, set<string>> class_used;
        unordered_map<string, set<string>> event_used_days;
        unordered_map<string, int> remaining_duration;
        for (const auto &e : instance.events)
        {
            remaining_duration[e.id] = e.total_duration;
        }

        vector<EventInfo> evs = instance.events;
        shuffle(evs.begin(), evs.end(), rng);
        sort(evs.begin(), evs.end(),
             [](const EventInfo &a, const EventInfo &b)
             {
                 return a.total_duration > b.total_duration;
             });

        for (const auto &e : evs)
        {
            while (remaining_duration[e.id] >= 2)
            {
                bool allocated = false;
                vector<TimeInfo> shuffled_times = instance.times;
                shuffle(shuffled_times.begin(), shuffled_times.end(), rng);

                for (const auto &t : shuffled_times)
                {
                    if (t.max_duration < 2)
                        continue;
                    if (teacher_used[e.teacher_id].count(t.id))
                        continue;
                    if (class_used[e.class_id].count(t.id))
                        continue;
                    if (event_used_days[e.id].count(t.day))
                        continue;
                    if (instance.teacher_unavailable_times.count(e.teacher_id) &&
                        instance.teacher_unavailable_times.at(e.teacher_id).count(t.id))
                        continue;

                    Allocation alloc;
                    alloc.event_id = e.id;
                    alloc.time_id = t.id;
                    alloc.duration = 2;

                    sol.allocations.push_back(alloc);
                    sol.event_allocations[e.id].push_back(alloc);
                    sol.allocated_duration[e.id] += 2;
                    sol.event_day_counts[e.id][t.day]++;
                    sol.event_double_lessons[e.id]++;
                    sol.teacher_schedule[e.teacher_id].insert(t.day);
                    sol.class_schedule[e.class_id].insert(t.day);
                    sol.teacher_occupation[t.id].insert(e.teacher_id);
                    sol.class_occupation[t.id].insert(e.class_id);

                    teacher_used[e.teacher_id].insert(t.id);
                    class_used[e.class_id].insert(t.id);
                    event_used_days[e.id].insert(t.day);
                    remaining_duration[e.id] -= 2;
                    allocated = true;
                    break;
                }
                if (!allocated)
                    break;
            }

            while (remaining_duration[e.id] > 0)
            {
                bool allocated = false;
                vector<TimeInfo> shuffled_times = instance.times;
                shuffle(shuffled_times.begin(), shuffled_times.end(), rng);

                for (const auto &t : shuffled_times)
                {
                    if (teacher_used[e.teacher_id].count(t.id))
                        continue;
                    if (class_used[e.class_id].count(t.id))
                        continue;
                    if (event_used_days[e.id].count(t.day))
                        continue;
                    if (instance.teacher_unavailable_times.count(e.teacher_id) &&
                        instance.teacher_unavailable_times.at(e.teacher_id).count(t.id))
                        continue;

                    Allocation alloc;
                    alloc.event_id = e.id;
                    alloc.time_id = t.id;
                    alloc.duration = 1;

                    sol.allocations.push_back(alloc);
                    sol.event_allocations[e.id].push_back(alloc);
                    sol.allocated_duration[e.id] += 1;
                    sol.event_day_counts[e.id][t.day]++;
                    sol.teacher_schedule[e.teacher_id].insert(t.day);
                    sol.class_schedule[e.class_id].insert(t.day);
                    sol.teacher_occupation[t.id].insert(e.teacher_id);
                    sol.class_occupation[t.id].insert(e.class_id);

                    teacher_used[e.teacher_id].insert(t.id);
                    class_used[e.class_id].insert(t.id);
                    event_used_days[e.id].insert(t.day);
                    remaining_duration[e.id] -= 1;
                    allocated = true;
                    break;
                }
                if (!allocated)
                {
                    complete_solution = false;
                    Allocation alloc;
                    alloc.event_id = e.id;
                    alloc.time_id = "UNALLOCATED";
                    alloc.duration = remaining_duration[e.id];
                    sol.allocations.push_back(alloc);
                    sol.event_allocations[e.id].push_back(alloc);
                    sol.allocated_duration[e.id] += remaining_duration[e.id];
                    break;
                }
            }
        }
        if (complete_solution)
            break;
    }
    return sol;
}

void Greedy::greedy_event_allocation(string event_id, Solution &solution, Instance &instance, bool record_bad) {
    const EventInfo &event = instance.events[instance.event_index.at(event_id)];
    int remaining = event.total_duration - solution.allocated_duration[event_id];

    if (instance.event_index.find(event_id) == instance.event_index.end()) {
        cerr << "Evento inválido: " << event_id << endl;
        return;
    }

    auto is_teacher_free = [&](const string &time_id) {
        return solution.teacher_occupation[time_id].find(event.teacher_id) == 
               solution.teacher_occupation[time_id].end();
    };
    
    auto is_class_free = [&](const string &time_id) {
        return solution.class_occupation[time_id].find(event.class_id) == 
               solution.class_occupation[time_id].end();
    };

    auto is_day_available = [&](const string &day) {
        return solution.event_day_counts[event_id][day] == 0;
    };

    while (remaining >= 2) {
        bool allocated = false;
        vector<TimeInfo> shuffled_times = instance.times;
        unsigned seed = chrono::system_clock::now().time_since_epoch().count();
        mt19937 rng(seed);
        shuffle(shuffled_times.begin(), shuffled_times.end(), rng);

        for (const auto &t : shuffled_times) {
            if (t.max_duration < 2) continue;
            if (!is_teacher_free(t.id)) continue;
            if (!is_class_free(t.id)) continue;
            if (!is_day_available(t.day)) continue;

            if (solution.bad_times[event_id].count(t.id)) {
                continue;
            }

            if (instance.teacher_unavailable_times.count(event.teacher_id) &&
                instance.teacher_unavailable_times[event.teacher_id].count(t.id)) {
                continue;
            }

            Allocation alloc;
            alloc.event_id = event_id;
            alloc.time_id = t.id;
            alloc.duration = 2;

            solution.allocations.push_back(alloc);
            solution.event_allocations[event_id].push_back(alloc);
            solution.allocated_duration[event_id] += 2;
            solution.event_day_counts[event_id][t.day]++;
            solution.event_double_lessons[event_id]++;
            solution.teacher_schedule[event.teacher_id].insert(t.day);
            solution.class_schedule[event.class_id].insert(t.day);
            solution.teacher_occupation[t.id].insert(event.teacher_id);
            solution.class_occupation[t.id].insert(event.class_id);
            
            remaining -= 2;
            allocated = true;
            break;
        }
        if (!allocated) break;
    }

    while (remaining > 0) {
        bool allocated = false;
        vector<TimeInfo> shuffled_times = instance.times;
        unsigned seed = chrono::system_clock::now().time_since_epoch().count();
        mt19937 rng(seed);
        shuffle(shuffled_times.begin(), shuffled_times.end(), rng);

        for (const auto &t : shuffled_times) {
            if (!is_teacher_free(t.id)) continue;
            if (!is_class_free(t.id)) continue;
            if (!is_day_available(t.day)) continue;
            
            if (solution.bad_times[event_id].count(t.id)) {
                continue;
            }
            
            if (instance.teacher_unavailable_times.count(event.teacher_id) &&
                instance.teacher_unavailable_times[event.teacher_id].count(t.id)) {
                continue;
            }

            Allocation alloc;
            alloc.event_id = event_id;
            alloc.time_id = t.id;
            alloc.duration = 1;

            solution.allocations.push_back(alloc);
            solution.event_allocations[event_id].push_back(alloc);
            solution.allocated_duration[event_id] += 1;
            solution.event_day_counts[event_id][t.day]++;
            solution.teacher_schedule[event.teacher_id].insert(t.day);
            solution.class_schedule[event.class_id].insert(t.day);
            solution.teacher_occupation[t.id].insert(event.teacher_id);
            solution.class_occupation[t.id].insert(event.class_id);
            
            remaining--;
            allocated = true;
            break;
        }
        if (!allocated) {
            if (record_bad) {
                for (const auto &t : shuffled_times) {
                    if (is_teacher_free(t.id) && is_class_free(t.id) && is_day_available(t.day)) {
                        solution.bad_times[event_id].insert(t.id);
                    }
                }
            }
            Allocation alloc;
            alloc.event_id = event_id;
            alloc.time_id = "UNALLOCATED";
            alloc.duration = remaining;
            solution.allocations.push_back(alloc);
            solution.event_allocations[event_id].push_back(alloc);
            solution.allocated_duration[event_id] += remaining;
            break;
        }
    }
}

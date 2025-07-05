#include "../include/Greedy.h"

#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>

Solution Greedy::generate_greedy(const Instance &instance)
{
    Solution sol;
    bool complete_solution = false;

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    mt19937 rng(seed);

    while (!complete_solution)
    {
        sol = Solution();
        complete_solution = true;

        for (const auto &t : instance.times)
        {
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
                    if (instance.next_time.find(t.id) == instance.next_time.end())
                        continue;

                    string next_id = instance.next_time.at(t.id);

                    if (teacher_used[e.teacher_id].count(next_id) || class_used[e.class_id].count(next_id))
                        continue;
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
                    sol.teacher_occupation[next_id].insert(e.teacher_id);
                    sol.class_occupation[t.id].insert(e.class_id);
                    sol.class_occupation[next_id].insert(e.class_id);

                    teacher_used[e.teacher_id].insert(t.id);
                    teacher_used[e.teacher_id].insert(next_id);
                    class_used[e.class_id].insert(t.id);
                    class_used[e.class_id].insert(next_id);
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
                // if (!allocated)
                // {
                //     complete_solution = false;
                //     Allocation alloc;
                //     alloc.event_id = e.id;
                //     alloc.time_id = "UNALLOCATED";
                //     alloc.duration = remaining_duration[e.id];
                //     sol.allocations.push_back(alloc);
                //     sol.event_allocations[e.id].push_back(alloc);
                //     sol.allocated_duration[e.id] += remaining_duration[e.id];
                //     break;
                // }
                if (!allocated)
                {
                    complete_solution = false;
                    break;
                }
            }
        }
        if (complete_solution)
            break;
    }
    return sol;
}

void Greedy::generate_greedy(vector<string> destroyed_events, Solution &solution, Instance &instance)
{
    Solution base_solution = solution;

    bool complete_solution = false;

    vector<EventInfo> to_realocate;
    unordered_map<string, int> base_remaining_duration;

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    mt19937 rng(seed);

    for (auto id : destroyed_events)
    {
        const EventInfo event = instance.events[instance.event_index.at(id)];
        to_realocate.push_back(event);
        base_remaining_duration[id] = event.total_duration - solution.allocated_duration[id];
    }

    auto is_teacher_free = [&](const string &time_id, const EventInfo &event)
    {
        return solution.teacher_occupation[time_id].find(event.teacher_id) == solution.teacher_occupation[time_id].end();
    };

    auto is_class_free = [&](const string &time_id, const EventInfo &event)
    {
        return solution.class_occupation[time_id].find(event.class_id) == solution.class_occupation[time_id].end();
    };

    auto is_day_available = [&](const string &day, const string &event_id)
    {
        if (solution.event_day_counts.find(event_id) == solution.event_day_counts.end())
            return true;

        if (solution.event_day_counts[event_id].find(day) == solution.event_day_counts[event_id].end())
            return true;

        return solution.event_day_counts[event_id][day] == 0;
    };

    while (!complete_solution)
    {
        solution = base_solution;
        complete_solution = true;

        auto remaining_duration = base_remaining_duration;

        shuffle(to_realocate.begin(), to_realocate.end(), rng);
        sort(to_realocate.begin(), to_realocate.end(),
             [](const EventInfo &a, const EventInfo &b)
             {
                 return a.total_duration > b.total_duration;
             });

        for (const auto &e : to_realocate)
        {
            while (remaining_duration[e.id] >= 2)
            {
                bool allocated = false;
                vector<TimeInfo> shuffled_times = instance.times;
                shuffle(shuffled_times.begin(), shuffled_times.end(), rng);

                for (const auto &t : shuffled_times)
                {
                    if (instance.next_time.find(t.id) == instance.next_time.end())
                        continue;

                    string next_id = instance.next_time.at(t.id);

                    if (!is_teacher_free(next_id, e) || !is_class_free(next_id, e))
                        continue;
                    if (t.max_duration < 2)
                        continue;
                    if (!is_teacher_free(t.id, e))
                        continue;
                    if (!is_class_free(t.id, e))
                        continue;
                    if (!is_day_available(t.day, e.id))
                        continue;
                    if (instance.teacher_unavailable_times.count(e.teacher_id) && instance.teacher_unavailable_times.at(e.teacher_id).count(t.id))
                        continue;

                    Allocation alloc;
                    alloc.event_id = e.id;
                    alloc.time_id = t.id;
                    alloc.duration = 2;

                    solution.allocations.push_back(alloc);
                    solution.event_allocations[e.id].push_back(alloc);
                    solution.allocated_duration[e.id] += 2;
                    solution.event_day_counts[e.id][t.day]++;
                    solution.event_double_lessons[e.id]++;
                    solution.teacher_schedule[e.teacher_id].insert(t.day);
                    solution.class_schedule[e.class_id].insert(t.day);

                    solution.teacher_occupation[t.id].insert(e.teacher_id);
                    solution.teacher_occupation[next_id].insert(e.teacher_id);
                    solution.class_occupation[t.id].insert(e.class_id);
                    solution.class_occupation[next_id].insert(e.class_id);

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
                    if (!is_teacher_free(t.id, e))
                        continue;
                    if (!is_class_free(t.id, e))
                        continue;
                    if (!is_day_available(t.day, e.id))
                        continue;
                    if (instance.teacher_unavailable_times.count(e.teacher_id) && instance.teacher_unavailable_times[e.teacher_id].count(t.id))
                        continue;

                    Allocation alloc;
                    alloc.event_id = e.id;
                    alloc.time_id = t.id;
                    alloc.duration = 1;

                    solution.allocations.push_back(alloc);
                    solution.event_allocations[e.id].push_back(alloc);
                    solution.allocated_duration[e.id] += 1;
                    solution.event_day_counts[e.id][t.day]++;
                    solution.teacher_schedule[e.teacher_id].insert(t.day);
                    solution.class_schedule[e.class_id].insert(t.day);
                    solution.teacher_occupation[t.id].insert(e.teacher_id);
                    solution.class_occupation[t.id].insert(e.class_id);

                    remaining_duration[e.id] -= 1;
                    allocated = true;
                    break;
                }

                if (!allocated)
                {
                    complete_solution = false;
                    break;
                }
            }
        }
        if (complete_solution)
            break;
    }
}

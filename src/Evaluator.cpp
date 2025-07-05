#include "../include/Evaluator.h"

#include <iostream>
#include <algorithm>

void Evaluator::evaluate(const Instance &instance, const Solution &solution)
{
    hard_violations = 0;
    soft_violations = 0;
    total_cost = 0;
    check_hard_constraints(instance, solution);
    check_soft_constraints(instance, solution);
}

void Evaluator::check_hard_constraints(const Instance &instance, const Solution &solution)
{
    // AssignTimeConstraint
    for (const auto &event : instance.events)
    {
        int allocated = 0;
        if (solution.allocated_duration.find(event.id) != solution.allocated_duration.end())
        {
            allocated = solution.allocated_duration.at(event.id);
        }

        if (allocated != event.total_duration)
        {
            hard_violations++;
        }
    }

    // AvoidClashesConstraint
    unordered_map<string, set<string>> teacher_allocations;
    unordered_map<string, set<string>> class_allocations;

    for (const Allocation &alloc : solution.allocations)
    {
        if (alloc.time_id == "UNALLOCATED")
            continue;

        const EventInfo &event = instance.events.at(instance.event_index.at(alloc.event_id));

        if (teacher_allocations[alloc.time_id].count(event.teacher_id))
        {
            hard_violations++;
        }
        teacher_allocations[alloc.time_id].insert(event.teacher_id);

        if (class_allocations[alloc.time_id].count(event.class_id))
        {
            hard_violations++;
        }
        class_allocations[alloc.time_id].insert(event.class_id);
    }

    // AvoidUnavailableTimesConstraint
    for (const Allocation &alloc : solution.allocations)
    {
        if (alloc.time_id == "UNALLOCATED")
            continue;

        const EventInfo &event = instance.events.at(instance.event_index.at(alloc.event_id));

        if (instance.teacher_unavailable_times.count(event.teacher_id))
        {
            const auto &unavailable_times = instance.teacher_unavailable_times.at(event.teacher_id);
            if (unavailable_times.count(alloc.time_id))
            {
                hard_violations++;
            }
        }
    }

    // SpreadEventsConstraint
    for (const auto &event : instance.events)
    {
        auto day_counts_it = solution.event_day_counts.find(event.id);
        if (day_counts_it != solution.event_day_counts.end())
        {
            for (const auto &day_count : day_counts_it->second)
            {
                if (day_count.second > 1)
                {
                    hard_violations++;
                }
            }
        }
    }
}

void Evaluator::check_soft_constraints(const Instance &instance, const Solution &solution)
{
    // DistributeSplitEventsConstraint
    for (const auto &event : instance.events)
    {
        if (instance.course_split_constraints.count(event.course_id))
        {
            auto constraint = instance.course_split_constraints.at(event.course_id);
            int min_double = constraint.first;
            int max_double = constraint.second;

            int actual_double = 0;
            if (solution.event_double_lessons.count(event.id))
            {
                actual_double = solution.event_double_lessons.at(event.id);
            }

            if (actual_double < min_double || actual_double > max_double)
            {
                soft_violations++;
                int cost = 1;
                for (const auto &c : instance.constraints)
                {
                    if (c.type == "DistributeSplitEventsConstraint" &&
                        c.applies_to_events.count(event.course_id))
                    {
                        cost = c.weight;
                        break;
                    }
                }
                total_cost += cost;
            }
        }
    }

    // ClusterBusyTimesConstraint
    for (const auto &teacher_pair : instance.teacher_name)
    {
        const string &teacher_id = teacher_pair.first;

        if (instance.teacher_max_days.count(teacher_id))
        {
            int max_days = instance.teacher_max_days.at(teacher_id);
            int actual_days = 0;

            if (solution.teacher_schedule.count(teacher_id))
            {
                actual_days = solution.teacher_schedule.at(teacher_id).size();
            }

            if (actual_days > max_days)
            {
                soft_violations++;
                int cost = 1;
                for (const auto &c : instance.constraints)
                {
                    if (c.type == "ClusterBusyTimesConstraint" &&
                        c.applies_to_teachers.count(teacher_id))
                    {
                        cost = c.weight;
                        break;
                    }
                }
                total_cost += cost;
            }
        }
    }

    // LimitIdleTimesConstraint
    for (const auto &teacher_sched : solution.teacher_schedule)
    {
        const string &teacher_id = teacher_sched.first;
        const set<string> &days = teacher_sched.second;

        for (const string &day : days)
        {
            vector<int> slots;
            for (const Allocation &alloc : solution.allocations)
            {
                // Skip unallocated entries
                if (alloc.time_id == "UNALLOCATED")
                    continue;

                const EventInfo &event = instance.events.at(instance.event_index.at(alloc.event_id));
                if (event.teacher_id == teacher_id)
                {
                    const auto it_time = instance.time_index.find(alloc.time_id);
                    if (it_time == instance.time_index.end())
                        continue;
                    const TimeInfo &t = instance.times.at(it_time->second);
                    if (t.day == day)
                    {
                        slots.push_back(t.slot);
                    }
                }
            }

            sort(slots.begin(), slots.end());
            for (int i = 1; i < slots.size(); i++)
            {
                if (slots[i] - slots[i - 1] > 1)
                {
                    soft_violations++;
                    int cost = 1;
                    for (const auto &c : instance.constraints)
                    {
                        if (c.type == "LimitIdleTimesConstraint")
                        {
                            cost = c.weight;
                            break;
                        }
                    }
                    total_cost += cost;
                    break;
                }
            }
        }
    }
}

void Evaluator::print_report() const
{
    cout << "\n=== RELATÓRIO DE AVALIAÇÃO ===" << endl;
    cout << "Violações HARD: " << hard_violations << endl;
    cout << "Violações SOFT: " << soft_violations << endl;
    cout << "Custo total: " << total_cost << endl;
    cout << (hard_violations == 0 ? "SOLUÇÃO VÁLIDA" : "SOLUÇÃO INVÁLIDA") << endl;
}

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
            // cout << "VIOLAÇÃO HARD: Evento " << event.id << " não foi completamente alocado ("
            //      << allocated << "/" << event.total_duration << ")\n";
        }
    }

    // AvoidClashesConstraint
    unordered_map<string, set<string>> teacher_allocations;
    unordered_map<string, set<string>> class_allocations;

    for (const Allocation &alloc : solution.allocations)
    {
        const EventInfo &event = instance.events.at(instance.event_index.at(alloc.event_id));

        // Verificar professor
        if (teacher_allocations.find(alloc.time_id) != teacher_allocations.end())
        {
            if (teacher_allocations[alloc.time_id].find(event.teacher_id) != teacher_allocations[alloc.time_id].end())
            {
                hard_violations++;
                // cout << "VIOLAÇÃO HARD: Professor " << event.teacher_id
                //      << " alocado duas vezes no horário " << alloc.time_id << "\n";
            }
        }
        teacher_allocations[alloc.time_id].insert(event.teacher_id);

        // Verificar turma
        if (class_allocations.find(alloc.time_id) != class_allocations.end())
        {
            if (class_allocations[alloc.time_id].find(event.class_id) != class_allocations[alloc.time_id].end())
            {
                hard_violations++;
                // cout << "VIOLAÇÃO HARD: Turma " << event.class_id
                //      << " alocada duas vezes no horário " << alloc.time_id << "\n";
            }
        }
        class_allocations[alloc.time_id].insert(event.class_id);
    }

    // AvoidUnavailableTimesConstraint
    for (const Allocation &alloc : solution.allocations)
    {
        const EventInfo &event = instance.events.at(instance.event_index.at(alloc.event_id));

        if (instance.teacher_unavailable_times.find(event.teacher_id) !=
            instance.teacher_unavailable_times.end())
        {

            const auto &unavailable_times = instance.teacher_unavailable_times.at(event.teacher_id);
            if (unavailable_times.find(alloc.time_id) != unavailable_times.end())
            {
                hard_violations++;
                // cout << "VIOLAÇÃO HARD: Professor " << event.teacher_id
                //      << " alocado em horário proibido " << alloc.time_id
                //      << " no evento " << alloc.event_id << "\n";
            }
        }
    }

    // SpreadEventsConstraint
    for (const auto &event : instance.events)
    {
        if (solution.event_day_counts.find(event.id) != solution.event_day_counts.end())
        {
            for (const auto &day_count : solution.event_day_counts.at(event.id))
            {
                if (day_count.second > 1)
                {
                    hard_violations++;
                    // cout << "VIOLAÇÃO HARD: Evento " << event.id
                    //      << " tem " << day_count.second << " aulas no dia " << day_count.first << "\n";
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
        if (instance.course_split_constraints.find(event.course_id) !=
            instance.course_split_constraints.end())
        {

            auto constraint = instance.course_split_constraints.at(event.course_id);
            int min_double = constraint.first;
            int max_double = constraint.second;

            int actual_double = 0;
            if (solution.event_double_lessons.find(event.id) != solution.event_double_lessons.end())
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
                        c.applies_to_events.find(event.course_id) != c.applies_to_events.end())
                    {
                        cost = c.weight;
                        break;
                    }
                }
                total_cost += cost;
                // cout << "VIOLAÇÃO SOFT: Evento " << event.id << " tem " << actual_double
                //      << " aulas duplas (deveria ter entre " << min_double << " e " << max_double << ")\n";
            }
        }
    }

    // ClusterBusyTimesConstraint
    for (const auto &teacher : instance.teacher_name)
    {
        const string &teacher_id = teacher.first;

        if (instance.teacher_max_days.find(teacher_id) != instance.teacher_max_days.end())
        {
            int max_days = instance.teacher_max_days.at(teacher_id);
            int actual_days = 0;

            if (solution.teacher_schedule.find(teacher_id) != solution.teacher_schedule.end())
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
                        c.applies_to_teachers.find(teacher_id) != c.applies_to_teachers.end())
                    {
                        cost = c.weight;
                        break;
                    }
                }
                total_cost += cost;
                // cout << "VIOLAÇÃO SOFT: Professor " << teacher_id << " trabalhou "
                //      << actual_days << " dias (máximo permitido: " << max_days << ")\n";
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
                const EventInfo &event = instance.events.at(instance.event_index.at(alloc.event_id));
                if (event.teacher_id == teacher_id)
                {
                    const TimeInfo &t = instance.times.at(instance.time_index.at(alloc.time_id));
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
                    // cout << "VIOLAÇÃO SOFT: Professor " << teacher_id
                    //      << " tem janela livre no dia " << day << "\n";
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

#ifndef SOLUTION
#define SOLUTION

#include "Allocation.h"

#include <vector>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <iostream>

using namespace std;

class Solution
{
public:
    vector<Allocation> allocations;
    unordered_map<string, vector<Allocation>> event_allocations;
    unordered_map<string, set<string>> teacher_schedule;
    unordered_map<string, set<string>> class_schedule;
    unordered_map<string, unordered_map<string, int>> event_day_counts;
    unordered_map<string, int> event_double_lessons;
    unordered_map<string, int> allocated_duration;
    unordered_map<string, unordered_set<string>> teacher_occupation; // time_id -> teacher_ids
    unordered_map<string, unordered_set<string>> class_occupation;   // time_id -> class_ids
    
    void print(const Instance& instance) const {
        std::cout << "\n=== Detalhes da Solução ===\n";
        std::cout << "Alocações:\n";
        
        for (const auto& alloc : allocations) {
            std::cout << "- Evento: " << alloc.event_id
                      << " | Horário: " << alloc.time_id
                      << " | Duração: " << alloc.duration << "\n";
        }

        std::cout << "\nEventos não alocados:\n";
        for (const auto& [event_id, duration] : allocated_duration) {
            const EventInfo& event = instance.events[instance.event_index.at(event_id)];
            if (duration < event.total_duration) {
                std::cout << "- " << event_id << ": " 
                          << duration << "/" << event.total_duration << " alocado\n";
            }
        }

        std::cout << "\nDias de trabalho por professor:\n";
        for (const auto& [teacher, days] : teacher_schedule) {
            std::cout << "- " << teacher << ": " << days.size() << " dias\n";
        }
    }
};

#endif

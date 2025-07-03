#ifndef SOLUTION
#define SOLUTION

#include "Allocation.h"

#include <vector>
#include <unordered_map>
#include <set>
#include <unordered_set>

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
    unordered_map<string, unordered_set<string>> bad_times;
};

#endif

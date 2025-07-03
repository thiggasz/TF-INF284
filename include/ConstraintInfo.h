#ifndef CONSTRAINTINFO
#define CONSTRAINTINFO

#include <string>
#include <unordered_set>

using namespace std;

class ConstraintInfo
{
public:
    string type;
    bool required;
    int weight;
    unordered_set<string> applies_to_events;
    unordered_set<string> applies_to_teachers;
    unordered_set<string> applies_to_classes;
    unordered_set<string> applies_to_times;
    int min_value;
    int max_value;
    int duration_constraint;
    unordered_set<string> time_groups;
};

#endif
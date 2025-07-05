#ifndef INSTANCE
#define INSTANCE

#include "tinyxml2.h"
#include "TimeInfo.h"
#include "ResourceInfo.h"
#include "EventInfo.h"
#include "ConstraintInfo.h"

#include <vector>
#include <unordered_map>
#include <algorithm>

using namespace std;
using namespace tinyxml2;

class Instance
{
public:
    vector<TimeInfo> times;
    vector<ResourceInfo> resources;
    vector<EventInfo> events;
    vector<ConstraintInfo> constraints;

    unordered_map<string, int> time_index;
    unordered_map<string, int> event_index;
    unordered_map<string, string> teacher_name;

    unordered_map<string, unordered_set<string>> teacher_unavailable_times;
    unordered_map<string, pair<int, int>> course_split_constraints;
    unordered_map<string, int> teacher_max_days;
    unordered_map<string, string> next_time;

    void load(const string &filename);
};

#endif

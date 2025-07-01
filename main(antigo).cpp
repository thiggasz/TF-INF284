#include "../include/tinyxml2.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <random>

using namespace tinyxml2;
using namespace std;

struct AssignTimeConstraint;
struct SplitEventsConstraint;
struct DistributeSplitEventsConstraint;
struct PreferTimesConstraint;
struct SpreadEventsConstraint;
struct ClashesConstraint;
struct UnavailableTimesConstraint;
struct LimitIdleTimesConstraint;
struct ClusterBusyTimesConstraint;

struct AssignTimeConstraint
{
    set<string> event_groups;
};

struct SplitEventsConstraint
{
    set<string> event_groups;
};

struct DistributeSplitEventsConstraint
{
    set<string> event_groups;
    int duration;
    int minimum;
    int maximum;
    int weight;
};

struct PreferTimesConstraint
{
    set<string> timeGroups;
    int duration;
};

struct SpreadEventsConstraint
{
    set<string> event_groups;
    unordered_map<string, pair<int, int>> day_limits; // Mapeia grupo de dia -> (min, max)
};

struct ClashesConstraint
{
    set<string> resources_group;
};

struct UnavailableTimesConstraint
{
    vector<string> times;
    string resourceId;
};

struct LimitIdleTimesConstraint
{
    set<string> resourceGroups;
    int minimum;
    int maximum;
    int weight;
};

struct ClusterBusyTimesConstraint
{
    set<string> resources;
    int minimumDays;
    int maximumDays;
    int weight;
};

vector<AssignTimeConstraint> assign_times;
vector<SplitEventsConstraint> split_events;
vector<DistributeSplitEventsConstraint> distribute_split_events;
vector<PreferTimesConstraint> prefer_times;
vector<SpreadEventsConstraint> spread_constraints;
vector<ClashesConstraint> clashes_constraint;
vector<UnavailableTimesConstraint> unavailable_times;
vector<LimitIdleTimesConstraint> limit_idle;
vector<ClusterBusyTimesConstraint> cluster_busy;

unordered_map<string, pair<int, int>> day_slot_index;
unordered_map<string, pair<int, int>> teacher_class_index;
unordered_map<string, int> teacher_index;
unordered_map<string, int> class_index;
unordered_map<string, int> slot_day_index;
unordered_map<string, string> slot_to_day_group;
unordered_map<string, bool> slot_in_timedurationtwo;
unordered_map<string, set<string>> resource_group_teachers;
unordered_map<string, string> event_to_course_group;
unordered_map<string, set<string>> event_to_event_groups;
unordered_map<string, int> slot_to_order;
unordered_map<string, set<string>> slot_time_groups;

vector<string> index_day_slot;
vector<string> index_teacher_class;
vector<int> remaining_duration;
vector<vector<int>> event_day_count;

int num_teachers = 0;
int num_classes = 0;

void load_instance(vector<string> &paths)
{
    for (string path : paths)
    {
        XMLDocument doc;
        if (doc.LoadFile(path.c_str()) != XML_SUCCESS)
        {
            cerr << "Erro ao carregar o arquivo XML\n";
            return;
        }

        XMLElement *root = doc.FirstChildElement("HighSchoolTimetableArchive");
        if (!root)
        {
            cerr << "Error: Could not find HighSchoolTimetableArchive root element in " << path << "\n";
            return;
        }

        XMLElement *instances = root->FirstChildElement("Instances");
        XMLElement *instance = instances->FirstChildElement("Instance");
        if (!instance)
        {
            cerr << "Error: No Instance element found in " << path << "\n";
            return;
        }
        else
        {
            XMLElement *times = instance->FirstChildElement("Times");
            if (!times)
            {
                cerr << "Error: No Times element found in " << path << "\n";
                return;
            }

            int count = 0;
            for (XMLElement *time = times->FirstChildElement("Time"); time; time = time->NextSiblingElement("Time"))
            {
                string id = time->Attribute("Id");
                XMLElement *timeGroup = time->FirstChildElement("TimeGroups")->FirstChildElement("TimeGroup");
                size_t pos = id.find('_');
                if (pos != string::npos)
                    slot_to_order[id] = stoi(id.substr(pos + 1));

                if (timeGroup && string(timeGroup->Attribute("Reference")) == "gr_TimesDurationTwo")
                {
                    slot_in_timedurationtwo[id] = true;
                }
                else
                {
                    slot_in_timedurationtwo[id] = false;
                }

                if (timeGroup && string(timeGroup->Attribute("Reference")) == "gr_TimesDurationTwo")
                {
                    day_slot_index.insert(make_pair(id, make_pair(count, 2)));
                    index_day_slot.push_back(id);
                }
                else
                {
                    day_slot_index.insert(make_pair(id, make_pair(count, 1)));
                    index_day_slot.push_back(id);
                }

                XMLElement *dayRef = time->FirstChildElement("Day");
                if (dayRef)
                {
                    const char *dayRefId = dayRef->Attribute("Reference");
                    slot_to_day_group[id] = dayRefId;

                    if (strcmp(dayRefId, "gr_Mo") == 0)
                        slot_day_index[id] = 0;
                    else if (strcmp(dayRefId, "gr_Tu") == 0)
                        slot_day_index[id] = 1;
                    else if (strcmp(dayRefId, "gr_We") == 0)
                        slot_day_index[id] = 2;
                    else if (strcmp(dayRefId, "gr_Th") == 0)
                        slot_day_index[id] = 3;
                    else if (strcmp(dayRefId, "gr_Fr") == 0)
                        slot_day_index[id] = 4;
                    else
                        slot_day_index[id] = -1;
                }

                count++;
            }

            XMLElement *resources = instance->FirstChildElement("Resources");
            if (!resources)
            {
                cerr << "Error: No Resources element found in " << path << "\n";
                return;
            }
            for (XMLElement *r = resources->FirstChildElement("Resource");
                 r;
                 r = r->NextSiblingElement("Resource"))
            {
                {
                    const char *id = r->Attribute("Id");
                    XMLElement *rt = r->FirstChildElement("ResourceType");
                    if (!rt)
                        continue;
                    const char *type = rt->Attribute("Reference");
                    if (strcmp(type, "Teacher") == 0)
                    {
                        teacher_index.insert(make_pair(id, num_teachers++));
                    }
                    else if (strcmp(type, "Class") == 0)
                    {
                        class_index.insert(make_pair(id, num_classes++));
                    }

                    XMLElement *rg = r->FirstChildElement("ResourceGroups");
                    if (rg)
                    {
                        for (XMLElement *group = rg->FirstChildElement("ResourceGroup"); group; group = group->NextSiblingElement("ResourceGroup"))
                        {
                            string group_id = group->Attribute("Reference");
                            resource_group_teachers[group_id].insert(id);
                        }
                    }
                }
            }

            XMLElement *events = instance->FirstChildElement("Events");
            if (!events)
            {
                cerr << "Error: No Events element found in " << path << "\n";
                return;
            }

            count = 0;
            for (XMLElement *event = events->FirstChildElement("Event"); event; event = event->NextSiblingElement("Event"))
            {
                string id = event->Attribute("Id");
                XMLElement *course = event->FirstChildElement("Course");
                if (course)
                    event_to_course_group[id] = course->Attribute("Reference");

                // Carregar grupos de eventos
                set<string> groups;
                XMLElement *egs = event->FirstChildElement("EventGroups");
                if (egs)
                {
                    for (XMLElement *eg = egs->FirstChildElement("EventGroup"); eg; eg = eg->NextSiblingElement("EventGroup"))
                    {
                        groups.insert(eg->Attribute("Reference"));
                    }
                }
                event_to_event_groups[id] = groups;

                int duration = atoi(event->FirstChildElement("Duration")->GetText());

                teacher_class_index.insert(make_pair(id, make_pair(count, duration)));
                index_teacher_class.push_back(id);
                remaining_duration.push_back(duration);

                count++;
            }

            XMLElement *constraints = instance->FirstChildElement("Constraints");
            if (!constraints)
            {
                cerr << "Warning: No Constraints element found in " << path << "\n";
                continue;
            }
            for (XMLElement *c = constraints->FirstChildElement();
                 c; c = c->NextSiblingElement())
            {
                string type = c->Name();

                if (type == "AssignTimeConstraint")
                {
                    AssignTimeConstraint ac;
                    for (XMLElement *eg = c->FirstChildElement("AppliesTo")
                                              ->FirstChildElement("EventGroups")
                                              ->FirstChildElement("EventGroup");
                         eg; eg = eg->NextSiblingElement("EventGroup"))
                        ac.event_groups.insert(eg->Attribute("Reference"));
                    assign_times.push_back(move(ac));
                }
                else if (type == "SplitEventsConstraint")
                {
                    SplitEventsConstraint sc;
                    for (XMLElement *eg = c->FirstChildElement("AppliesTo")
                                              ->FirstChildElement("EventGroups")
                                              ->FirstChildElement("EventGroup");
                         eg; eg = eg->NextSiblingElement("EventGroup"))
                        sc.event_groups.insert(eg->Attribute("Reference"));
                    split_events.push_back(move(sc));
                }
                else if (type == "DistributeSplitEventsConstraint")
                {
                    DistributeSplitEventsConstraint dc;
                    for (XMLElement *eg = c->FirstChildElement("AppliesTo")
                                              ->FirstChildElement("EventGroups")
                                              ->FirstChildElement("EventGroup");
                         eg; eg = eg->NextSiblingElement("EventGroup"))
                        dc.event_groups.insert(eg->Attribute("Reference"));
                    dc.duration = c->IntAttribute("Duration");
                    dc.minimum = c->IntAttribute("Minimum");
                    dc.maximum = c->IntAttribute("Maximum");
                    dc.weight = c->IntAttribute("Weight");
                    distribute_split_events.push_back(move(dc));
                }
                else if (type == "PreferTimesConstraint")
                {
                    PreferTimesConstraint pc;
                    pc.duration = c->IntAttribute("Duration");
                    for (XMLElement *tg = c->FirstChildElement("TimeGroups")
                                              ->FirstChildElement("TimeGroup");
                         tg; tg = tg->NextSiblingElement("TimeGroup"))
                        pc.timeGroups.insert(tg->Attribute("Reference"));
                    prefer_times.push_back(move(pc));
                }
                else if (type == "SpreadEventsConstraint")
                {
                    SpreadEventsConstraint sc;
                    for (XMLElement *eg = c->FirstChildElement("AppliesTo")
                                              ->FirstChildElement("EventGroups")
                                              ->FirstChildElement("EventGroup");
                         eg; eg = eg->NextSiblingElement("EventGroup"))
                        sc.event_groups.insert(eg->Attribute("Reference"));

                    for (XMLElement *tg = c->FirstChildElement("TimeGroups")
                                              ->FirstChildElement("TimeGroup");
                         tg; tg = tg->NextSiblingElement("TimeGroup"))
                    {
                        string time_group = tg->Attribute("Reference");
                        int min_val = tg->IntAttribute("Minimum");
                        int max_val = tg->IntAttribute("Maximum");
                        sc.day_limits[time_group] = make_pair(min_val, max_val);
                    }
                    spread_constraints.push_back(move(sc));
                }
                else if (type == "AvoidClashesConstraint")
                {
                    ClashesConstraint cc;
                    clashes_constraint.push_back(move(cc));
                }
                else if (type.rfind("AvoidUnavailableTimesConstraint", 0) == 0)
                {
                    UnavailableTimesConstraint uc;
                    uc.resourceId = c->FirstChildElement("AppliesTo")
                                        ->FirstChildElement("Resources")
                                        ->FirstChildElement("Resource")
                                        ->Attribute("Reference");
                    for (XMLElement *t = c->FirstChildElement("Times")
                                             ->FirstChildElement("Time");
                         t; t = t->NextSiblingElement("Time"))
                        uc.times.push_back(t->Attribute("Reference"));
                    unavailable_times.push_back(move(uc));
                }
                else if (type == "LimitIdleTimesConstraint")
                {
                    LimitIdleTimesConstraint lic;
                    for (XMLElement *rg = c->FirstChildElement("AppliesTo")
                                              ->FirstChildElement("ResourceGroups")
                                              ->FirstChildElement("ResourceGroup");
                         rg; rg = rg->NextSiblingElement("ResourceGroup"))
                        lic.resourceGroups.insert(rg->Attribute("Reference"));
                    lic.minimum = c->IntAttribute("Minimum");
                    lic.maximum = c->IntAttribute("Maximum");
                    lic.weight = c->IntAttribute("Weight");
                    limit_idle.push_back(move(lic));
                }
                else if (type == "ClusterBusyTimesConstraint")
                {
                    ClusterBusyTimesConstraint cbc;
                    for (XMLElement *r = c->FirstChildElement("AppliesTo")
                                             ->FirstChildElement("Resources")
                                             ->FirstChildElement("Resource");
                         r; r = r->NextSiblingElement("Resource"))
                        cbc.resources.insert(r->Attribute("Reference"));
                    cbc.minimumDays = c->IntAttribute("Minimum");
                    cbc.maximumDays = c->IntAttribute("Maximum");
                    cbc.weight = c->IntAttribute("Weight");
                    cluster_busy.push_back(move(cbc));
                }
            }
        }
    }
}

void validate_info()
{
    for (const auto &entry : teacher_index)
    {
        cout << "Teacher ID: " << entry.first << ", Index: " << entry.second << "\n";
    }

    cout << "\n";

    for (const auto &entry : class_index)
    {
        cout << "Class ID: " << entry.first << ", Index: " << entry.second << "\n";
    }

    cout << "\n";

    cout << day_slot_index.size() << " " << teacher_class_index.size() << "\n";

    for (const auto &entry : day_slot_index)
    {
        cout << "Time ID: " << entry.first << ", Index: " << entry.second.first << ", Duration: " << entry.second.second << "\n";
    }

    cout << "\n";

    for (const auto &entry : teacher_class_index)
    {
        cout << "Teacher-Class ID: " << entry.first << ", Index: " << entry.second.first << ", Duration: " << entry.second.second << "\n";
    }

    cout << "\n";

    cout << "Parsed: " << assign_times.size() << " assign times constraints\n";
    cout << "Parsed: " << split_events.size() << " split events constraints\n";
    cout << "Parsed: " << distribute_split_events.size() << " distribute split events constraints\n";
    cout << "Parsed: " << prefer_times.size() << " prefer time constraints\n";
    cout << "Parsed: " << spread_constraints.size() << " spread constraints constraints\n";
    cout << "Parsed: " << clashes_constraint.size() << " clashes constraint constraints\n";
    cout << "Parsed: " << unavailable_times.size() << " unavailable times constraints\n";
    cout << "Parsed: " << limit_idle.size() << " limit-idle constraints\n";
    cout << "Parsed: " << cluster_busy.size() << " cluster-busy-times constraints\n";
}

bool is_valid(int event_idx, int slot_idx, vector<vector<int>> &occ_teacher, vector<vector<int>> &occ_class, vector<vector<int>> &event_day_count)
{
    string teacher_class = index_teacher_class[event_idx];
    size_t pos = teacher_class.find('-');
    if (pos == string::npos)
        return false;

    string t = teacher_class.substr(0, pos);
    string c = teacher_class.substr(pos + 1);

    int index_t = teacher_index[t];
    int index_c = class_index[c];

    if (occ_teacher[index_t][slot_idx] >= 1 || occ_class[index_c][slot_idx] >= 1)
    {
        return false;
    }

    for (const auto &uc : unavailable_times)
    {
        if (uc.resourceId == t)
        {
            string slot_name = index_day_slot[slot_idx];
            for (const string &forbidden : uc.times)
            {
                if (forbidden == slot_name)
                    return false;
            }
        }
    }

    if (slot_day_index.find(index_day_slot[slot_idx]) != slot_day_index.end())
    {
        int day = slot_day_index[index_day_slot[slot_idx]];
        if (event_day_count[event_idx][day] >= 1)
        {
            return false;
        }
    }

    int slot_duration = day_slot_index[index_day_slot[slot_idx]].second;
    if (slot_duration > remaining_duration[event_idx])
    {
        return false;
    }

    return true;
}

int evaluate(vector<vector<int>> &solution,
             vector<vector<int>> &occ_teacher,
             vector<vector<int>> &occ_class,
             vector<vector<int>> &event_day_count,
             const vector<int> &original_durations)
{
    const int HARD_PENALTY = 1000000;
    int num_slots  = index_day_slot.size();
    int num_events = solution.size();
    int cost = 0;

    // 1. Reset and rebuild occupancy and day count
    for (auto &row : occ_teacher) fill(row.begin(), row.end(), 0);
    for (auto &row : occ_class)   fill(row.begin(), row.end(), 0);
    event_day_count.assign(num_events, vector<int>(5, 0));

    vector<int> rem = original_durations;
    for (int i = 0; i < num_events; i++) {
        auto ids = index_teacher_class[i];
        string teacher_id = ids.substr(0, ids.find(':'));
        string class_id   = ids.substr(ids.find(':') + 1);
        int t_idx = teacher_index[teacher_id];
        int c_idx = class_index[class_id];

        for (int slot_idx : solution[i]) {
            string slot_name = index_day_slot[slot_idx];
            int slot_dur = day_slot_index[slot_name].second;
            rem[i] -= slot_dur;
            occ_teacher[t_idx][slot_idx]++;
            occ_class[c_idx][slot_idx]++;
            int day = slot_day_index[slot_name];
            if (day >= 0 && day < 5) event_day_count[i][day]++;
        }
    }

    // 2. Duration mismatches
    for (int i = 0; i < num_events; i++) {
        if (rem[i] != 0) cost += HARD_PENALTY * abs(rem[i]);
    }

    // 3. Clashes
    for (int t = 0; t < num_teachers; t++)
        for (int s = 0; s < num_slots; s++)
            if (occ_teacher[t][s] > 1)
                cost += HARD_PENALTY * (occ_teacher[t][s] - 1);

    for (int c = 0; c < num_classes; c++)
        for (int s = 0; s < num_slots; s++)
            if (occ_class[c][s] > 1)
                cost += HARD_PENALTY * (occ_class[c][s] - 1);

    // 4. SpreadEventsConstraint
    for (auto &sc : spread_constraints) {
        for (int i = 0; i < num_events; i++) {
            string ev = index_teacher_class[i];
            bool applies = false;
            for (auto &eg : sc.event_groups)
                if (event_to_event_groups[ev].count(eg)) applies = true;
            if (!applies) continue;

            for (auto &dl : sc.day_limits) {
                int day = -1;
                for (const auto &pair : slot_day_index) {
                    const string &slot_id = pair.first;
                    int day_idx = pair.second;
                    if (slot_to_day_group[slot_id] == dl.first) {
                        day = day_idx;
                        break;
                    }
                }
                if (day == -1) continue;

                int cnt = event_day_count[i][day];
                int minv = dl.second.first, maxv = dl.second.second;
                if (cnt < minv) cost += HARD_PENALTY * (minv - cnt);
                if (cnt > maxv) cost += HARD_PENALTY * (cnt - maxv);
            }
        }
    }

    // 5. UnavailableTimesConstraint
    for (auto &uc : unavailable_times) {
        if (!teacher_index.count(uc.resourceId)) continue;
        int t = teacher_index[uc.resourceId];
        for (int s = 0; s < num_slots; s++) {
            if (occ_teacher[t][s] && 
                find(uc.times.begin(), uc.times.end(), index_day_slot[s]) != uc.times.end())
                cost += HARD_PENALTY;
        }
    }

    // 6. PreferTimesConstraint (hard)
    for (auto &pc : prefer_times) {
        for (int i = 0; i < num_events; i++) {
            for (int s : solution[i]) {
                string slot = index_day_slot[s];
                bool ok = false;
                for (auto &tg : pc.timeGroups)
                    if (slot_to_day_group[slot] == tg) { ok = true; break; }
                if (!ok) cost += HARD_PENALTY;
            }
        }
    }

    // 7. SplitEventsConstraint
    for (int i = 0; i < num_events; i++)
        for (int s : solution[i])
            if (day_slot_index[index_day_slot[s]].second != 1 &&
                day_slot_index[index_day_slot[s]].second != 2)
                cost += HARD_PENALTY;

    // 8. ClusterBusyTimesConstraint
    for (auto &c : cluster_busy) {
        for (auto &rid : c.resources) {
            if (!teacher_index.count(rid)) continue;
            int t = teacher_index[rid];
            set<int> busy;
            for (int s = 0; s < num_slots; s++)
                if (occ_teacher[t][s]) busy.insert(slot_day_index[index_day_slot[s]]);
            int d = busy.size();
            if (d < c.minimumDays) cost += c.weight * (c.minimumDays - d);
            if (d > c.maximumDays) cost += c.weight * (d - c.maximumDays);
        }
    }

    // 9. LimitIdleTimesConstraint
    for (auto &c : limit_idle) {
        set<string> teachers;
        for (auto &rg : c.resourceGroups)
            if (resource_group_teachers.count(rg))
                teachers.insert(resource_group_teachers[rg].begin(),
                                resource_group_teachers[rg].end());

        for (auto &tid : teachers) {
            if (!teacher_index.count(tid)) continue;
            int t = teacher_index[tid];
            for (int day = 0; day < 5; day++) {
                vector<int> slots;
                for (int s = 0; s < num_slots; s++)
                    if (occ_teacher[t][s] && slot_day_index[index_day_slot[s]] == day)
                        slots.push_back(slot_to_order[index_day_slot[s]]);
                sort(slots.begin(), slots.end());
                int gaps = 0;
                for (int k = 1; k < (int)slots.size(); k++)
                    if (slots[k] - slots[k-1] > 1) gaps++;
                if (gaps < c.minimum) cost += c.weight * (c.minimum - gaps);
                if (gaps > c.maximum) cost += c.weight * (gaps - c.maximum);
            }
        }
    }

    // 10. DistributeSplitEventsConstraint
    for (auto &c : distribute_split_events) {
        int cnt = 0;
        for (int i = 0; i < num_events; i++) {
            string ev = index_teacher_class[i];
            bool applies = false;
            for (auto &eg : event_to_event_groups[ev])
                if (c.event_groups.count(eg)) applies = true;
            if (!applies) continue;

            for (int s : solution[i])
                if (day_slot_index[index_day_slot[s]].second == c.duration)
                    cnt++;
        }
        if (cnt < c.minimum) cost += c.weight * (c.minimum - cnt);
        if (cnt > c.maximum) cost += c.weight * (cnt - c.maximum);
    }

    return cost;
}


void greedy_solution(const vector<int> &original_durations)
{
    int num_slots = index_day_slot.size();
    int num_events = index_teacher_class.size();

    vector<int> remaining = original_durations;
    vector<vector<int>> occ_teacher(num_teachers, vector<int>(num_slots, 0));
    vector<vector<int>> occ_class(num_classes, vector<int>(num_slots, 0));
    vector<vector<int>> event_day_count(num_events, vector<int>(5, 0));
    vector<vector<int>> solution(num_events);
    vector<int> last_unallocated(num_events, 0);

    mt19937_64 rng(random_device{}());
    int last_unallocated_count = num_events;

    while (true)
    {
        vector<int> unallocated;
        for (int e = 0; e < num_events; ++e)
        {
            if (remaining[e] <= 0)
                continue;

            vector<int> domain;
            for (int s = 0; s < num_slots; ++s)
            {
                if (is_valid(e, s, occ_teacher, occ_class, event_day_count))
                {
                    int dur = day_slot_index[index_day_slot[s]].second;
                    if (dur <= remaining[e])
                        domain.push_back(s);
                }
            }

            if (domain.empty())
            {
                unallocated.push_back(e);
                continue;
            }

            shuffle(domain.begin(), domain.end(), rng);
            bool allocated = false;
            for (int s : domain)
            {
                if (!is_valid(e, s, occ_teacher, occ_class, event_day_count))
                    continue;

                int dur = day_slot_index[index_day_slot[s]].second;
                if (dur > remaining[e])
                    continue;

                // Alocar este slot
                solution[e].push_back(s);
                remaining[e] -= dur;
                int day = slot_day_index[index_day_slot[s]];
                if (day >= 0 && day < 5)
                    event_day_count[e][day]++;

                string tc = index_teacher_class[e];
                size_t pos = tc.find('-');
                string t = tc.substr(0, pos), c = tc.substr(pos + 1);
                occ_teacher[teacher_index[t]][s] += 1;
                occ_class[class_index[c]][s] += 1;
                allocated = true;
                break;
            }
            if (!allocated)
                unallocated.push_back(e);
        }

        if (unallocated.empty())
            break;
        if (unallocated.size() >= last_unallocated_count)
            break; // Estagnação
        last_unallocated_count = unallocated.size();
    }

    cout << "\nFinal Solution:\n";
    for (int i = 0; i < solution.size(); i++)
    {
        cout << "Event " << index_teacher_class[i]
             << " (Rem: " << remaining[i] << "): ";
        for (int slot : solution[i])
        {
            cout << index_day_slot[slot]
                 << "(" << day_slot_index[index_day_slot[slot]].second << ") ";
        }
        cout << "\n";
    }
    int total_cost = evaluate(solution, occ_teacher, occ_class, event_day_count, original_durations);
    cout << "Solution cost: " << total_cost << endl;
}

void load_and_evaluate_solution(const string &sol_path, const vector<int> &original_durations)
{
    XMLDocument doc;
    if (doc.LoadFile(sol_path.c_str()) != XML_SUCCESS)
    {
        cerr << "Erro ao carregar arquivo de solução: " << sol_path << "\n";
        return;
    }

    XMLElement *root = doc.FirstChildElement("HighSchoolTimetableArchive");
    if (!root)
    {
        cerr << "Formato inválido: raiz não encontrada\n";
        return;
    }

    // Loop através de todos os SolutionGroups
    XMLElement *solGroup = root->FirstChildElement("SolutionGroups")->FirstChildElement("SolutionGroup");
    while (solGroup)
    {
        cout << "\nProcessando SolutionGroup: " << solGroup->Attribute("Id") << "\n";

        // Loop através de todas as Solutions dentro do grupo
        XMLElement *solutionElem = solGroup->FirstChildElement("Solution");
        while (solutionElem)
        {
            // Reinicializar estruturas para CADA solução
            vector<vector<int>> solution(index_teacher_class.size());
            vector<vector<int>> occ_teacher(num_teachers, vector<int>(index_day_slot.size(), 0));
            vector<vector<int>> occ_class(num_classes, vector<int>(index_day_slot.size(), 0));
            vector<vector<int>> evt_day_count(index_teacher_class.size(), vector<int>(5, 0));

            // Mapeamento ID Evento -> Índice
            unordered_map<string, int> event_id_to_index;
            for (int i = 0; i < index_teacher_class.size(); i++)
            {
                event_id_to_index[index_teacher_class[i]] = i;
            }

            // Processar cada evento na solução
            XMLElement *eventsElem = solutionElem->FirstChildElement("Events");
            for (XMLElement *eventElem = eventsElem->FirstChildElement("Event");
                 eventElem;
                 eventElem = eventElem->NextSiblingElement("Event"))
            {

                const char *ref = eventElem->Attribute("Reference");
                if (!ref)
                    continue;

                string event_id = ref;
                auto it = event_id_to_index.find(event_id);
                if (it == event_id_to_index.end())
                {
                    cerr << "Evento desconhecido: " << event_id << "\n";
                    continue;
                }
                int event_idx = it->second;

                XMLElement *durationElem = eventElem->FirstChildElement("Duration");
                XMLElement *timeElem = eventElem->FirstChildElement("Time");
                if (!durationElem || !timeElem)
                    continue;

                int duration = atoi(durationElem->GetText());
                const char *time_ref = timeElem->Attribute("Reference");
                if (!time_ref)
                    continue;

                string slot_id = time_ref;
                auto slot_it = day_slot_index.find(slot_id);
                if (slot_it == day_slot_index.end())
                {
                    cerr << "Slot desconhecido: " << slot_id << "\n";
                    continue;
                }
                int slot_idx = slot_it->second.first;

                // Adicionar slot ao evento
                solution[event_idx].push_back(slot_idx);

                // Recursos do evento
                string tc = index_teacher_class[event_idx];
                size_t pos = tc.find('-');
                string t = tc.substr(0, pos);
                string c = tc.substr(pos + 1);

                int t_idx = teacher_index[t];
                int c_idx = class_index[c];

                // Atualizar ocupações
                occ_teacher[t_idx][slot_idx] += 1;
                occ_class[c_idx][slot_idx] += 1;

                // Atualizar contagem por dia
                auto day_it = slot_day_index.find(slot_id);
                if (day_it != slot_day_index.end() && day_it->second >= 0)
                {
                    int day = day_it->second;
                    if (day < 5)
                        evt_day_count[event_idx][day]++;
                }

                // DEBUG: Mostrar alocação
                cout << "  Alocado: " << event_id << " em " << slot_id
                     << " (Duração: " << duration << ")\n";
            }

            // Avaliar esta solução específica
            int cost = evaluate(solution, occ_teacher, occ_class, evt_day_count, original_durations);
            cout << "Custo desta solução: " << cost << "\n";

            solutionElem = solutionElem->NextSiblingElement("Solution");
        }
        solGroup = solGroup->NextSiblingElement("SolutionGroup");
    }
}

int main()
{
    vector<string> instance_paths = {"../instances/instance1.xml"};
    load_instance(instance_paths);

    vector<int> orig_durations;
    orig_durations.reserve(index_teacher_class.size());
    for (const auto &event_id : index_teacher_class)
    {
        auto [idx, duration] = teacher_class_index[event_id];
        orig_durations.push_back(duration);
    }

    load_and_evaluate_solution("../instances/original/instance1_sol.xml", orig_durations);

    greedy_solution(orig_durations);

    return 0;
}
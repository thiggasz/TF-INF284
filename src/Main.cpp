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
};

struct PreferTimesConstraint
{
    set<string> timeGroups;
    int duration;
};

struct SpreadEventsConstraint
{
    set<string> event_groups; //"gr_T1-S1"
    int minPerDay;
    int maxPerDay;
};

struct ClashesConstraint
{
    set<string> resources_group;
};

struct UnavailableTimesConstraint
{
    vector<string> times; // "Mo_1"
    string resourceId;    // "T1"
};

struct LimitIdleConstraint
{
    set<string> resourceGroups;
    int minimum;
    int maximum;
};

struct ClusterBusyTimesConstraint
{
    set<string> resources;
    int minimumDays;
    int maximumDays;
};

vector<AssignTimeConstraint> assign_times;
vector<SplitEventsConstraint> split_events;
vector<DistributeSplitEventsConstraint> distribute_split_events;
vector<PreferTimesConstraint> prefer_times;
vector<SpreadEventsConstraint> spread_constraints;
vector<ClashesConstraint> clashes_constraint;
vector<UnavailableTimesConstraint> unavailable_times;
vector<LimitIdleConstraint> limit_idle;
vector<ClusterBusyTimesConstraint> cluster_busy;

unordered_map<string, pair<int, int>> day_slot_index;      // index "Mo_1"
unordered_map<string, pair<int, int>> teacher_class_index; // index "T1-S1"
unordered_map<string, int> teacher_index;                  // index "S1"
unordered_map<string, int> class_index;                    // index "T1
unordered_map<string, int> slot_day_index;                 // index "Mo"
unordered_map<string, string> slot_to_day_group; // Slot -> Grupo do dia (gr_Mo, etc.)
unordered_map<string, bool> slot_in_timedurationtwo; // Slot ID -> true if in gr_TimesDurationTwo

vector<string> index_day_slot;
vector<string> index_teacher_class;
vector<int> remaining_duration;
vector<vector<int>> event_day_count; // [event_index][day] = count

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

                if (timeGroup && string(timeGroup->Attribute("Reference")) == "gr_TimesDurationTwo") {
                    slot_in_timedurationtwo[id] = true;
                } else {
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

                XMLElement* dayRef = time->FirstChildElement("Day");
                if (dayRef) {
                    const char* dayRefId = dayRef->Attribute("Reference");
                    slot_to_day_group[id] = dayRefId;
                    
                    if (strcmp(dayRefId, "gr_Mo") == 0) slot_day_index[id] = 0;
                    else if (strcmp(dayRefId, "gr_Tu") == 0) slot_day_index[id] = 1;
                    else if (strcmp(dayRefId, "gr_We") == 0) slot_day_index[id] = 2;
                    else if (strcmp(dayRefId, "gr_Th") == 0) slot_day_index[id] = 3;
                    else if (strcmp(dayRefId, "gr_Fr") == 0) slot_day_index[id] = 4;
                    else slot_day_index[id] = -1;
                }

                count++;
            }

            XMLElement *resources = instance->FirstChildElement("Resources");
            if (!times)
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
                    // dentro de <Resource> ... <ResourceType Reference="Teacher" /> ou "Class"
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
                    XMLElement *tg = c->FirstChildElement("TimeGroups")
                                         ->FirstChildElement("TimeGroup");
                    sc.minPerDay = tg->IntAttribute("Minimum");
                    sc.maxPerDay = tg->IntAttribute("Maximum");
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
                    LimitIdleConstraint lic;
                    for (XMLElement *rg = c->FirstChildElement("AppliesTo")
                                              ->FirstChildElement("ResourceGroups")
                                              ->FirstChildElement("ResourceGroup");
                         rg; rg = rg->NextSiblingElement("ResourceGroup"))
                        lic.resourceGroups.insert(rg->Attribute("Reference"));
                    lic.minimum = c->IntAttribute("Minimum");
                    lic.maximum = c->IntAttribute("Maximum");
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

bool is_valid(int event_idx, int slot_idx, vector<vector<int>> &occ_teacher, vector<vector<int>> &occ_class, vector<vector<int>> &event_day_count) {
    string teacher_class = index_teacher_class[event_idx];
    size_t pos = teacher_class.find('-');
    if (pos == string::npos) return false;
    
    string t = teacher_class.substr(0, pos);
    string c = teacher_class.substr(pos + 1);
    
    int index_t = teacher_index[t];
    int index_c = class_index[c];
    
    // AvoidClashesConstraint
    if (occ_teacher[index_t][slot_idx] || occ_class[index_c][slot_idx]) {
        return false;
    }
    
    // AvoidUnavailableTimesConstraint
    for (const auto& uc : unavailable_times) {
        if (uc.resourceId == t) {
            string slot_name = index_day_slot[slot_idx];
            for (const string& forbidden : uc.times) {
                if (forbidden == slot_name) return false;
            }
        }
    }
    
    // SpreadEventsConstraint (max 1 parte por dia)
    if (slot_day_index.find(index_day_slot[slot_idx]) != slot_day_index.end()) {
        int day = slot_day_index[index_day_slot[slot_idx]];
        if (event_day_count[event_idx][day] >= 1) {
            return false;
        }
    }
    
    // Duração compatível
    int slot_duration = day_slot_index[index_day_slot[slot_idx]].second;
    if (slot_duration > remaining_duration[event_idx]) {
        return false;
    }
    
    return true;
}

// hard: alocar todas os eventos durante sua duração;
// dividir eventos em horário simples e duplo
// eventos duplos devem ser alocados em horários válidos
// nenhum recurso(professor/classe) pode ser alocado em dois eventos no mesmo slot
// cada evento no máximo uma aula por dia
// professores não podem ser alocados em dias que não trabalham
//
// soft: exatamente 1 horário duplo(1)
// exatamente 2 horários duplos(1), ao menos
// nenhuma janela livre entre aulas de um mesmo professor em um dia
// para um conjunto de professores no máximo 2 dias de aula por semana

int evaluate(vector<vector<int>> &solution) {
    int cost = 0;
    const int HARD_PENALTY = 1000000;
    
    // AssignTimeConstraint (eventos não alocados)
    for (int i = 0; i < remaining_duration.size(); i++) {
        if (remaining_duration[i] > 0) {
            cost += HARD_PENALTY;
        }
    }
    
    // SplitEventsConstraint (partes válidas)
    for (int event_idx = 0; event_idx < solution.size(); event_idx++) {
        for (int slot_idx : solution[event_idx]) {
            int slot_duration = day_slot_index[index_day_slot[slot_idx]].second;
            if (slot_duration != 1 && slot_duration != 2) {
                cost += HARD_PENALTY;
            }
        }
    }
    
    // PreferTimesConstraint (partes de 2 tempos)
    for (int event_idx = 0; event_idx < solution.size(); event_idx++) {
        for (int slot_idx : solution[event_idx]) {
            int slot_duration = day_slot_index[index_day_slot[slot_idx]].second;
            if (slot_duration == 2) {
                string slot_id = index_day_slot[slot_idx];
                if (!slot_in_timedurationtwo[slot_id]) {
                    cost += HARD_PENALTY;
                }
            }
        }
    }
    
    return cost;
}

void greedy_solution() {
    int num_slots = index_day_slot.size();
    int num_events = index_teacher_class.size();

    vector<vector<int>> occ_teacher(num_teachers, vector<int>(num_slots, 0));
    vector<vector<int>> occ_class(num_classes, vector<int>(num_slots, 0));
    vector<vector<int>> event_day_count(num_events, vector<int>(5, 0));
    vector<vector<int>> solution(num_events);

    mt19937_64 rng(random_device{}());
    for (int pass = 0; pass < 3; pass++) {
        vector<vector<int>> domains(num_events);
        for (int e = 0; e < num_events; ++e) {
            if (remaining_duration[e] <= 0) continue;
            for (int s = 0; s < num_slots; ++s) {
                if (is_valid(e, s, occ_teacher, occ_class, event_day_count)) {
                    int dur = day_slot_index[index_day_slot[s]].second;
                    if (dur <= remaining_duration[e])
                        domains[e].push_back(s);
                }
            }
        }

        vector<int> event_order;
        for (int e = 0; e < num_events; ++e)
            if (remaining_duration[e] > 0)
                event_order.push_back(e);
        sort(event_order.begin(), event_order.end(),
             [&](int a, int b) {
                 return domains[a].size() < domains[b].size();
             });

        for (int e : event_order) {
            auto &dom = domains[e];
            if (dom.empty()) continue;            
            shuffle(dom.begin(), dom.end(), rng);

            for (int s : dom) {
                if (!is_valid(e, s, occ_teacher, occ_class, event_day_count))
                    continue;
                int dur = day_slot_index[index_day_slot[s]].second;
                if (dur > remaining_duration[e]) continue;

                solution[e].push_back(s);
                remaining_duration[e] -= dur;
                int day = slot_day_index[index_day_slot[s]];
                event_day_count[e][day]++;

                string tc = index_teacher_class[e];
                auto pos = tc.find('-');
                string t = tc.substr(0, pos), c = tc.substr(pos+1);
                occ_teacher[teacher_index[t]][s] = 1;
                occ_class[class_index[c]][s] = 1;
                break;
            }
        }
    }

    vector<int> unallocated;
    for (int i = 0; i < num_events; i++)
        if (remaining_duration[i] > 0)
            unallocated.push_back(i);
    for (int idx : unallocated) {
        for (int s = 0; s < num_slots; s++) {
            if (!is_valid(idx, s, occ_teacher, occ_class, event_day_count))
                continue;
            int dur = day_slot_index[index_day_slot[s]].second;
            if (dur > remaining_duration[idx]) continue;
            solution[idx].push_back(s);
            remaining_duration[idx] -= dur;
            int day = slot_day_index[index_day_slot[s]];
            event_day_count[idx][day]++;
            string tc = index_teacher_class[idx];
            auto pos = tc.find('-');
            string t = tc.substr(0, pos), c = tc.substr(pos+1);
            occ_teacher[teacher_index[t]][s] = 1;
            occ_class[class_index[c]][s] = 1;
            break;
        }
    }

    cout << "\nFinal Solution:\n";
    for (int i = 0; i < solution.size(); i++) {
        cout << "Event " << index_teacher_class[i]
             << " (Rem: " << remaining_duration[i] << "): ";
        for (int slot : solution[i]) {
            cout << index_day_slot[slot]
                 << "(" << day_slot_index[index_day_slot[slot]].second << ") ";
        }
        cout << "\n";
    }
    int total_cost = evaluate(solution);
    cout << "Solution cost: " << total_cost << endl;
}

int main()
{
    vector<string> paths = {
        "../instances/instance1.xml",
        "../instances/instance2.xml",
        "../instances/instance3.xml",
        "../instances/instance4.xml",
        "../instances/instance5.xml",
        "../instances/instance6.xml"
    };

    load_instance(paths);
    
    remaining_duration.clear();
    for (const auto& el : teacher_class_index) {
        remaining_duration.push_back(el.second.second);
    }
    
    greedy_solution();
    return 0;
}

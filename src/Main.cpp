#include "../include/tinyxml2.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>

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
    set<string> event_groups; // e.g., {"gr_T1-S1", ...}
    int minPerDay;
    int maxPerDay;
};

struct ClashesConstraint
{
    set<string> resources_group;
};

struct UnavailableTimesConstraint
{
    vector<string> times; // e.g., "Mo_1", "Tu_3"
    string resourceId;    // e.g., "T1"
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

unordered_map<string, pair<int, int>> day_slot_index;      // "Mo_1"
unordered_map<string, pair<int, int>> teacher_class_index; // "T1-S1"

vector<string> index_day_slot;
vector<string> index_teacher_class;
vector<int> remaining_duration;

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
                    // dentro de <Resource> ... <ResourceType Reference="Teacher" /> ou "Class"
                    XMLElement *rt = r->FirstChildElement("ResourceType");
                    if (!rt)
                        continue;
                    const char *type = rt->Attribute("Reference");
                    if (strcmp(type, "Teacher") == 0)
                    {
                        ++num_teachers;
                    }
                    else if (strcmp(type, "Class") == 0)
                    {
                        ++num_classes;
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
                string id = event->Attribute("Id"); // problema com const char?
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
                continue; // Skip to next file instead of return
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
    cout << day_slot_index.size() << " " << teacher_class_index.size() << "\n";

    for (const auto &entry : day_slot_index)
    {
        cout << "Time ID: " << entry.first << ", Index: " << entry.second.first << ", Duration: " << entry.second.second << "\n";
    }

    cout << "\n";

    for (int i = 0; i < index_day_slot.size(); i++)
    {
        cout << "Time ID: " << index_day_slot[i] << ", Index: " << i << "\n";
    }

    cout << "\n";

    for (const auto &entry : teacher_class_index)
    {
        cout << "Teacher-Class ID: " << entry.first << ", Index: " << entry.second.first << ", Duration: " << entry.second.second << "\n";
    }

    cout << "\n";

    for (int i = 0; i < index_teacher_class.size(); i++)
    {
        cout << "Teacher-Class ID: " << index_teacher_class[i] << " Remaining: " << remaining_duration[i] << ", Index: " << i << "\n";
    }
}

int evaluate()
{
    // hard: alocar todas os eventos durante sua duração;
    // dividir eventos em horário simples e duplo
    // eventos duplos devem ser alocados em horários válidos
    // nenhum recurso(professor/aluno) pode ser alocado em dois eventos no mesmo slot
    // cada evento no máximo uma aula por dia
    // professores não podem ser alocados em dias que não trabalham
    //
    // soft: exatamente 1 horário duplo(1)
    // exatamente 2 horários duplos(1), ao menos
    // nenhuma janela livre entre aulas de um mesmo professor em um dia
    // para um conjunto de professores no máximo 2 dias de aula por semana

    // Rascunho
    /*
    Cost(sol) =
    H  × (  Vh_assign   + Vh_clashes   + Vh_split
          + Vh_prefer    + Vh_spread   + Vh_unavail   )
  + w₁ × Vsoft_distribute
  + w₂ × Vsoft_idle
  + w₃ × Vsoft_cluster

    */

    return 0; // Placeholder return value
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
    validate_info();

    int num_slots = 25, num_events = teacher_class_index.size();

    cout << "view:" << teacher_class_index.size() << " " <<  num_classes * num_teachers << endl;

    vector<vector<int>> occTeacher(num_teachers, vector<int>(num_slots, 0));
    vector<vector<int>> occClass(num_classes, vector<int>(num_slots, 0));

    vector<int> assign(num_events, -1);

    cout << "Parsed " << assign_times.size() << " assign times constraints\n";
    cout << "Parsed " << split_events.size() << " split events constraints\n";
    cout << "Parsed " << distribute_split_events.size() << " distribute split events constraints\n";
    cout << "Parsed " << prefer_times.size() << " prefer time constraints\n";
    cout << "Parsed " << spread_constraints.size() << " spread constraints constraints\n";
    cout << "Parsed " << clashes_constraint.size() << " clashes constraint constraints\n";
    cout << "Parsed " << unavailable_times.size() << " unavailable times constraints\n";
    cout << "Parsed " << limit_idle.size() << " limit-idle constraints\n";
    cout << "Parsed " << cluster_busy.size() << " cluster-busy-times constraints\n";

    return 0;
}

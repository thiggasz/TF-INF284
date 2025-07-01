#include "../include/tinyxml2.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <set>
#include <string>
#include <sstream>

using namespace tinyxml2;
using namespace std;

// Estruturas de dados para a instância
struct TimeInfo
{
    string id;
    string day;
    int slot;
    int max_duration; // 1 ou 2
};

struct ResourceInfo
{
    string id;
    string name;
    string type; // "Teacher" ou "Class"
};

struct EventInfo
{
    string id;
    int total_duration;
    string course_id;
    string teacher_id;
    string class_id;
};

struct ConstraintInfo
{
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
    unordered_map<string, string> className;

    unordered_map<string, unordered_set<string>> teacher_unavailable_times;
    unordered_map<string, pair<int, int>> course_split_constraints;
    unordered_map<string, int> teacher_max_days;

    void load(const string &filename);
};

void Instance::load(const string &filename)
{
    XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS)
    {
        cerr << "Erro ao carregar o arquivo XML: " << filename << endl;
        return;
    }

    XMLElement *root = doc.FirstChildElement("HighSchoolTimetableArchive");
    if (!root)
    {
        cerr << "Elemento raiz não encontrado!" << endl;
        return;
    }

    XMLElement *instance = root->FirstChildElement("Instances")->FirstChildElement("Instance");
    if (!instance)
    {
        cerr << "Instância não encontrada!" << endl;
        return;
    }

    // Carregar tempos
    XMLElement *times_elem = instance->FirstChildElement("Times");
    int time_count = 0;
    for (XMLElement *time_elem = times_elem->FirstChildElement("Time"); time_elem; time_elem = time_elem->NextSiblingElement("Time"))
    {
        TimeInfo t;
        t.id = time_elem->Attribute("Id");

        // Obter dia
        XMLElement *day_elem = time_elem->FirstChildElement("Day");
        if (day_elem)
        {
            t.day = day_elem->Attribute("Reference");
            if (t.day.find("gr_") == 0)
            {
                t.day = t.day.substr(3); // Remover "gr_"
            }
        }

        // Obter slot
        XMLElement *name_elem = time_elem->FirstChildElement("Name");
        if (name_elem && name_elem->GetText())
        {
            string name = name_elem->GetText();
            size_t pos = name.find('_');
            if (pos != string::npos)
            {
                t.slot = stoi(name.substr(pos + 1));
            }
        }

        // Verificar duração máxima
        t.max_duration = 1;
        XMLElement *timeGroups = time_elem->FirstChildElement("TimeGroups");
        if (timeGroups)
        {
            XMLElement *tg = timeGroups->FirstChildElement("TimeGroup");
            while (tg)
            {
                if (tg->Attribute("Reference") && string(tg->Attribute("Reference")) == "gr_TimesDurationTwo")
                {
                    t.max_duration = 2;
                    break;
                }
                tg = tg->NextSiblingElement("TimeGroup");
            }
        }

        times.push_back(t);
        time_index[t.id] = time_count++;
    }

    // Carregar recursos
    XMLElement *resources_elem = instance->FirstChildElement("Resources");
    for (XMLElement *res_elem = resources_elem->FirstChildElement("Resource"); res_elem; res_elem = res_elem->NextSiblingElement("Resource"))
    {
        ResourceInfo r;
        r.id = res_elem->Attribute("Id");

        XMLElement *name_elem = res_elem->FirstChildElement("Name");
        if (name_elem && name_elem->GetText())
        {
            r.name = name_elem->GetText();
        }

        XMLElement *type_elem = res_elem->FirstChildElement("ResourceType");
        if (type_elem && type_elem->Attribute("Reference"))
        {
            r.type = type_elem->Attribute("Reference");
        }

        resources.push_back(r);

        if (r.type == "Teacher")
        {
            teacher_name[r.id] = r.name;
        }
        else if (r.type == "Class")
        {
            className[r.id] = r.name;
        }
    }

    // Carregar eventos
    XMLElement *events_elem = instance->FirstChildElement("Events");
    int event_count = 0;
    for (XMLElement *event_elem = events_elem->FirstChildElement("Event"); event_elem; event_elem = event_elem->NextSiblingElement("Event"))
    {
        EventInfo e;
        e.id = event_elem->Attribute("Id");

        XMLElement *duration_elem = event_elem->FirstChildElement("Duration");
        if (duration_elem && duration_elem->GetText())
        {
            e.total_duration = atoi(duration_elem->GetText());
        }

        XMLElement *course_elem = event_elem->FirstChildElement("Course");
        if (course_elem && course_elem->Attribute("Reference"))
        {
            e.course_id = course_elem->Attribute("Reference");
        }

        // Obter recursos (professor e turma)
        XMLElement *resources = event_elem->FirstChildElement("Resources");
        if (resources)
        {
            for (XMLElement *res_elem = resources->FirstChildElement("Resource"); res_elem; res_elem = res_elem->NextSiblingElement("Resource"))
            {
                XMLElement *role_elem = res_elem->FirstChildElement("Role");
                if (role_elem && role_elem->GetText())
                {
                    string role = role_elem->GetText();
                    string res_id = res_elem->Attribute("Reference");

                    if (role == "Teacher")
                        e.teacher_id = res_id;
                    else if (role == "Class")
                        e.class_id = res_id;
                }
            }
        }

        events.push_back(e);
        event_index[e.id] = event_count++;
    }

    // Carregar restrições
    XMLElement *constraints_elem = instance->FirstChildElement("Constraints");
    for (XMLElement *constr_elem = constraints_elem->FirstChildElement(); constr_elem; constr_elem = constr_elem->NextSiblingElement())
    {
        ConstraintInfo c;
        c.type = constr_elem->Value();

        XMLElement *required_elem = constr_elem->FirstChildElement("Required");
        if (required_elem && required_elem->GetText())
        {
            c.required = (string(required_elem->GetText()) == "true");
        }

        XMLElement *weight_elem = constr_elem->FirstChildElement("Weight");
        if (weight_elem && weight_elem->GetText())
        {
            c.weight = atoi(weight_elem->GetText());
        }

        // Aplicação da restrição
        XMLElement *applies_to = constr_elem->FirstChildElement("AppliesTo");
        if (applies_to)
        {
            // Eventos
            XMLElement *event_groups = applies_to->FirstChildElement("EventGroups");
            if (event_groups)
            {
                for (XMLElement *eg = event_groups->FirstChildElement("EventGroup"); eg; eg = eg->NextSiblingElement("EventGroup"))
                {
                    if (eg->Attribute("Reference"))
                    {
                        c.applies_to_events.insert(eg->Attribute("Reference"));
                    }
                }
            }

            // Professores
            XMLElement *teachers = applies_to->FirstChildElement("Resources");
            if (teachers)
            {
                for (XMLElement *t = teachers->FirstChildElement("Resource"); t; t = t->NextSiblingElement("Resource"))
                {
                    if (t->Attribute("Reference"))
                    {
                        c.applies_to_teachers.insert(t->Attribute("Reference"));
                    }
                }
            }

            // Turmas
            XMLElement *classes = applies_to->FirstChildElement("ResourceGroups");
            if (classes)
            {
                for (XMLElement *cl = classes->FirstChildElement("ResourceGroup"); cl; cl = cl->NextSiblingElement("ResourceGroup"))
                {
                    if (cl->Attribute("Reference"))
                    {
                        c.applies_to_classes.insert(cl->Attribute("Reference"));
                    }
                }
            }

            // Horários
            XMLElement *times = applies_to->FirstChildElement("Times");
            if (times)
            {
                for (XMLElement *tm = times->FirstChildElement("Time"); tm; tm = tm->NextSiblingElement("Time"))
                {
                    if (tm->Attribute("Reference"))
                    {
                        c.applies_to_times.insert(tm->Attribute("Reference"));
                    }
                }
            }
        }

        // Parâmetros específicos
        if (c.type == string("DistributeSplitEventsConstraint"))
        {
            XMLElement *duration_elem = constr_elem->FirstChildElement("Duration");
            if (duration_elem && duration_elem->GetText())
            {
                c.duration_constraint = atoi(duration_elem->GetText());
            }

            XMLElement *min_elem = constr_elem->FirstChildElement("Minimum");
            if (min_elem && min_elem->GetText())
            {
                c.min_value = atoi(min_elem->GetText());
            }

            XMLElement *max_elem = constr_elem->FirstChildElement("Maximum");
            if (max_elem && max_elem->GetText())
            {
                c.max_value = atoi(max_elem->GetText());
            }

            // Armazenar para uso posterior
            for (const string &course_id : c.applies_to_events)
            {
                course_split_constraints[course_id] = make_pair(c.min_value, c.max_value);
            }
        }
        else if (c.type == string("ClusterBusyTimesConstraint"))
        {
            XMLElement *min_elem = constr_elem->FirstChildElement("Minimum");
            if (min_elem && min_elem->GetText())
            {
                c.min_value = atoi(min_elem->GetText());
            }

            XMLElement *max_elem = constr_elem->FirstChildElement("Maximum");
            if (max_elem && max_elem->GetText())
            {
                c.max_value = atoi(max_elem->GetText());
            }

            for (const string &teacher_id : c.applies_to_teachers)
            {
                teacher_max_days[teacher_id] = c.max_value;
            }
        }
        else if (c.type == string("AvoidUnavailableTimesConstraint"))
        {
            for (const string &teacher_id : c.applies_to_teachers)
            {
                for (const string &time_id : c.applies_to_times)
                {
                    teacher_unavailable_times[teacher_id].insert(time_id);
                }
            }
        }

        constraints.push_back(c);
    }
}

// Representação de uma alocação
struct Allocation
{
    string event_id;
    string time_id;
    int duration;
};

// Representação de uma solução
struct Solution
{
    vector<Allocation> allocations;
    unordered_map<string, vector<Allocation>> event_allocations;
    unordered_map<string, set<string>> teacher_schedule;
    unordered_map<string, set<string>> class_schedule;
    unordered_map<string, unordered_map<string, int>> event_day_counts;
    unordered_map<string, int> event_double_lessons;
    unordered_map<string, int> allocated_duration;
};

// Classe para avaliação de soluções
class Evaluator
{
public:
    int hard_violations = 0;
    int soft_violations = 0;
    int total_cost = 0;

    void evaluate(const Instance &instance, const Solution &solution);
    void print_report() const;

private:
    void check_hard_constraints(const Instance &instance, const Solution &solution);
    void check_soft_constraints(const Instance &instance, const Solution &solution);
};

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
    // 1. Verificar se todos os eventos foram completamente alocados
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
            cout << "VIOLAÇÃO HARD: Evento " << event.id << " não foi completamente alocado ("
                 << allocated << "/" << event.total_duration << ")\n";
        }
    }

    // 2. Verificar conflitos de recursos
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
                cout << "VIOLAÇÃO HARD: Professor " << event.teacher_id
                     << " alocado duas vezes no horário " << alloc.time_id << "\n";
            }
        }
        teacher_allocations[alloc.time_id].insert(event.teacher_id);

        // Verificar turma
        if (class_allocations.find(alloc.time_id) != class_allocations.end())
        {
            if (class_allocations[alloc.time_id].find(event.class_id) != class_allocations[alloc.time_id].end())
            {
                hard_violations++;
                cout << "VIOLAÇÃO HARD: Turma " << event.class_id
                     << " alocada duas vezes no horário " << alloc.time_id << "\n";
            }
        }
        class_allocations[alloc.time_id].insert(event.class_id);
    }

    // 3. Verificar horários proibidos
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
                cout << "VIOLAÇÃO HARD: Professor " << event.teacher_id
                     << " alocado em horário proibido " << alloc.time_id
                     << " no evento " << alloc.event_id << "\n";
            }
        }
    }

    // 4. Verificar limite de aulas por dia por evento
    for (const auto &event : instance.events)
    {
        if (solution.event_day_counts.find(event.id) != solution.event_day_counts.end())
        {
            for (const auto &day_count : solution.event_day_counts.at(event.id))
            {
                if (day_count.second > 1)
                {
                    hard_violations++;
                    cout << "VIOLAÇÃO HARD: Evento " << event.id
                         << " tem " << day_count.second << " aulas no dia " << day_count.first << "\n";
                }
            }
        }
    }
}

void Evaluator::check_soft_constraints(const Instance &instance, const Solution &solution)
{
    // 1. Verificar restrições de divisão de aulas
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
                int cost = 1; // Valor padrão
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
                cout << "VIOLAÇÃO SOFT: Evento " << event.id << " tem " << actual_double
                     << " aulas duplas (deveria ter entre " << min_double << " e " << max_double << ")\n";
            }
        }
    }

    // 2. Verificar dias de trabalho dos professores
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
                int cost = 1; // Valor padrão
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
                cout << "VIOLAÇÃO SOFT: Professor " << teacher_id << " trabalhou "
                     << actual_days << " dias (máximo permitido: " << max_days << ")\n";
            }
        }
    }

    // 3. Verificar janelas livres entre aulas
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
                    int cost = 1; // Valor padrão
                    for (const auto &c : instance.constraints)
                    {
                        if (c.type == "LimitIdleTimesConstraint")
                        {
                            cost = c.weight;
                            break;
                        }
                    }
                    total_cost += cost;
                    cout << "VIOLAÇÃO SOFT: Professor " << teacher_id
                         << " tem janela livre no dia " << day << "\n";
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

// Função para carregar solução do XML
vector<Solution> load_solutions_from_xml(const string &filename, const Instance &instance) {
    XMLDocument doc;
    vector<Solution> solutions;

    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS) {
        cerr << "Erro ao carregar o arquivo XML: " << filename << endl;
        return solutions;
    }

    XMLElement *root = doc.FirstChildElement("HighSchoolTimetableArchive");
    if (!root) return solutions;

    // Percorrer todos os grupos de solução
    XMLElement *solution_groups = root->FirstChildElement("SolutionGroups");
    if (!solution_groups) return solutions;

    for (XMLElement *group = solution_groups->FirstChildElement("SolutionGroup"); 
         group; 
         group = group->NextSiblingElement("SolutionGroup")) {
        
        // Percorrer todas as soluções dentro do grupo
        for (XMLElement *solution_elem = group->FirstChildElement("Solution"); 
             solution_elem; 
             solution_elem = solution_elem->NextSiblingElement("Solution")) {
            
            Solution solution;
            XMLElement *events_elem = solution_elem->FirstChildElement("Events");
            if (!events_elem) continue;

            for (XMLElement *event_elem = events_elem->FirstChildElement("Event"); 
                 event_elem; 
                 event_elem = event_elem->NextSiblingElement("Event")) {
                
                Allocation alloc;
                alloc.event_id = event_elem->Attribute("Reference");

                XMLElement *duration_elem = event_elem->FirstChildElement("Duration");
                if (duration_elem && duration_elem->GetText()) {
                    alloc.duration = atoi(duration_elem->GetText());
                }

                XMLElement *time_elem = event_elem->FirstChildElement("Time");
                if (time_elem && time_elem->Attribute("Reference")) {
                    alloc.time_id = time_elem->Attribute("Reference");
                }

                solution.allocations.push_back(alloc);
                solution.event_allocations[alloc.event_id].push_back(alloc);
                solution.allocated_duration[alloc.event_id] += alloc.duration;

                if (instance.event_index.find(alloc.event_id) != instance.event_index.end()) {
                    const EventInfo &event = instance.events.at(instance.event_index.at(alloc.event_id));
                    const TimeInfo &t = instance.times.at(instance.time_index.at(alloc.time_id));
                    
                    solution.event_day_counts[alloc.event_id][t.day]++;
                    solution.teacher_schedule[event.teacher_id].insert(t.day);
                    
                    if (alloc.duration == 2) {
                        solution.event_double_lessons[alloc.event_id]++;
                    }
                }
            }
            solutions.push_back(solution);
        }
    }
    return solutions;
}

int main() {
    Instance instance;
    instance.load("../instances/instance1.xml");
    
    vector<Solution> solutions = load_solutions_from_xml("../instances/original/instance1_sol.xml", instance);
    
    for (size_t i = 0; i < solutions.size(); ++i) {
        cout << "\n=== Avaliando solução " << i+1 << " ===" << endl;
        Evaluator evaluator;
        evaluator.evaluate(instance, solutions[i]);
        evaluator.print_report();
    }
    
    return 0;
}
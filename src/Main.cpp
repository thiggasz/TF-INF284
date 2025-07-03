#include "../include/Instance.h"
#include "../include/IteratedGreedy.h"
#include "../include/BeeColony.h"
#include "../include/tinyxml2.h"

#include <iostream>

using namespace tinyxml2;
using namespace std;

vector<Solution> load_solutions_from_xml(const string &filename, const Instance &instance)
{
    XMLDocument doc;
    vector<Solution> solutions;

    if (doc.LoadFile(filename.c_str()) != XML_SUCCESS)
    {
        cerr << "Erro ao carregar o arquivo XML: " << filename << endl;
        return solutions;
    }

    XMLElement *root = doc.FirstChildElement("HighSchoolTimetableArchive");
    if (!root)
        return solutions;

    // Percorrer todos os grupos de solução
    XMLElement *solution_groups = root->FirstChildElement("SolutionGroups");
    if (!solution_groups)
        return solutions;

    for (XMLElement *group = solution_groups->FirstChildElement("SolutionGroup");
         group;
         group = group->NextSiblingElement("SolutionGroup"))
    {

        // Percorrer todas as soluções dentro do grupo
        for (XMLElement *solution_elem = group->FirstChildElement("Solution");
             solution_elem;
             solution_elem = solution_elem->NextSiblingElement("Solution"))
        {

            Solution solution;
            XMLElement *events_elem = solution_elem->FirstChildElement("Events");
            if (!events_elem)
                continue;

            for (XMLElement *event_elem = events_elem->FirstChildElement("Event");
                 event_elem;
                 event_elem = event_elem->NextSiblingElement("Event"))
            {

                Allocation alloc;
                alloc.event_id = event_elem->Attribute("Reference");

                XMLElement *duration_elem = event_elem->FirstChildElement("Duration");
                if (duration_elem && duration_elem->GetText())
                {
                    alloc.duration = atoi(duration_elem->GetText());
                }

                XMLElement *time_elem = event_elem->FirstChildElement("Time");
                if (time_elem && time_elem->Attribute("Reference"))
                {
                    alloc.time_id = time_elem->Attribute("Reference");
                }

                solution.allocations.push_back(alloc);
                solution.event_allocations[alloc.event_id].push_back(alloc);
                solution.allocated_duration[alloc.event_id] += alloc.duration;

                if (instance.event_index.find(alloc.event_id) != instance.event_index.end())
                {
                    const EventInfo &event = instance.events.at(instance.event_index.at(alloc.event_id));
                    const TimeInfo &t = instance.times.at(instance.time_index.at(alloc.time_id));

                    solution.event_day_counts[alloc.event_id][t.day]++;
                    solution.teacher_schedule[event.teacher_id].insert(t.day);

                    if (alloc.duration == 2)
                    {
                        solution.event_double_lessons[alloc.event_id]++;
                    }
                }
            }
            solutions.push_back(solution);
        }
    }
    return solutions;
}

int main()
{
    string path = "instances/instance7.xml";
    Instance instance;
    instance.load(path);

    vector<Solution> solutions = load_solutions_from_xml(path, instance);
    for (size_t i = 0; i < solutions.size(); ++i)
    {
        cout << "\n=== Avaliando solução " << i + 1 << " ===" << endl;
        Evaluator evaluator;
        evaluator.evaluate(instance, solutions[i]);
        evaluator.print_report();
    }

    cout << "\n===Solução gulosa ===" << endl;
    Greedy greedy;
    Solution init = greedy.generate_greedy(instance, 100);
    Evaluator ev_g;
    ev_g.evaluate(instance, init);
    ev_g.print_report();

    cout << "\n===Solução IG ===" << endl;
    IteratedGreedy iterated_greedy;
    Solution sol = iterated_greedy.solve(instance, 200, 0.4);
    Evaluator ev_ig;
    ev_ig.evaluate(instance, sol);
    ev_ig.print_report();

    cout << "\n=== Melhor solução Bee Colony ===" << endl;
    // Parâmetros do ABC
    int population = 15;       // Tamanho da população
    int limit = 50;    // Limite de tentativas antes do abandono
    int max_cycles = 200; // Número máximo de ciclos
    double destruction_rate = 0.15; // Taxa de destruição

    BeeColony bee_colony(instance);
    bee_colony.solve(population, limit, max_cycles, destruction_rate);
    
    Solution best_solution = bee_colony.getBestSolution();

    // Avaliar a melhor solução
    Evaluator evaluator;
    evaluator.evaluate(instance, best_solution);
    evaluator.print_report();

    return 0;
}
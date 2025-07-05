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

    XMLElement *solution_groups = root->FirstChildElement("SolutionGroups");
    if (!solution_groups)
        return solutions;

    for (XMLElement *group = solution_groups->FirstChildElement("SolutionGroup");
         group;
         group = group->NextSiblingElement("SolutionGroup"))
    {
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

#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>

std::string solutionToXML(
    const std::vector<Allocation> &allocations,
    const std::string &instanceId = "BrazilInstance1_XHSTT-v2014")
{
    // Gerar data atual no formato "December 2011"
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream dateStream;
    dateStream << std::put_time(&tm, "%B %Y");
    std::string currentDate = dateStream.str();

    // Início do documento XML
    std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    xml += "<HighSchoolTimetableArchive>\n";
    xml += "  <SolutionGroups>\n";
    xml += "    <SolutionGroup Id=\"GeneratedSolution\">\n";
    xml += "      <MetaData>\n";
    xml += "        <Contributor>Automated Solution Generator</Contributor>\n";
    xml += "        <Date>" + currentDate + "</Date>\n";
    xml += "        <Description>Solution generated programmatically</Description>\n";
    xml += "      </MetaData>\n";
    xml += "      <Solution Reference=\"" + instanceId + "\">\n";
    xml += "        <Events>\n";

    // Adicionar cada alocação como evento
    for (const auto &alloc : allocations)
    {
        xml += "          <Event Reference=\"" + alloc.event_id + "\">\n";
        xml += "            <Duration>" + std::to_string(alloc.duration) + "</Duration>\n";
        xml += "            <Time Reference=\"" + alloc.time_id + "\"/>\n";
        xml += "          </Event>\n";
    }

    // Fechar tags do XML
    xml += "        </Events>\n";
    xml += "      </Solution>\n";
    xml += "    </SolutionGroup>\n";
    xml += "  </SolutionGroups>\n";
    xml += "</HighSchoolTimetableArchive>";

    return xml;
}

int main()
{
    string path = "instances/instance1.xml";

    Instance instance;
    instance.load(path);

    Evaluator evaluator;

    vector<Solution> solutions = load_solutions_from_xml(path, instance);
    for (size_t i = 0; i < solutions.size(); ++i)
    {
        break;
        cout << "\n=== Avaliando solução " << i + 1 << " ===" << endl;
        solutions[i].print(instance);
        Evaluator evaluator;
        evaluator.evaluate(instance, solutions[i]);
        evaluator.print_report();
    }

    // cout << "\n=== Solução gulosa ===" << endl;
    // Greedy greedy;
    // Solution initial_solution = greedy.generate_greedy(instance);
    // initial_solution.print(instance);
    // evaluator.evaluate(instance, initial_solution);
    // evaluator.print_report();

    // cout << "\n=== Solução por Iterated Greedy ===" << endl;
    IteratedGreedy iterated_greedy;
    Solution iterated_greedy_solution = iterated_greedy.solve(instance, 200, 0.3);
    // iterated_greedy_solution.print(instance);
    // evaluator.evaluate(instance, iterated_greedy_solution);
    // evaluator.print_report();

    // cout << "\n=== Melhor solução Bee Colony ===" << endl;

    // int population = 15;
    // int limit = 50;
    // int max_cycles = 200;
    // double destruction_rate = 0.15;

    // BeeColony bee_colony(instance);
    // bee_colony.solve(population, limit, max_cycles, destruction_rate);

    // Solution bee_colony_solution = bee_colony.getBestSolution();
    // bee_colony_solution.print(instance);

    // evaluator.evaluate(instance, bee_colony_solution);
    // evaluator.print_report();

    std::string xmlSolution = solutionToXML(iterated_greedy_solution.allocations);
    std::cout << xmlSolution << std::endl;

    return 0;
}
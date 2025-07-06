#include "../include/Instance.h"
#include "../include/IteratedGreedy.h"
#include "../include/BeeColony.h"
#include "../include/tinyxml2.h"

#include <chrono>
#include <ctime>
#include <iomanip>

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
                    alloc.duration = atoi(duration_elem->GetText());

                XMLElement *time_elem = event_elem->FirstChildElement("Time");
                if (time_elem && time_elem->Attribute("Reference"))
                    alloc.time_id = time_elem->Attribute("Reference");

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
                        solution.event_double_lessons[alloc.event_id]++;
                }
            }

            solutions.push_back(solution);
        }
    }

    return solutions;
}

string solutionToXML(
    const vector<Allocation> &allocations,
    const string &instanceId = "BrazilInstance1_XHSTT-v2014")
{
    auto t = time(nullptr);
    auto tm = *localtime(&t);
    ostringstream dateStream;
    dateStream << put_time(&tm, "%B %Y");
    string currentDate = dateStream.str();

    string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
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

    for (const auto &alloc : allocations)
    {
        xml += "          <Event Reference=\"" + alloc.event_id + "\">\n";
        xml += "            <Duration>" + to_string(alloc.duration) + "</Duration>\n";
        xml += "            <Time Reference=\"" + alloc.time_id + "\"/>\n";
        xml += "          </Event>\n";
    }

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

    cout << path << endl;

    cout << "\n=== Solução por Greedy ===" << endl;

    Greedy greedy;

    auto start = chrono::high_resolution_clock::now();
    Solution initial_solution = greedy.generate_greedy(instance);
    auto end = chrono::high_resolution_clock::now();

    // initial_solution.print(instance);
    evaluator.evaluate(instance, initial_solution);
    evaluator.print_report();

    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Solução por Greedy - Tempo de execução: " << duration.count() << " milissegundos" << endl;

    cout << "\n=== Solução por Iterated Greedy ===" << endl;

    IteratedGreedy iterated_greedy;

    start = chrono::high_resolution_clock::now();
    Solution iterated_greedy_solution = iterated_greedy.solve(instance, 200, 0.3, 200);
    end = chrono::high_resolution_clock::now();

    // iterated_greedy_solution.print(instance);
    evaluator.evaluate(instance, iterated_greedy_solution);
    evaluator.print_report();

    duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Solução por Iterated Greedy - Tempo de execução: " << duration.count() << " milissegundos" << endl;

    // string xmlSolution_ig = solutionToXML(iterated_greedy_solution.allocations);
    // cout << "\n"
    //      << xmlSolution_ig << endl;

    cout << "\n=== Solução por Artificial Bee Colony ===" << endl;

    int population = 10;
    int limit = 30;
    int max_cycles = 100;
    double destruction_rate = 0.20;

    BeeColony bee_colony(instance);

    start = chrono::high_resolution_clock::now();
    bee_colony.solve(population, limit, max_cycles, destruction_rate);
    end = chrono::high_resolution_clock::now();

    duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    cout << "Solução por Artificial Bee Colony - Tempo de execução: " << duration.count() << " milissegundos" << endl;

    Solution bee_colony_solution = bee_colony.getBestSolution();
    // bee_colony_solution.print(instance);

    evaluator.evaluate(instance, bee_colony_solution);
    evaluator.print_report();

    // string xmlSolution_bee = solutionToXML(bee_colony_solution.allocations);
    // cout << "\n"
    //      << xmlSolution_bee << endl;

    return 0;
}

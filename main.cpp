#include "tinyxml2.h"
#include <iostream>
#include <string>

using namespace tinyxml2;
using namespace std;

int main() {
    XMLDocument doc;
    if (doc.LoadFile("instances/instance1.xml") != XML_SUCCESS) {
        cerr << "Erro ao carregar o arquivo XML\n";
        return 1;
    }

    XMLElement* root = doc.FirstChildElement("HighSchoolTimetableArchive");
    if (!root) return 1;

    XMLElement* instance = root->FirstChildElement("Instances")->FirstChildElement("Instance");
    XMLElement* eventsTag = instance->FirstChildElement("Events");

    for (XMLElement* event = eventsTag->FirstChildElement("Event"); event; event = event->NextSiblingElement("Event")) {
        const char* id = event->Attribute("Id");
        const char* name = event->FirstChildElement("Name")->GetText();
        int duration = atoi(event->FirstChildElement("Duration")->GetText());
        const char* courseRef = event->FirstChildElement("Course")->Attribute("Reference");

        cout << "Event ID: " << id << ", Name: " << name
             << ", Duration: " << duration << ", Course: " << courseRef << endl;

        // Recursos do evento
        XMLElement* resources = event->FirstChildElement("Resources");
        for (XMLElement* res = resources->FirstChildElement("Resource"); res; res = res->NextSiblingElement("Resource")) {
            const char* resRef = res->Attribute("Reference");
            const char* role = res->FirstChildElement("Role")->GetText();
            const char* resType = res->FirstChildElement("ResourceType")->Attribute("Reference");

            cout << "  Resource: " << resRef << " (" << role << ", Type: " << resType << ")\n";
        }
    }

    return 0;
}

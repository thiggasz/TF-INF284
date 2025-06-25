#include "tinyxml2.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

using namespace tinyxml2;
using namespace std;

unordered_map<string, pair<int, int>> daySlot_index;   // "Mo_1"
unordered_map<string, pair<int, int>> teacherClass_index;  // "T1-S1"

vector<string> index_daySlot; 
vector<string> index_teacherClass;

void load_instance(){
    XMLDocument doc;
    if (doc.LoadFile("instances/instance1.xml") != XML_SUCCESS) {
        cerr << "Erro ao carregar o arquivo XML\n";
        return;
    }

    XMLElement* root = doc.FirstChildElement("HighSchoolTimetableArchive");
    if (!root) 
        return;

    XMLElement* instance = root->FirstChildElement("Instances")->FirstChildElement("Instance");
    XMLElement* times = instance->FirstChildElement("Times");

    int count = 0;
    for (XMLElement* time = times->FirstChildElement("Time"); time; time = time->NextSiblingElement("Time")) {
        string id = time->Attribute("Id");
        XMLElement* timeGroup = time->FirstChildElement("TimeGroups")->FirstChildElement("TimeGroup");

        if(timeGroup && string(timeGroup->Attribute("Reference")) == "gr_TimesDurationTwo") {
            daySlot_index.insert(make_pair(id, make_pair(count, 2)));
            index_daySlot.push_back(id);
        } else {
            daySlot_index.insert(make_pair(id, make_pair(count, 1)));
            index_daySlot.push_back(id);
        }

        count++;
    }

    XMLElement* events = instance->FirstChildElement("Events");

    count = 0;
    for (XMLElement* event = events->FirstChildElement("Event"); event; event = event->NextSiblingElement("Event")) {
        string id = event->Attribute("Id"); //problema com const char?
        int duration = atoi(event->FirstChildElement("Duration")->GetText());

        teacherClass_index.insert(make_pair(id, make_pair(count, duration)));
        index_teacherClass.push_back(id);

        count++;
    }
}

void validade(){
    cout << daySlot_index.size() << " " << teacherClass_index.size() << "\n";

    for (const auto& entry : daySlot_index) {
        cout << "Time ID: " << entry.first << ", Index: " << entry.second.first << ", Duration: " << entry.second.second << endl;
    }

    cout << "\n";

    for(int i=0; i<index_daySlot.size(); i++){
        cout << "Time ID: " << index_daySlot[i] << ", Index: " << i << "\n"; 
    }

    cout << "\n";

    for (const auto& entry : teacherClass_index) {
        cout << "Teacher-Class ID: " << entry.first << ", Index: " << entry.second.first << ", Duration: " << entry.second.second << endl;
    }

    cout << "\n";
    
    for(int i=0; i<index_teacherClass.size(); i++){
        cout << "Teacher-Class ID: " << index_teacherClass[i] << ", Index: " << i << "\n"; 
    }

}

int main() {
    load_instance();
    validade();

    vector<vector<int>> solution(daySlot_index.size());

    return 0;
}

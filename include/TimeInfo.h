#ifndef TIMEINFO
#define TIMEINFO

#include <string>

using namespace std;

class TimeInfo
{
public:
    string id;
    string day;
    int slot;
    int max_duration; // 1 ou 2
};

#endif
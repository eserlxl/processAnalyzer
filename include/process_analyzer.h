#ifndef PROCESS_ANALYZER_H
#define PROCESS_ANALYZER_H

#include <string>
#include <vector>

struct ProcessInfo {
    int pid;
    std::string name;
    std::string state;
    long memory_usage; // in KB
};

class ProcessAnalyzer {
public:
    ProcessAnalyzer();
    std::vector<int> getPids();
    ProcessInfo getProcessDetails(int pid);
    void printAllProcesses();
    
private:
    std::string proc_dir;
};

#endif

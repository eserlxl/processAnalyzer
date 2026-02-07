// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef ANALYZER_H
#define ANALYZER_H

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

// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "cli/output.h"
#include "utils/string.h"
#include "utils/time.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <cassert>
#include <utility>
#include <string_view>

namespace {
    // Helper to get string value of a ProcessInfo field based on column name
    std::string getProcessInfoValue(const ProcessInfo& info, std::string_view col) {
        if (col == "pid") return std::to_string(info.pid);
        if (col == "ppid") return std::to_string(info.ppid);
        if (col == "uid") return std::to_string(info.uid);
        if (col == "user") return info.username;
        if (col == "name") return info.name;
        if (col == "state") return info.state;
        if (col == "rss") return std::to_string(info.residentMemory);
        if (col == "vm") return std::to_string(info.virtualMemory);
        if (col == "threads") return std::to_string(info.threadCount);
        if (col == "cmdline") return info.cmdline;
        // New columns for Iteration 13
        if (col == "start-time") {
            return utils::formatTimestamp(info.startTimeUnix).value_or("N/A");
        }
        if (col == "elapsed-time") return info.elapsedTime;
        if (col == "exec-path") return info.executablePath;
        if (col == "nice") return std::to_string(info.priority);
        if (col == "cwd") return info.currentWorkingDirectory;
        std::unreachable();
    }
}

// Escape a string for embedding in a JSON string literal (RFC 8259 §7):
// the short escapes for the common control characters, \uXXXX for any
// other control byte below 0x20, and \" / \\ for quote and backslash.
std::string jsonEscape(std::string_view value) {
    constexpr unsigned char firstPrintableChar = 0x20U; // chars below this are control chars
    constexpr unsigned int nibbleBits = 4U;
    constexpr unsigned int nibbleMask = 0xFU;
    std::string out;
    out.reserve(value.size());
    for (const char ch : value) {
        const auto byte = static_cast<unsigned char>(ch);
        switch (ch) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (byte < firstPrintableChar) {
                    constexpr std::string_view hexDigits = "0123456789abcdef";
                    out += "\\u00";
                    out += hexDigits[(byte >> nibbleBits) & nibbleMask];
                    out += hexDigits[byte & nibbleMask];
                } else {
                    out += ch;
                }
                break;
        }
    }
    return out;
}


std::vector<std::string> getDefaultColumnsForTable(bool fullDetails) {
    if (fullDetails) {
        return {"pid", "user", "name", "state", "rss", "vm", "threads", "cmdline"};
    }
    return {"pid", "user", "name", "state", "rss"};
}

void printVerticalProcessDetails(const ProcessInfo& info) {
    std::cout << "PID:               " << info.pid << "\n"
              << "PPID:              " << info.ppid << "\n"
              << "UID:               " << info.uid << "\n"
              << "User:              " << info.username << "\n"
              << "Name:              " << info.name << "\n"
              << "State:             " << info.state << "\n"
              << "Nice Value:        " << info.priority << "\n"
              << "RSS Memory:        " << info.residentMemory << " KB\n"
              << "Virtual Memory:    " << info.virtualMemory << " KB\n"
              << "Threads:           " << info.threadCount << "\n"
              << "Start Time:        " << utils::formatTimestamp(info.startTimeUnix).value_or("N/A") << "\n"
              << "Elapsed Time:      " << info.elapsedTime << "\n"
              << "Executable Path:   " << info.executablePath << "\n"
              << "Working Directory: " << info.currentWorkingDirectory << "\n"
              << "CPU User Time:     " << info.cpuUserTimeTicks << " ticks\n"
              << "CPU Kernel Time:   " << info.cpuKernelTimeTicks << " ticks\n"
              << "IO Read:           " << info.ioReadBytes << " B\n"
              << "IO Write:          " << info.ioWriteBytes << " B\n"
              << "Command:           " << info.cmdline << "\n";
}

void printProcessTable(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns, bool noTruncateCmdline) {
    if (processes.empty()) return;

    // Define column widths
    static const std::map<std::string, int> defaultColumnWidths = {
        {"pid", 8},
        {"ppid", 8},
        {"uid", 8},
        {"user", 15},
        {"name", 25},
        {"state", 12},
        {"rss", 10},
        {"vm", 10},
        {"threads", 8},
        {"cmdline", 40},
        {"start-time", 22},
        {"elapsed-time", 14},
        {"exec-path", 30},
        {"nice", 6},
        {"cwd", 30}
    };
    std::map<std::string, int> widths = defaultColumnWidths; // Use a mutable copy if needed to adjust widths dynamically later

    // Print header
    for (const auto& col : columns) {
        std::string header = col;
        // Use std::ranges::transform for modernization
        std::ranges::transform(header, header.begin(), ::toupper);
        if (col == "rss" || col == "vm") header += "(KB)";
        std::cout << std::left << std::setw(widths[col]) << header;
    }
    std::cout << std::endl;

    // Print separator
    for (const auto& col : columns) {
        std::cout << std::string(widths[col], '-') ;
    }
    std::cout << std::endl;

    // Print rows
    for (const auto& info : processes) {
        for (const auto& col : columns) {
            std::string value = getProcessInfoValue(info, col);
            
            int width = widths[col];
            if (col == "cmdline" && noTruncateCmdline) {
                 std::cout << std::left << value;
            } else {
                if (value.length() > static_cast<size_t>(width)) {
                    value = value.substr(0, width - 1) + "~";
                }
                std::cout << std::left << std::setw(width) << value;
            }
        }
        std::cout << std::endl;
    }
}

void printProcessCsv(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns) {
    // Header
    std::cout << utils::join(columns, ",") << std::endl;

    // Rows
    for (const auto& info : processes) {
        std::vector<std::string> values;
        for (const auto& col : columns) {
            std::string value = getProcessInfoValue(info, col);

            // Quote if necessary (RFC 4180: comma, quote, or embedded CR/LF)
            if (value.contains(',') || value.contains('"') || value.contains('\n') || value.contains('\r')) {
                value = std::string("\"") + utils::replaceAll(value, "\"", "\"\"") + "\"";
            }
            values.push_back(value);
        }
        std::cout << utils::join(values, ",") << std::endl;
    }
}

void printProcessJson(const std::vector<ProcessInfo>& processes, const std::vector<std::string>& columns) {
    std::cout << "[\n";
    for (size_t i = 0; i < processes.size(); ++i) {
        const auto& info = processes[i];
        std::cout << "  {\n";
        
        std::vector<std::string> pairs;
        for (const auto& col : columns) {
            std::stringstream ss;
            std::string value = getProcessInfoValue(info, col);

            ss << "    \"" << col << "\": ";
            if (col == "pid" || col == "ppid" || col == "uid" || col == "rss" || col == "vm" || col == "threads") {
                ss << value; // Numerical values as is
            } else {
                // Escape string values (quotes, backslashes, and control chars)
                ss << "\"" << jsonEscape(value) << "\"";
            }
            pairs.push_back(ss.str());
        }
        std::cout << utils::join(pairs, ",\n");
        std::cout << "\n  }";
        if (i < processes.size() - 1) {
            std::cout << ",";
        }
        std::cout << "\n";
    }
    std::cout << "]\n";
}

namespace {
    constexpr int kForestIndentWidth = 2;

    void printForestNode(const std::map<pid_t, std::vector<const ProcessInfo*>>& childrenByPpid,
                         const ProcessInfo& node, int depth,
                         std::set<pid_t>& visited, bool noTruncateCmdline) {
        if (!visited.insert(node.pid).second) {
            return; // already printed — guards against malformed ppid cycles
        }
        std::cout << std::string(static_cast<std::size_t>(depth) * kForestIndentWidth, ' ')
                  << node.pid << ' ' << node.name;
        if (noTruncateCmdline && !node.cmdline.empty()) {
            std::cout << "  " << node.cmdline;
        }
        std::cout << '\n';
        if (auto it = childrenByPpid.find(node.pid); it != childrenByPpid.end()) {
            for (const ProcessInfo* child : it->second) {
                printForestNode(childrenByPpid, *child, depth + 1, visited, noTruncateCmdline);
            }
        }
    }
} // namespace

void printProcessForest(const std::vector<ProcessInfo>& processes, bool noTruncateCmdline) {
    std::set<pid_t> presentPids;
    for (const auto& proc : processes) {
        presentPids.insert(proc.pid);
    }
    // A process whose ppid is also in the displayed set is a child; everything
    // else (parent not shown, or self-referential) roots the forest.
    std::map<pid_t, std::vector<const ProcessInfo*>> childrenByPpid;
    std::vector<const ProcessInfo*> roots;
    for (const auto& proc : processes) {
        if (proc.ppid != proc.pid && presentPids.contains(proc.ppid)) {
            childrenByPpid[proc.ppid].push_back(&proc);
        } else {
            roots.push_back(&proc);
        }
    }
    const auto byPid = [](const ProcessInfo* lhs, const ProcessInfo* rhs) { return lhs->pid < rhs->pid; };
    for (auto& [ppid, kids] : childrenByPpid) {
        std::ranges::sort(kids, byPid);
    }
    std::ranges::sort(roots, byPid);

    std::set<pid_t> visited;
    for (const ProcessInfo* root : roots) {
        printForestNode(childrenByPpid, *root, 0, visited, noTruncateCmdline);
    }
    // Any node not reached from a root (e.g. caught in a ppid cycle) is still
    // printed so the forest never silently drops a process.
    for (const auto& proc : processes) {
        if (!visited.contains(proc.pid)) {
            printForestNode(childrenByPpid, proc, 0, visited, noTruncateCmdline);
        }
    }
}

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

namespace Utils {
    std::string readFile(const std::string& path);
    std::vector<std::string> split(const std::string& s, char delimiter);
    bool isNumeric(const std::string& s);
}

#endif

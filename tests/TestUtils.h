// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <map>

namespace fs = std::filesystem;

class MockProc {
public:
    explicit MockProc(const std::string& basePath = "mock_proc") : root(basePath) {
        fs::create_directory(root);
    }

    ~MockProc() {
        fs::remove_all(root);
    }

    std::string getPath() const {
        return root.string();
    }

    void createProcFile(int pid, const std::string& filename, const std::string& content) {
        fs::path pidPath = root / std::to_string(pid);
        fs::create_directory(pidPath);
        std::ofstream(pidPath / filename) << content;
    }
    
    void createSymlink(int pid, const std::string& linkname, const std::string& target) {
        fs::path pidPath = root / std::to_string(pid);
        fs::create_directory(pidPath);
        fs::path linkPath = pidPath / linkname;
        fs::create_symlink(target, linkPath);
    }
    
    void createPidDir(int pid) {
        fs::create_directory(root / std::to_string(pid));
    }

    void createFile(const std::string& filename, const std::string& content) {
        std::ofstream(root / filename) << content;
    }

private:
    fs::path root;
};

#endif // TEST_UTILS_H

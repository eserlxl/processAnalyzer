// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <map>
#include <utility> // For std::pair

namespace fs = std::filesystem;

class MockProc {
public:
    explicit MockProc(const std::string& basePath = "mock_proc") : root(basePath) {
        fs::create_directory(root);
    }

    ~MockProc() {
        fs::remove_all(root);
    }

    [[nodiscard]] std::string getPath() const {
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


    // --- Iteration 6 Additions ---
    /**
     * @brief Creates a file at a path relative to the mock root. Intermediate directories will be created.
     * @param relativePath The path relative to the mock root.
     * @param content The content of the file.
     */
    void createFileAt(const std::filesystem::path& relativePath, const std::string& content) {
        fs::path fullPath = root / relativePath;
        fs::create_directories(fullPath.parent_path());
        std::ofstream(fullPath) << content;
    }

    /**
     * @brief Creates a directory at a path relative to the mock root. Intermediate directories will be created.
     * @param relativePath The path relative to the mock root.
     */
    void createDirectoryAt(const std::filesystem::path& relativePath) {
        fs::create_directories(root / relativePath);
    }

    /**
     * @brief Creates a symbolic link at a path relative to the mock root.
     * @param relativeLinkPath The path where the symlink will be created, relative to the mock root.
     * @param targetPath The target path of the symlink (can be absolute or relative to the link's parent).
     */
    void createSymlinkAt(const std::filesystem::path& relativeLinkPath, const std::filesystem::path& targetPath) {
        fs::path fullLinkPath = root / relativeLinkPath;
        fs::create_directories(fullLinkPath.parent_path());
        fs::create_symlink(targetPath, fullLinkPath);
    }

    /**
     * @brief Creates the /proc/<pid>/cmdline file with null-separated arguments.
     * @param pid The process ID.
     * @param args The command line arguments.
     */
    void createCmdline(int pid, const std::vector<std::string>& args) {
        fs::path pidPath = root / std::to_string(pid);
        fs::create_directory(pidPath);
        std::ofstream cmdlineFile(pidPath / "cmdline");
        for (size_t i = 0; i < args.size(); ++i) {
            cmdlineFile << args[i];
            if (i < args.size() - 1) {
                cmdlineFile << '\0';
            }
        }
    }

    /**
     * @brief Creates the /proc/<pid>/status file from a map of key-value pairs.
     * @param pid The process ID.
     * @param data A map where keys are status field names and values are their corresponding strings.
     */
    void createStatus(int pid, const std::map<std::string, std::string>& data) {
        fs::path pidPath = root / std::to_string(pid);
        fs::create_directory(pidPath);
        std::ofstream statusFile(pidPath / "status");
        for (const auto& pair : data) {
            statusFile << pair.first << ": " << pair.second << '\n';
        }
    }

    /**
     * @brief Creates the /proc/<pid>/environ file with null-separated environment variables.
     * @param pid The process ID.
     * @param env_vars A map of environment variable names and values.
     */
    void createEnviron(int pid, const std::map<std::string, std::string>& envVars) {
        fs::path pidPath = root / std::to_string(pid);
        fs::create_directory(pidPath); // Ensure pid directory exists
        std::ofstream environFile(pidPath / "environ");
        bool first = true;
        for (const auto& pair : envVars) { // Use range-based for loop for std::map
            if (!first) {
                environFile << '\0'; // Null terminator between variables
            }
            // Structured binding works directly with map elements (pairs) from range-based for loop
            environFile << pair.first << "=" << pair.second;
            first = false;
        }
    }

    /**
     * @brief Creates the /proc/<pid>/fd directory and populates it with symbolic links for file descriptors.
     * @param pid The process ID.
     * @param fds A vector of pairs, where each pair contains the file descriptor number and its target path.
     */
    void createFdDir(int pid, const std::vector<std::pair<int, std::string>>& fds) {
        fs::path fdPath = root / std::to_string(pid) / "fd";
        fs::create_directory(fdPath);
        for (const auto& fdPair : fds) {
            fs::create_symlink(fdPair.second, fdPath / std::to_string(fdPair.first));
        }
    }

    /**
     * @brief Adds a basic mock process, creating its PID directory and minimal status/cmdline files.
     * @param pid The process ID.
     * @param name The name of the process.
     * @param cmdline_args Optional command line arguments for /proc/<pid>/cmdline.
     */
    void addProcess(int pid, const std::string& name, const std::vector<std::string>& cmdlineArgs = {}) {
        createPidDir(pid); // Ensures /proc/<pid> exists
        
        // Create /proc/<pid>/status with at least the Name field
        std::map<std::string, std::string> statusData;
        statusData["Name"] = name;
        createStatus(pid, statusData);

        // Create /proc/<pid>/cmdline if arguments are provided
        if (!cmdlineArgs.empty()) {
            createCmdline(pid, cmdlineArgs);
        }
    }
    // --- End Iteration 6 Additions ---

private:
    fs::path root;
};

#endif // TEST_UTILS_H

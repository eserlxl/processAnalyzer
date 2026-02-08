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
        // Ensure target path is absolute or relative to the link's location
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
     * @param envVars A map of environment variable names and values.
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
        fs::create_directories(fdPath);
        for (const auto& fdPair : fds) {
            fs::create_symlink(fdPair.second, fdPath / std::to_string(fdPair.first));
        }
    }

    /**
     * @brief Creates a symbolic link for a single file descriptor.
     * @param pid The process ID.
     * @param fd The file descriptor number.
     * @param target The target path of the symlink.
     */
    void createProcFdLink(int pid, int fd, const std::string& target) {
        fs::path fdPath = root / std::to_string(pid) / "fd";
        fs::create_directories(fdPath); // Ensure the fd directory exists
        fs::create_symlink(target, fdPath / std::to_string(fd));
    }

    // --- End Iteration 6 Additions ---

    // --- Iteration 8 Additions ---

    // New PID-Specific Symlink Creators
    /**
     * @brief Creates the /proc/<pid>/exe symbolic link.
     * @param pid The process ID.
     * @param targetPath The path the symlink should point to.
     */
    void createExeSymlink(int pid, const fs::path& targetPath);

    /**
     * @brief Creates the /proc/<pid>/cwd symbolic link.
     * @param pid The process ID.
     * @param targetPath The path the symlink should point to.
     */
    void createCwdSymlink(int pid, const fs::path& targetPath);

    /**
     * @brief Creates the /proc/<pid>/root symbolic link.
     * @param pid The process ID.
     * @param targetPath The path the symlink should point to.
     */
    void createRootSymlink(int pid, const fs::path& targetPath);

    // New Creator for /proc/<pid>/comm
    /**
     * @brief Creates the /proc/<pid>/comm file.
     * @param pid The process ID.
     * @param commName The command name string. A newline will be appended automatically.
     */
    void createComm(int pid, const std::string& commName);

    // Structured Content Generation for Complex Files

    struct ProcMapEntry {
        std::string addressRange; // e.g., "7f000000-7f010000"
        std::string perms;         // e.g., "r-xp"
        unsigned long offset;      // e.g., 0x0
        std::string dev;           // e.g., "00:00"
        unsigned long inode;       // e.g., 0
        fs::path pathname;         // e.g., "/usr/lib/libc.so"

        /**
         * @brief Formats the ProcMapEntry struct into a string line for /proc/<pid>/maps.
         * @return String representation of the map entry.
         */
        [[nodiscard]] std::string toString() const;
    };

    /**
     * @brief Creates the /proc/<pid>/maps file with multiple entries.
     * @param pid The process ID.
     * @param entries A vector of ProcMapEntry objects.
     */
    void createMaps(int pid, const std::vector<ProcMapEntry>& entries);

    struct ProcIoStats {
        unsigned long rchar = 0;
        unsigned long wchar = 0;
        unsigned long syscr = 0;
        unsigned long syscw = 0;
        unsigned long readBytes = 0;
        unsigned long writeBytes = 0;
        unsigned long cancelledWriteBytes = 0;

        /**
         * @brief Formats the ProcIoStats struct into a string for /proc/<pid>/io.
         * @return String representation of the I/O statistics.
         */
        [[nodiscard]] std::string toString() const;
    };

    /**
     * @brief Creates the /proc/<pid>/io file.
     * @param pid The process ID.
     * @param stats The ProcIoStats object.
     */
    void createIo(int pid, const ProcIoStats& stats);

    // NOTE: Fields for ProcStatData are based on the /proc/<pid>/stat documentation.
    // This is a subset, and more can be added if needed.
    struct ProcStatData {
        int pid = 0;
        std::string comm; // Filename of the executable in parentheses.
        char state = 'R';       // R, S, D, Z, T, W, X, K, W.
        int ppid = 0;
        int pgrp = 0;
        int session = 0;
        int tty_nr = 0;
        int tpgid = 0;
        unsigned long flags = 0;
        unsigned long minflt = 0;
        unsigned long cminflt = 0;
        unsigned long majflt = 0;
        unsigned long cmajflt = 0;
        unsigned long utime = 0;
        unsigned long stime = 0;
        long cutime = 0;
        long cstime = 0;
        long priority = 0;
        long nice = 0;
        long num_threads = 0;
        long itrealvalue = 0;
        unsigned long starttime = 0;
        unsigned long vsize = 0;
        long rss = 0;
        unsigned long rsslim = 0;
        unsigned long startcode = 0;
        unsigned long endcode = 0;
        unsigned long startstack = 0;
        unsigned long kstkesp = 0;
        unsigned long kstkeip = 0;
        unsigned long signal = 0;
        unsigned long blocked = 0;
        unsigned long sigignore = 0;
        unsigned long sigcatch = 0;
        unsigned long wchan = 0;
        unsigned long nswap = 0;
        unsigned long cnswap = 0;
        int exit_signal = 0;
        int processor = 0;
        unsigned long rt_priority = 0;
        unsigned long policy = 0;
        unsigned long delayacct_blkio_ticks = 0;
        unsigned long guest_time = 0;
        long cguest_time = 0;
        unsigned long start_data = 0;
        unsigned long end_data = 0;
        unsigned long start_brk = 0;
        unsigned long arg_start = 0;
        unsigned long arg_end = 0;
        unsigned long env_start = 0;
        unsigned long env_end = 0;
        int exit_code = 0;

        /**
         * @brief Formats the ProcStatData struct into a string for /proc/<pid>/stat.
         * @return String representation of the stat entry.
         */
        [[nodiscard]] std::string toString() const;
    };

    /**
     * @brief Creates the /proc/<pid>/stat file.
     * @param pid The process ID.
     * @param data The ProcStatData object.
     */
    void createStat(int pid, const ProcStatData& data);

    // System-wide /proc File Creators

    struct MeminfoData {
        unsigned long memTotalKb = 0;
        unsigned long memFreeKb = 0;
        unsigned long memAvailableKb = 0;
        unsigned long buffersKb = 0;
        unsigned long cachedKb = 0;
        unsigned long swapTotalKb = 0;
        unsigned long swapFreeKb = 0;
        // ... (other common fields can be added as needed)

        /**
         * @brief Formats the MeminfoData struct into a string for /proc/meminfo.
         * @return String representation of the memory info.
         */
        [[nodiscard]] std::string toString() const;
    };

    /**
     * @brief Creates the /proc/meminfo file.
     * @param data The MeminfoData object.
     */
    void createMeminfo(const MeminfoData& data);

    struct CpuinfoData {
        int processorId = 0;
        std::string vendorId = "GenuineIntel";
        std::string modelName = "Intel(R) Core(TM) i7-10750H CPU @ 2.60GHz";
        float cpuMhz = 2600.000;
        int siblings = 12;
        int cpuCores = 6;
        // ... (other common fields can be added)

        /**
         * @brief Formats a single CPU's data into a string for /proc/cpuinfo.
         * @return String representation of the CPU info for one core.
         */
        [[nodiscard]] std::string toString() const;
    };

    /**
     * @brief Creates the /proc/cpuinfo file for a multi-core system.
     * @param cores A vector of CpuinfoData objects, one for each CPU core.
     */
    void createCpuinfo(const std::vector<CpuinfoData>& cores);

    // Enhanced addProcess Functionality

    /**
     * @brief Options struct to consolidate all configurable options for a mock process.
     */
    struct AddProcessOptions {
        std::string name;                          // For /proc/<pid>/status 'Name' and /proc/<pid>/comm
        std::vector<std::string> cmdlineArgs;      // For /proc/<pid>/cmdline
        fs::path exePath;                          // Target for /proc/<pid>/exe symlink
        fs::path cwdPath;                          // Target for /proc/<pid>/cwd symlink
        fs::path rootPath;                         // Target for /proc/<pid>/root symlink
        std::map<std::string, std::string> statusExtra; // Additional fields for /proc/<pid>/status
        std::map<std::string, std::string> environVars; // For /proc/<pid>/environ
        std::vector<std::pair<int, std::string>> fds; // For /proc/<pid>/fd directory
        ProcIoStats ioStats;                       // Optional: Data for /proc/<pid>/io
        ProcStatData statData;                     // Optional: Data for /proc/<pid>/stat
        bool populateDefaultStat = false;          // If true, populates sensible defaults for statData if not provided.
    };

    /**
     * @brief Creates a mock process directory and populates various process-related files and symlinks.
     * This is a significant improvement over previous overloads, providing a single point
     * of configuration for a mock process. The old overload addProcess(int pid, const std::string& name, const std::vector<std::string>& cmdlineArgs)
     * is considered deprecated and will be removed in favor of this more comprehensive version.
     * @param pid The process ID.
     * @param options Configuration options for the mock process.
     */
    void addProcess(int pid, const AddProcessOptions& options);
    // --- End Iteration 8 Additions ---

private:
    fs::path root;
};

#endif // TEST_UTILS_H

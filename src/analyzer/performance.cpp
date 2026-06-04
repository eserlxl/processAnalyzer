// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "analyzer/internal/helpers.h"
#include "utils/core.h"
#include "utils/types.h"

#include <sched.h>
#include <sys/resource.h>
#include <algorithm>
#include <cerrno>
#include <sstream>
#include <string>
#include <thread>

namespace {

// Parse a CPU range-list token (e.g., "0-3" or "2") into individual core IDs.
void expandCpuToken(std::string_view token, std::vector<int>& out) {
    const auto dashPos = token.find('-');
    if (dashPos == std::string_view::npos) {
        if (auto v = utils::parseInteger<int>(token)) {
            out.push_back(*v);
        }
        return;
    }
    auto lo = utils::parseInteger<int>(token.substr(0, dashPos));
    auto hi = utils::parseInteger<int>(token.substr(dashPos + 1));
    if (lo && hi) {
        for (int cpu = *lo; cpu <= *hi; ++cpu) {
            out.push_back(cpu);
        }
    }
}

} // namespace

utils::Result<CpuSet> ProcessAnalyzer::getProcessCpuAffinity(int pid) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) {
        return std::unexpected(check.error());
    }

    const auto statusPath = procPath / std::to_string(pid) / "status";
    auto content = utils::readTextFile(statusPath.string());
    if (!content) {
        return std::unexpected(content.error());
    }

    std::istringstream iss{*content};
    std::string line;
    static constexpr std::string_view kPrefix = "Cpus_allowed_list:";
    while (std::getline(iss, line)) {
        if (!line.starts_with(kPrefix)) {
            continue;
        }
        const auto valueStr = utils::trim(line.substr(kPrefix.size()));
        CpuSet result;
        for (const auto& token : utils::split(valueStr, ',')) {
            expandCpuToken(utils::trim(token), result.cpus);
        }
        std::ranges::sort(result.cpus);
        return result;
    }

    return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
}

utils::Result<void> ProcessAnalyzer::setProcessCpuAffinity(int pid, const CpuSet& affinity) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) {
        return std::unexpected(check.error());
    }

    cpu_set_t mask;
    CPU_ZERO(&mask);
    for (int cpu : affinity.cpus) {
        if (cpu >= 0) {
            CPU_SET(static_cast<size_t>(cpu), &mask);
        }
    }

    if (::sched_setaffinity(static_cast<pid_t>(pid), sizeof(cpu_set_t), &mask) == 0) {
        return {};
    }

    switch (errno) {
        case ESRCH:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
        case EPERM:
        case EACCES:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
        case EINVAL:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
        default:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerSystemError));
    }
}

utils::Result<void> ProcessAnalyzer::setProcessPriority(int pid, int niceValue) const {
    auto check = Internal::checkPidPathExistsAndPermissions(procPath, pid);
    if (!check) {
        return std::unexpected(check.error());
    }

    if (::setpriority(PRIO_PROCESS, static_cast<id_t>(pid), niceValue) == 0) {
        return {};
    }

    switch (errno) {
        case ESRCH:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerProcessNotFound));
        case EPERM:
        case EACCES:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerPermissionDenied));
        case EINVAL:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
        default:
            return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerSystemError));
    }
}

// Implementation of ProcessAnalyzer::getProcessCpuUsage
utils::Result<ProcessCpuUsage> ProcessAnalyzer::getProcessCpuUsage(int pid, std::chrono::milliseconds duration) const {
    auto proc1 = getProcessDetails(pid);
    if (!proc1) return std::unexpected(proc1.error());
    auto sys1  = getSystemCpuStats();
    if (!sys1)  return std::unexpected(sys1.error());

    std::this_thread::sleep_for(duration);

    auto proc2 = getProcessDetails(pid);
    if (!proc2) return std::unexpected(proc2.error());
    auto sys2  = getSystemCpuStats();
    if (!sys2)  return std::unexpected(sys2.error());

    const auto procTicks1 = proc1->cpuUserTimeTicks + proc1->cpuKernelTimeTicks;
    const auto procTicks2 = proc2->cpuUserTimeTicks + proc2->cpuKernelTimeTicks;
    const auto sysActive1 = sys1->user + sys1->nice + sys1->system + sys1->irq + sys1->softirq + sys1->steal;
    const auto sysActive2 = sys2->user + sys2->nice + sys2->system + sys2->irq + sys2->softirq + sys2->steal;
    const auto sysTotal1  = sysActive1 + sys1->idle + sys1->iowait;
    const auto sysTotal2  = sysActive2 + sys2->idle + sys2->iowait;
    const auto totalDelta = sysTotal2 - sysTotal1;
    if (totalDelta == 0ULL) {
        return ProcessCpuUsage{.pid = pid, .cpuPercentage = 0.0};
    }
    const auto procDelta = static_cast<double>(procTicks2 - procTicks1);
    return ProcessCpuUsage{.pid = pid, .cpuPercentage = procDelta / static_cast<double>(totalDelta) * 100.0};
}

// Implementation of ProcessAnalyzer::getAllProcessesCpuUsage
utils::Result<std::vector<ProcessCpuUsage>> ProcessAnalyzer::getAllProcessesCpuUsage(std::chrono::milliseconds duration) const {
    auto pids = getPids();
    if (!pids) return std::unexpected(pids.error());
    auto sys1 = getSystemCpuStats();
    if (!sys1) return std::unexpected(sys1.error());

    std::vector<std::pair<int, long long>> snap1;
    snap1.reserve(pids->size());
    for (int pid : *pids) {
        auto proc = getProcessDetails(pid);
        if (proc) snap1.emplace_back(pid, proc->cpuUserTimeTicks + proc->cpuKernelTimeTicks);
    }

    std::this_thread::sleep_for(duration);

    auto sys2 = getSystemCpuStats();
    if (!sys2) return std::unexpected(sys2.error());

    const auto sysActive1 = sys1->user + sys1->nice + sys1->system + sys1->irq + sys1->softirq + sys1->steal;
    const auto sysActive2 = sys2->user + sys2->nice + sys2->system + sys2->irq + sys2->softirq + sys2->steal;
    const auto totalDelta = (sysActive2 + sys2->idle + sys2->iowait) - (sysActive1 + sys1->idle + sys1->iowait);

    std::vector<ProcessCpuUsage> result;
    for (const auto& [pid, ticks1] : snap1) {
        auto proc2 = getProcessDetails(pid);
        if (!proc2) continue;
        const auto ticks2 = proc2->cpuUserTimeTicks + proc2->cpuKernelTimeTicks;
        double pct = 0.0;
        if (totalDelta > 0ULL) {
            pct = static_cast<double>(ticks2 - ticks1) / static_cast<double>(totalDelta) * 100.0;
        }
        result.push_back(ProcessCpuUsage{.pid = pid, .cpuPercentage = pct});
    }
    return result;
}

// Implementation of ProcessAnalyzer::getProcessDiskIoUsage
utils::Result<ProcessDiskIoUsage> ProcessAnalyzer::getProcessDiskIoUsage(int pid, std::chrono::milliseconds duration) const {
    auto proc1 = getProcessDetails(pid);
    if (!proc1) return std::unexpected(proc1.error());

    std::this_thread::sleep_for(duration);

    auto proc2 = getProcessDetails(pid);
    if (!proc2) return std::unexpected(proc2.error());

    const double durationSec = static_cast<double>(std::max(duration.count(), decltype(duration.count()){1})) / 1000.0;
    const auto readDelta  = proc2->ioReadBytes  - proc1->ioReadBytes;
    const auto writeDelta = proc2->ioWriteBytes - proc1->ioWriteBytes;
    return ProcessDiskIoUsage{
        .pid              = pid,
        .readBytesPerSec  = static_cast<long long>(static_cast<double>(readDelta)  / durationSec),
        .writeBytesPerSec = static_cast<long long>(static_cast<double>(writeDelta) / durationSec),
    };
}

// Implementation of ProcessAnalyzer::getAllProcessesDiskIoUsage
utils::Result<std::vector<ProcessDiskIoUsage>> ProcessAnalyzer::getAllProcessesDiskIoUsage(std::chrono::milliseconds duration) const {
    auto pids = getPids();
    if (!pids) return std::unexpected(pids.error());

    std::vector<std::pair<int, std::pair<long long, long long>>> snap1;
    snap1.reserve(pids->size());
    for (int pid : *pids) {
        auto proc = getProcessDetails(pid);
        if (proc) snap1.emplace_back(pid, std::make_pair(proc->ioReadBytes, proc->ioWriteBytes));
    }

    std::this_thread::sleep_for(duration);

    const double durationSec = static_cast<double>(std::max(duration.count(), decltype(duration.count()){1})) / 1000.0;
    std::vector<ProcessDiskIoUsage> result;
    for (const auto& [pid, io1] : snap1) {
        auto proc2 = getProcessDetails(pid);
        if (!proc2) continue;
        const auto readDelta  = proc2->ioReadBytes  - io1.first;
        const auto writeDelta = proc2->ioWriteBytes - io1.second;
        result.push_back(ProcessDiskIoUsage{
            .pid              = pid,
            .readBytesPerSec  = static_cast<long long>(static_cast<double>(readDelta)  / durationSec),
            .writeBytesPerSec = static_cast<long long>(static_cast<double>(writeDelta) / durationSec),
        });
    }
    return result;
}

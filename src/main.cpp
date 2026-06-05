// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include <iostream>
#include <sstream>
#include <span>
#include <iomanip>
#include <filesystem>
#include <thread>
#include <algorithm>

#include "analyzer/core.h"
#include "cli/args.h"
#include "cli/output.h"
#include "utils/time.h"
#include "utils/string.h"

int main(int argc, char* argv[]) {
    try {
        auto argsOpt = parseCommandLine(argc, std::span(argv, argc));
        if (!argsOpt) {
            return 1;
        }
        ParsedArguments args = *argsOpt;

        if (args.showHelp) {
            printUsage();
            return 0;
        }

        ProcessAnalyzer analyzer(std::filesystem::path("/proc"));
        std::vector<ProcessInfo> processesToDisplay;
        ProcessFilter filter;
        
        // Populate filter from args
        if (args.name) filter.nameContains = *args.name;
        if (args.user) filter.userFilter = *args.user;
        if (args.ppidFilter) filter.ppidFilter = args.ppidFilter;
        filter.stateFilter = args.stateFilter;
        filter.minResidentMemoryKB = args.minRssKb;
        filter.maxResidentMemoryKB = args.maxRssKb;
        filter.minThreads = args.minThreads;
        filter.maxThreads = args.maxThreads;
        filter.uidFilter = args.uidFilter;
        if (args.cmdlineFilter) filter.cmdlineContains = *args.cmdlineFilter;
        filter.minVirtualMemoryKB = args.minVmKb;
        filter.maxVirtualMemoryKB = args.maxVmKb;
        filter.minPriority = args.minPriority;
        filter.maxPriority = args.maxPriority;
        if (args.networkPortFilter) {
            ProcessFilter::NetworkFilterCriteria netCrit;
            netCrit.localPort = args.networkPortFilter;
            filter.networkConnectionFilter = netCrit;
        }

        if (args.command == "system") {
            constexpr int labelWidth = 20;
            constexpr int kCpuSampleMs = 200;

            // Machine-readable JSON output for automation/diagnostics tooling.
            if (args.outputFormat == "json") {
                const auto sampleDuration = std::chrono::milliseconds(kCpuSampleMs);
                std::vector<std::string> members;
                auto quote = [](std::string_view s) { return "\"" + jsonEscape(s) + "\""; };

                if (auto r = analyzer.getSystemInfo()) {
                    std::ostringstream s;
                    s << R"("system_info": {"hostname": )" << quote(r->hostname)
                      << ", \"os_name\": " << quote(r->osName)
                      << ", \"kernel_version\": " << quote(r->kernelVersion)
                      << ", \"uptime_seconds\": "
                      << std::chrono::duration_cast<std::chrono::seconds>(r->uptime).count() << "}";
                    members.push_back(s.str());
                }
                if (auto r = analyzer.getSystemLoadAverage()) {
                    std::ostringstream s;
                    s << R"("load_average": {"one": )" << r->oneMin << ", \"five\": " << r->fiveMin
                      << ", \"fifteen\": " << r->fifteenMin << "}";
                    members.push_back(s.str());
                }
                if (auto r = analyzer.getSystemCpuUsage(sampleDuration)) {
                    std::ostringstream s;
                    s << "\"cpu_usage_percent\": " << r->cpuPercentage;
                    members.push_back(s.str());
                }
                if (auto r = analyzer.getPerCpuUsage(sampleDuration)) {
                    std::ostringstream s;
                    s << "\"per_cpu\": [";
                    for (std::size_t i = 0; i < r->cpuUsages.size(); ++i) {
                        const auto& c = r->cpuUsages[i];
                        if (i != 0) { s << ", "; }
                        s << "{\"cpu_id\": " << c.cpuId << ", \"usage_percent\": " << c.cpuPercentage << "}";
                    }
                    s << "]";
                    members.push_back(s.str());
                }
                if (auto r = analyzer.getSystemMemoryInfo()) {
                    std::ostringstream s;
                    s << R"("memory_kb": {"total": )" << r->memTotal << ", \"free\": " << r->memFree
                      << ", \"available\": " << r->memAvailable << ", \"buffers\": " << r->buffers
                      << ", \"cached\": " << r->cached << ", \"swap_total\": " << r->swapTotal
                      << ", \"swap_free\": " << r->swapFree << "}";
                    members.push_back(s.str());
                }
                if (auto r = analyzer.getSystemDiskUsage()) {
                    std::ostringstream s;
                    s << "\"disk_usage\": [";
                    for (std::size_t i = 0; i < r->size(); ++i) {
                        const auto& m = (*r)[i];
                        if (i != 0) { s << ", "; }
                        s << "{\"device\": " << quote(m.device) << ", \"mount_point\": " << quote(m.mountPoint)
                          << ", \"filesystem_type\": " << quote(m.filesystemType)
                          << ", \"total_bytes\": " << m.totalSpaceBytes
                          << ", \"free_bytes\": " << m.freeSpaceBytes
                          << ", \"available_bytes\": " << m.availableSpaceBytes << "}";
                    }
                    s << "]";
                    members.push_back(s.str());
                }
                if (auto r = analyzer.getNetworkInterfaceStats()) {
                    std::ostringstream s;
                    s << "\"network_interfaces\": [";
                    for (std::size_t i = 0; i < r->size(); ++i) {
                        const auto& n = (*r)[i];
                        if (i != 0) { s << ", "; }
                        s << "{\"interface\": " << quote(n.interfaceName) << ", \"rx_bytes\": " << n.rxBytes
                          << ", \"tx_bytes\": " << n.txBytes << ", \"rx_packets\": " << n.rxPackets
                          << ", \"tx_packets\": " << n.txPackets << "}";
                    }
                    s << "]";
                    members.push_back(s.str());
                }
                if (auto r = analyzer.getNetworkInterfaceRates(sampleDuration)) {
                    std::ostringstream s;
                    s << "\"network_interface_rates\": [";
                    for (std::size_t i = 0; i < r->size(); ++i) {
                        const auto& n = (*r)[i];
                        if (i != 0) { s << ", "; }
                        s << "{\"interface\": " << quote(n.interfaceName)
                          << ", \"rx_bytes_per_sec\": " << n.rxBytesPerSec
                          << ", \"tx_bytes_per_sec\": " << n.txBytesPerSec
                          << ", \"rx_packets_per_sec\": " << n.rxPacketsPerSec
                          << ", \"tx_packets_per_sec\": " << n.txPacketsPerSec << "}";
                    }
                    s << "]";
                    members.push_back(s.str());
                }
                if (auto r = analyzer.getSystemDiskIoStats()) {
                    std::ostringstream s;
                    s << "\"disk_io_stats\": [";
                    for (std::size_t i = 0; i < r->size(); ++i) {
                        const auto& d = (*r)[i];
                        if (i != 0) { s << ", "; }
                        s << "{\"device\": " << quote(d.deviceName) << ", \"reads_completed\": " << d.readsCompleted
                          << ", \"writes_completed\": " << d.writesCompleted << ", \"sectors_read\": " << d.sectorsRead
                          << ", \"sectors_written\": " << d.sectorsWritten << "}";
                    }
                    s << "]";
                    members.push_back(s.str());
                }
                if (auto r = analyzer.getSystemDiskIoRates(sampleDuration)) {
                    std::ostringstream s;
                    s << "\"disk_io_rates\": [";
                    for (std::size_t i = 0; i < r->size(); ++i) {
                        const auto& d = (*r)[i];
                        if (i != 0) { s << ", "; }
                        s << "{\"device\": " << quote(d.deviceName) << ", \"reads_per_sec\": " << d.readsPerSec
                          << ", \"writes_per_sec\": " << d.writesPerSec
                          << ", \"sectors_read_per_sec\": " << d.sectorsReadPerSec
                          << ", \"sectors_written_per_sec\": " << d.sectorsWrittenPerSec << "}";
                    }
                    s << "]";
                    members.push_back(s.str());
                }
                if (auto r = analyzer.getSystemActivityStats()) {
                    std::ostringstream s;
                    s << R"("system_activity": {"context_switches": )" << r->contextSwitches
                      << ", \"interrupts\": " << r->interruptsTotal << ", \"forks\": " << r->processesForked << "}";
                    members.push_back(s.str());
                }
                if (auto r = analyzer.getSystemActivityRates(sampleDuration)) {
                    std::ostringstream s;
                    s << R"("system_activity_rates": {"context_switches_per_sec": )" << r->contextSwitchesPerSec
                      << ", \"interrupts_per_sec\": " << r->interruptsPerSec
                      << ", \"forks_per_sec\": " << r->processForkRate << "}";
                    members.push_back(s.str());
                }

                std::cout << "{\n  " << utils::join(members, ",\n  ") << "\n}\n";
                return 0;
            }

            // --watch repeats the human-readable report until interrupted (Ctrl-C);
            // without it the body runs exactly once and breaks at the bottom.
            const bool watchMode = args.watchIntervalSeconds.has_value();
            while (true) {
            if (watchMode) {
                std::cout << "\033[2J\033[H";  // clear screen, move cursor home
            }
            // System info
            auto infoResult = analyzer.getSystemInfo();
            if (infoResult) {
                const auto& info = *infoResult;
                std::cout << "=== System Information ===\n";
                std::cout << std::left << std::setw(labelWidth) << "Hostname:" << info.hostname << "\n";
                std::cout << std::left << std::setw(labelWidth) << "OS:" << info.osName << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Kernel:" << info.kernelVersion << "\n";
                long long uptimeSecs = std::chrono::duration_cast<std::chrono::seconds>(info.uptime).count();
                std::string uptimeStr = utils::formatElapsedTime(uptimeSecs).value_or("N/A");
                std::cout << std::left << std::setw(labelWidth) << "Uptime:" << uptimeStr << "\n";
            }
            // Load average
            auto loadResult = analyzer.getSystemLoadAverage();
            if (loadResult) {
                const auto& avg = *loadResult;
                std::cout << "\n=== Load Average ===\n";
                std::cout << std::left << std::setw(labelWidth) << "1 min:" << avg.oneMin << "\n";
                std::cout << std::left << std::setw(labelWidth) << "5 min:" << avg.fiveMin << "\n";
                std::cout << std::left << std::setw(labelWidth) << "15 min:" << avg.fifteenMin << "\n";
            }
            // CPU usage (sampled over 200 ms)
            auto cpuUsageResult = analyzer.getSystemCpuUsage(std::chrono::milliseconds(kCpuSampleMs));
            auto perCpuResult   = analyzer.getPerCpuUsage(std::chrono::milliseconds(kCpuSampleMs));
            if (cpuUsageResult || perCpuResult) {
                std::cout << "\n=== CPU Usage (200 ms sample) ===\n";
                if (cpuUsageResult) {
                    std::cout << std::left << std::setw(labelWidth) << "Total:"
                              << std::fixed << std::setprecision(1)
                              << cpuUsageResult->cpuPercentage << "%\n";
                }
                if (perCpuResult) {
                    for (const auto& core : perCpuResult->cpuUsages) {
                        std::string label = "Core " + std::to_string(core.cpuId) + ":";
                        std::cout << std::left << std::setw(labelWidth) << label
                                  << std::fixed << std::setprecision(1)
                                  << core.cpuPercentage << "%\n";
                    }
                }
            }
            // Memory
            auto memResult = analyzer.getSystemMemoryInfo();
            if (memResult) {
                const auto& mem = *memResult;
                constexpr unsigned long kbPerMib = 1024;
                std::cout << "\n=== Memory (MiB) ===\n";
                std::cout << std::left << std::setw(labelWidth) << "Total:" << mem.memTotal / kbPerMib << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Free:" << mem.memFree / kbPerMib << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Available:" << mem.memAvailable / kbPerMib << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Buffers:" << mem.buffers / kbPerMib << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Cached:" << mem.cached / kbPerMib << "\n";
                if (mem.swapTotal > 0) {
                    std::cout << std::left << std::setw(labelWidth) << "Swap Total:" << mem.swapTotal / kbPerMib << "\n";
                    std::cout << std::left << std::setw(labelWidth) << "Swap Free:" << mem.swapFree / kbPerMib << "\n";
                }
            }
            // Disk usage
            auto diskResult = analyzer.getSystemDiskUsage();
            if (diskResult && !diskResult->empty()) {
                constexpr unsigned long long bytesPerGib = 1024ULL * 1024 * 1024;
                std::cout << "\n=== Disk Usage ===\n";
                constexpr int devWidth = 20;
                constexpr int mpWidth = 24;
                constexpr int numWidth = 10;
                std::cout << std::left << std::setw(devWidth) << "Device"
                          << std::setw(mpWidth) << "Mount Point"
                          << std::right << std::setw(numWidth) << "Total(GiB)"
                          << std::setw(numWidth) << "Free(GiB)" << "\n";
                std::cout << std::string(devWidth + mpWidth + (numWidth * 2), '-') << "\n";
                for (const auto& mp : *diskResult) {
                    std::cout << std::left << std::setw(devWidth) << mp.device
                              << std::setw(mpWidth) << mp.mountPoint
                              << std::right << std::setw(numWidth)
                              << std::fixed << std::setprecision(1)
                              << static_cast<double>(mp.totalSpaceBytes) / static_cast<double>(bytesPerGib)
                              << std::setw(numWidth)
                              << static_cast<double>(mp.freeSpaceBytes) / static_cast<double>(bytesPerGib)
                              << "\n";
                }
            }
            // Network interface stats
            auto netIfResult = analyzer.getNetworkInterfaceStats();
            if (netIfResult && !netIfResult->empty()) {
                constexpr int ifNameWidth = 12;
                constexpr int numWidth = 14;
                std::cout << "\n=== Network Interfaces ===\n";
                std::cout << std::left << std::setw(ifNameWidth) << "Interface"
                          << std::right << std::setw(numWidth) << "RX Bytes"
                          << std::setw(numWidth) << "TX Bytes"
                          << std::setw(numWidth) << "RX Packets"
                          << std::setw(numWidth) << "TX Packets"
                          << "\n";
                std::cout << std::string(ifNameWidth + (numWidth * 4), '-') << "\n";
                for (const auto& iface : *netIfResult) {
                    std::cout << std::left << std::setw(ifNameWidth) << iface.interfaceName
                              << std::right << std::setw(numWidth) << iface.rxBytes
                              << std::setw(numWidth) << iface.txBytes
                              << std::setw(numWidth) << iface.rxPackets
                              << std::setw(numWidth) << iface.txPackets
                              << "\n";
                }
            }
            // Network interface rates
            auto netRatesResult = analyzer.getNetworkInterfaceRates(std::chrono::milliseconds(kCpuSampleMs));
            if (netRatesResult && !netRatesResult->empty()) {
                constexpr int ifNameWidth = 12;
                constexpr int numWidth = 16;
                std::cout << "\n=== Network Interface Rates (" << kCpuSampleMs << " ms sample) ===\n";
                std::cout << std::left << std::setw(ifNameWidth) << "Interface"
                          << std::right << std::setw(numWidth) << "RX bytes/s"
                          << std::setw(numWidth) << "TX bytes/s"
                          << "\n";
                std::cout << std::string(ifNameWidth + (numWidth * 2), '-') << "\n";
                for (const auto& iface : *netRatesResult) {
                    std::cout << std::left << std::setw(ifNameWidth) << iface.interfaceName
                              << std::right << std::fixed << std::setprecision(0)
                              << std::setw(numWidth) << iface.rxBytesPerSec
                              << std::setw(numWidth) << iface.txBytesPerSec
                              << "\n";
                }
            }
            // Disk I/O stats
            auto diskIoResult = analyzer.getSystemDiskIoStats();
            if (diskIoResult && !diskIoResult->empty()) {
                constexpr int devWidth = 14;
                constexpr int numWidth = 14;
                std::cout << "\n=== Disk I/O Stats ===\n";
                std::cout << std::left << std::setw(devWidth) << "Device"
                          << std::right << std::setw(numWidth) << "Reads"
                          << std::setw(numWidth) << "Writes"
                          << std::setw(numWidth) << "Rd Sectors"
                          << std::setw(numWidth) << "Wr Sectors"
                          << "\n";
                std::cout << std::string(devWidth + (numWidth * 4), '-') << "\n";
                for (const auto& dev : *diskIoResult) {
                    std::cout << std::left << std::setw(devWidth) << dev.deviceName
                              << std::right << std::setw(numWidth) << dev.readsCompleted
                              << std::setw(numWidth) << dev.writesCompleted
                              << std::setw(numWidth) << dev.sectorsRead
                              << std::setw(numWidth) << dev.sectorsWritten
                              << "\n";
                }
            }
            // Disk I/O rates
            auto diskRatesResult = analyzer.getSystemDiskIoRates(std::chrono::milliseconds(kCpuSampleMs));
            if (diskRatesResult && !diskRatesResult->empty()) {
                constexpr int devWidth = 14;
                constexpr int numWidth = 14;
                std::cout << "\n=== Disk I/O Rates (" << kCpuSampleMs << " ms sample) ===\n";
                std::cout << std::left << std::setw(devWidth) << "Device"
                          << std::right << std::setw(numWidth) << "Reads/s"
                          << std::setw(numWidth) << "Writes/s"
                          << "\n";
                std::cout << std::string(devWidth + (numWidth * 2), '-') << "\n";
                for (const auto& dev : *diskRatesResult) {
                    std::cout << std::left << std::setw(devWidth) << dev.deviceName
                              << std::right << std::fixed << std::setprecision(1)
                              << std::setw(numWidth) << dev.readsPerSec
                              << std::setw(numWidth) << dev.writesPerSec
                              << "\n";
                }
            }
            // System activity stats
            auto activityResult = analyzer.getSystemActivityStats();
            if (activityResult) {
                const auto& act = *activityResult;
                std::cout << "\n=== System Activity ===\n";
                std::cout << std::left << std::setw(labelWidth) << "Context Switches:" << act.contextSwitches << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Interrupts:" << act.interruptsTotal << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Forks:" << act.processesForked << "\n";
            }
            // System activity rates (live, sampled over kCpuSampleMs)
            auto activityRatesResult = analyzer.getSystemActivityRates(std::chrono::milliseconds(kCpuSampleMs));
            if (activityRatesResult) {
                const auto& rates = *activityRatesResult;
                std::cout << "\n=== System Activity Rates (" << kCpuSampleMs << " ms sample) ===\n";
                std::cout << std::left << std::setw(labelWidth) << "Context Switches/s:"
                          << std::fixed << std::setprecision(1) << rates.contextSwitchesPerSec << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Interrupts/s:"
                          << std::fixed << std::setprecision(1) << rates.interruptsPerSec << "\n";
                std::cout << std::left << std::setw(labelWidth) << "Forks/s:"
                          << std::fixed << std::setprecision(1) << rates.processForkRate << "\n";
            }
            if (!watchMode) {
                break;
            }
            std::cout.flush();
            std::this_thread::sleep_for(std::chrono::seconds(*args.watchIntervalSeconds));
            }
            return 0;
        }
        if (args.command == "top") {
            constexpr int kTopSampleMs = 500;   // longer window than --perf for steadier rates
            constexpr int kDefaultTopCount = 15;
            constexpr int pidWidth = 8;
            constexpr int cpuWidth = 8;
            constexpr int ioWidth = 14;
            const auto sample = std::chrono::milliseconds(kTopSampleMs);
            const auto topCount = static_cast<std::size_t>(args.topCount.value_or(kDefaultTopCount));
            const bool asJson = (args.outputFormat == "json");

            auto nameOf = [&analyzer](int pid) -> std::string {
                if (auto details = analyzer.getProcessDetails(pid)) {
                    return details->name;
                }
                return "?";
            };

            if (args.topByMem) {
                auto snapResult = analyzer.snapshot();
                if (!snapResult) {
                    std::cerr << "Error reading process snapshot: " << snapResult.error().message() << "\n";
                    return 1;
                }
                auto procs = *snapResult;
                std::ranges::sort(procs, [](const ProcessInfo& a, const ProcessInfo& b) {
                    return a.residentMemory > b.residentMemory;
                });
                const std::size_t count = std::min(topCount, procs.size());
                if (asJson) {
                    std::ostringstream out;
                    out << "[";
                    for (std::size_t i = 0; i < count; ++i) {
                        const auto& p = procs[i];
                        if (i != 0) { out << ", "; }
                        out << R"({"pid": )" << p.pid
                            << R"(, "resident_kb": )" << p.residentMemory
                            << R"(, "name": ")" << jsonEscape(p.name) << "\"}";
                    }
                    out << "]";
                    std::cout << out.str() << "\n";
                    return 0;
                }
                std::cout << "=== Top processes by resident memory ===\n";
                std::cout << std::left << std::setw(pidWidth) << "PID"
                          << std::right << std::setw(ioWidth) << "RSS (KB)"
                          << "  " << std::left << "NAME" << "\n";
                for (std::size_t i = 0; i < count; ++i) {
                    const auto& p = procs[i];
                    std::cout << std::left << std::setw(pidWidth) << p.pid
                              << std::right << std::setw(ioWidth) << p.residentMemory
                              << "  " << std::left << p.name << "\n";
                }
                return 0;
            }

            if (args.topByIo) {
                auto ioResult = analyzer.getAllProcessesDiskIoUsage(sample);
                if (!ioResult) {
                    std::cerr << "Error sampling process disk I/O: " << ioResult.error().message() << "\n";
                    return 1;
                }
                auto usages = *ioResult;
                std::ranges::sort(usages, [](const ProcessDiskIoUsage& a, const ProcessDiskIoUsage& b) {
                    return (a.readBytesPerSec + a.writeBytesPerSec) > (b.readBytesPerSec + b.writeBytesPerSec);
                });
                const std::size_t count = std::min(topCount, usages.size());
                if (asJson) {
                    std::ostringstream out;
                    out << "[";
                    for (std::size_t i = 0; i < count; ++i) {
                        const auto& u = usages[i];
                        if (i != 0) { out << ", "; }
                        out << R"({"pid": )" << u.pid
                            << R"(, "read_bytes_per_sec": )" << u.readBytesPerSec
                            << R"(, "write_bytes_per_sec": )" << u.writeBytesPerSec
                            << R"(, "name": ")" << jsonEscape(nameOf(u.pid)) << "\"}";
                    }
                    out << "]";
                    std::cout << out.str() << "\n";
                    return 0;
                }
                std::cout << "=== Top processes by disk I/O (" << kTopSampleMs << " ms sample) ===\n";
                std::cout << std::left << std::setw(pidWidth) << "PID"
                          << std::right << std::setw(ioWidth) << "Read B/s"
                          << std::setw(ioWidth) << "Write B/s"
                          << "  " << std::left << "NAME" << "\n";
                for (std::size_t i = 0; i < count; ++i) {
                    const auto& u = usages[i];
                    std::cout << std::left << std::setw(pidWidth) << u.pid
                              << std::right << std::setw(ioWidth) << u.readBytesPerSec
                              << std::setw(ioWidth) << u.writeBytesPerSec
                              << "  " << std::left << nameOf(u.pid) << "\n";
                }
                return 0;
            }

            auto usageResult = analyzer.getAllProcessesCpuUsage(sample);
            if (!usageResult) {
                std::cerr << "Error sampling process CPU usage: " << usageResult.error().message() << "\n";
                return 1;
            }
            auto usages = *usageResult;
            std::ranges::sort(usages, [](const ProcessCpuUsage& a, const ProcessCpuUsage& b) {
                return a.cpuPercentage > b.cpuPercentage;
            });
            const std::size_t count = std::min(topCount, usages.size());
            if (asJson) {
                std::ostringstream out;
                out << "[";
                for (std::size_t i = 0; i < count; ++i) {
                    const auto& u = usages[i];
                    if (i != 0) { out << ", "; }
                    out << R"({"pid": )" << u.pid
                        << R"(, "cpu_percent": )" << u.cpuPercentage
                        << R"(, "name": ")" << jsonEscape(nameOf(u.pid)) << "\"}";
                }
                out << "]";
                std::cout << out.str() << "\n";
                return 0;
            }
            std::cout << "=== Top processes by CPU (" << kTopSampleMs << " ms sample) ===\n";
            std::cout << std::left << std::setw(pidWidth) << "PID"
                      << std::right << std::setw(cpuWidth) << "CPU%"
                      << "  " << std::left << "NAME" << "\n";
            for (std::size_t i = 0; i < count; ++i) {
                const auto& u = usages[i];
                std::cout << std::left << std::setw(pidWidth) << u.pid
                          << std::right << std::setw(cpuWidth) << std::fixed << std::setprecision(1)
                          << u.cpuPercentage
                          << "  " << std::left << nameOf(u.pid) << "\n";
            }
            return 0;
        }
        if (args.command == "list" || args.command == "name" || args.command == "user") {
            auto processesResult = analyzer.queryProcesses(filter, args.sortBy.value_or(ProcessSortField::pid), args.sortOrder);
            if (processesResult) {
                processesToDisplay = *processesResult;
            } else {
                std::cerr << "Error listing processes: " << processesResult.error().message() << "\n";
                return 1;
            }
        } else if (args.command == "pid" || args.command == "show") {
            if (!args.pid.has_value()) {
                std::cerr << "Internal error: PID expected.\n";
                return 1;
            }
            int targetPid = *args.pid;

            auto infoResult = analyzer.getProcessDetails(targetPid);
            if (!infoResult) {
                std::cerr << "Error: " << infoResult.error().message() << "\n";
                return 1;
            }
            
            // If --columns is not used, print vertical details and exit.
            if (args.selectedColumns.empty() && !args.outputFormat) {
                printVerticalProcessDetails(*infoResult);
                if (args.showChildren) {
                    auto childrenResult = analyzer.getChildProcesses(targetPid);
                    if (childrenResult) {
                        auto& children = *childrenResult;
                        if (!children.empty()) {
                            std::cout << "\nChildren:\n";
                            printProcessTable(children, getDefaultColumnsForTable(false), args.noTruncateCmdline);
                        } else {
                            std::cout << "\nNo children found.\n";
                        }
                    } else {
                        std::cerr << "\nError getting children: " << childrenResult.error().message() << "\n";
                    }
                }
                if (args.showOpenFiles) {
                    try {
                        auto fdsResult = analyzer.getProcessOpenFileDetails(targetPid);
                        if (fdsResult) {
                            auto& fds = *fdsResult;
                            if (!fds.empty()) {
                                std::cout << "\nOpen Files:\n";
                                for (const auto& fdInfo : fds) {
                                    std::cout << "  fd " << std::setw(3) << fdInfo.fd << ": " << fdInfo.path << "\n";
                                }
                            } else {
                                std::cout << "\nNo open files found.\n";
                            }
                        } else {
                            std::cerr << "\nError reading open files: " << fdsResult.error().message() << "\n";
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error reading open files: " << e.what() << "\n";
                    }
                }
                if (args.showNetworkConnections) {
                    try {
                        auto connsResult = analyzer.getNetworkConnections(targetPid);
                        if (connsResult) {
                            auto& conns = *connsResult;
                            if (!conns.empty()) {
                                std::cout << "\nNetwork Connections:\n";
                                // Header
                                constexpr int colWidthProto = 8;
                                constexpr int colWidthAddress = 28;
                                constexpr int separatorWidth = 78;
                                std::cout << "  " << std::left << std::setw(colWidthProto) << "Proto" << std::setw(colWidthAddress) << "Local Address" << std::setw(colWidthAddress) << "Remote Address" << "State\n";
                                std::cout << "  " << std::string(separatorWidth, '-') << "\n";
                                for (const auto& conn : conns) {
                                    std::cout << "  " << std::left 
                                              << std::setw(colWidthProto) << conn.protocol
                                              << std::setw(colWidthAddress) << conn.localAddress
                                              << std::setw(colWidthAddress) << conn.remoteAddress
                                              << conn.state << "\n";
                                }
                            } else {
                                std::cout << "\nNo network connections found.\n";
                            }
                        } else {
                            std::cerr << "\nError reading network connections: " << connsResult.error().message() << "\n";
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "Error reading network connections: " << e.what() << "\n";
                    }
                }
                if (args.showThreads) {
                    auto threadsResult = analyzer.getProcessThreads(targetPid);
                    if (threadsResult) {
                         auto& threads = *threadsResult;
                         if (!threads.empty()) {
                             std::cout << "\nThreads:\n";
                             // Header and Loop
                             constexpr int tidColumnWidth = 8;
                             constexpr int threadSeparatorWidth = 30;
                             std::cout << "  " << std::left << std::setw(tidColumnWidth) << "TID" << "Name\n";
                             std::cout << "  " << std::string(threadSeparatorWidth, '-') << "\n";
                             for(const auto& thread : threads) {
                                 std::cout << "  " << std::left << std::setw(tidColumnWidth) << thread.tid << thread.name << "\n";
                             }
                         } else {
                             std::cout << "\nNo threads found.\n";
                         }
                    } else {
                        std::cerr << "\nError reading threads: " << threadsResult.error().message() << "\n";
                    }
                }
                if (args.showEnv) {
                    auto envResult = analyzer.getProcessEnvironment(targetPid);
                    if (envResult) {
                        auto& env = *envResult;
                        if (!env.empty()) {
                            std::cout << "\nEnvironment Variables:\n";
                            for (const auto& entry : env) {
                                std::cout << "  " << entry << "\n";
                            }
                        } else {
                            std::cout << "\nNo environment variables found.\n";
                        }
                    } else {
                        std::cerr << "\nError reading environment: " << envResult.error().message() << "\n";
                    }
                }
                if (args.showMemoryMaps) {
                    auto mapsResult = analyzer.getProcessMemoryMaps(targetPid);
                    if (mapsResult) {
                        auto& maps = *mapsResult;
                        if (!maps.empty()) {
                            constexpr int addrWidth = 20;
                            constexpr int permWidth = 6;
                            constexpr int separatorWidth = 60;
                            std::cout << "\nMemory Maps:\n";
                            std::cout << "  " << std::left << std::setw(addrWidth) << "Address Range"
                                      << std::setw(permWidth) << "Perms" << "Pathname\n";
                            std::cout << "  " << std::string(separatorWidth, '-') << "\n";
                            for (const auto& map : maps) {
                                std::ostringstream range;
                                range << std::hex << map.startAddress << "-" << map.endAddress;
                                std::cout << "  " << std::left << std::setw(addrWidth) << range.str()
                                          << std::setw(permWidth) << map.permissions
                                          << map.pathname << "\n";
                            }
                        } else {
                            std::cout << "\nNo memory maps found.\n";
                        }
                    } else {
                        std::cerr << "\nError reading memory maps: " << mapsResult.error().message() << "\n";
                    }
                }
                if (args.showLimits) {
                    auto limitsResult = analyzer.getProcessResourceLimits(targetPid);
                    if (limitsResult) {
                        auto& info = *limitsResult;
                        if (!info.limits.empty()) {
                            constexpr int resourceWidth = 28;
                            constexpr int limitWidth = 22;
                            constexpr int separatorWidth = 76;
                            std::cout << "\nResource Limits:\n";
                            std::cout << "  " << std::left << std::setw(resourceWidth) << "Limit"
                                      << std::setw(limitWidth) << "Soft Limit"
                                      << std::setw(limitWidth) << "Hard Limit"
                                      << "Units\n";
                            std::cout << "  " << std::string(separatorWidth, '-') << "\n";
                            for (const auto& lim : info.limits) {
                                std::cout << "  " << std::left << std::setw(resourceWidth) << lim.resource
                                          << std::setw(limitWidth) << lim.softLimit
                                          << std::setw(limitWidth) << lim.hardLimit
                                          << lim.units << "\n";
                            }
                        } else {
                            std::cout << "\nNo resource limits found.\n";
                        }
                    } else {
                        std::cerr << "\nError reading resource limits: " << limitsResult.error().message() << "\n";
                    }
                }
                if (args.showCgroupInfo) {
                    auto cgroupResult = analyzer.getProcessCgroupInfo(targetPid);
                    if (cgroupResult) {
                        auto& info = *cgroupResult;
                        if (!info.entries.empty()) {
                            std::cout << "\nCgroup Membership:\n";
                            for (const auto& entry : info.entries) {
                                std::cout << "  " << entry.id << ":"
                                          << entry.controllers << ":"
                                          << entry.path << "\n";
                            }
                        } else {
                            std::cout << "\nNo cgroup entries found.\n";
                        }
                    } else {
                        std::cerr << "\nError reading cgroup info: " << cgroupResult.error().message() << "\n";
                    }
                }
                if (args.showPerf) {
                    auto cpuResult = analyzer.getProcessCpuUsage(
                        targetPid, std::chrono::milliseconds(args.perfDurationMs));
                    auto ioResult = analyzer.getProcessDiskIoUsage(
                        targetPid, std::chrono::milliseconds(args.perfDurationMs));
                    std::cout << "\nPerformance (" << args.perfDurationMs << " ms sample):\n";
                    if (cpuResult) {
                        std::cout << "  CPU:         " << std::fixed << std::setprecision(1)
                                  << cpuResult->cpuPercentage << "%\n";
                    } else {
                        std::cerr << "  CPU:         error: " << cpuResult.error().message() << "\n";
                    }
                    if (ioResult) {
                        std::cout << "  Read:        " << ioResult->readBytesPerSec << " bytes/s\n";
                        std::cout << "  Write:       " << ioResult->writeBytesPerSec << " bytes/s\n";
                    } else {
                        std::cerr << "  I/O:         error: " << ioResult.error().message() << "\n";
                    }
                }
                return 0; // Done with pid-specific output
            }
            // Otherwise, add to list for table/csv/json output
            processesToDisplay.push_back(*infoResult);

        } else {
            std::cerr << "Error: Unknown command '" << args.command << "'.\n";
            printUsage();
            return 1;
        }
        
        // Determine columns for output
        std::vector<std::string> columns = args.selectedColumns;
        if (columns.empty()) {
            columns = getDefaultColumnsForTable(args.command == "list" && !args.briefMode);
        }

        // Output formatting
        if (processesToDisplay.empty() && args.command != "pid") {
            std::cout << "No processes found matching criteria.\n";
        } else if (args.outputFormat == "csv") {
            printProcessCsv(processesToDisplay, columns);
        } else if (args.outputFormat == "json") {
            printProcessJson(processesToDisplay, columns);
        } else if (args.outputFormat == "vertical") {
            for (size_t i = 0; i < processesToDisplay.size(); ++i) {
                printVerticalProcessDetails(processesToDisplay[i]);
                if (i + 1 < processesToDisplay.size()) {
                    std::cout << "\n";
                }
            }
        } else {
            printProcessTable(processesToDisplay, columns, args.noTruncateCmdline);
        }
    } catch (const std::exception& e) {
        std::cerr << "Unhandled exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

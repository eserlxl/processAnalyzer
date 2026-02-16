// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "utils/core.h"
#include "analyzer/analyzer_core.h"
#include "analyzer/network_model.h"

#include <filesystem>
#include <sstream>
#include <vector>
#include <charconv>
#include <format>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <set>
#include <bit>
#include <cstring>

constexpr int decimalBase = 10;

namespace {
    constexpr std::string_view anyIpV4AddrPort = "00000000:0000";
    constexpr std::string_view anyIpV6AddrPort = "00000000000000000000000000000000:0000";
    constexpr int hexBase = 16;
    constexpr size_t ipv4HexLen = 8;
    constexpr size_t ipv6HexLen = 32;
    constexpr size_t ipv6ChunkLen = 8;
    constexpr size_t ipv6ChunkCount = 4;
    constexpr std::string_view ipv4MappedPrefix = "0000000000000000FFFF0000";
    constexpr size_t ipv4HexLenInIpv6 = 8;

    // Holds the result of parsing a procfs network address entry.
    struct IpPortInfo {
        std::string ip;
        uint16_t port = 0;
        AddressFamily family = AddressFamily::unknown;
        bool success = false;
    };


    IpPortInfo parseIpPort(const std::string& addrStr) {
        IpPortInfo result;
        size_t colonPos = addrStr.find(':');
        if (colonPos == std::string::npos) return result;

        std::string ipHex = addrStr.substr(0, colonPos);
        std::string portHex = addrStr.substr(colonPos + 1);

        if (auto parsedPort = utils::parseIntegerNoThrow<long>(portHex, hexBase)) {
            result.port = static_cast<uint16_t>(*parsedPort);
        } else {
            return result; // Invalid port
        }

        if (ipHex.length() == ipv4HexLen) { // IPv4
            auto ipValue = utils::parseIntegerNoThrow<uint32_t>(ipHex, hexBase);
            if (!ipValue) return result;

            // The IP address in /proc/net/tcp is a little-endian hex number.
            // We need to format it to the standard dot-decimal notation.
            // Example: 0100007F -> 7F.00.00.01 -> 127.0.0.1
            const unsigned octet1 = *ipValue & 0xFFU;
            const unsigned octet2 = (*ipValue >> 8U) & 0xFFU;
            const unsigned octet3 = (*ipValue >> 16U) & 0xFFU;
            const unsigned octet4 = (*ipValue >> 24U) & 0xFFU;
            result.ip = std::format("{}.{}.{}.{}", octet1, octet2, octet3, octet4);
            result.family = AddressFamily::iPv4;
            result.success = true;
        } else if (ipHex.length() == ipv6HexLen) { // IPv6
            // The /proc/net/tcp6 file stores IPv6 address chunks in host-byte order (so, little-endian on x86).
            // We need to convert them to network byte order (big-endian) for standard library functions.
            
            // Handle IPv4-mapped IPv6 addresses (::ffff:a.b.c.d) explicitly for clarity and to avoid
            // potential formatting issues with inet_ntop.
            if (ipHex.starts_with(ipv4MappedPrefix)) {
                auto ipValue = utils::parseIntegerNoThrow<uint32_t>(ipHex.substr(ipv4MappedPrefix.length(), ipv4HexLenInIpv6), hexBase);
                if (!ipValue) return result;

                // The IPv4 part is also stored little-endian, so we read it the same way as a normal IPv4 address.
                const unsigned octet1 = *ipValue & 0xFFU;
                const unsigned octet2 = (*ipValue >> 8U) & 0xFFU;
                const unsigned octet3 = (*ipValue >> 16U) & 0xFFU;
                const unsigned octet4 = (*ipValue >> 24U) & 0xFFU;
                result.ip = std::format("::ffff:{}.{}.{}.{}", octet1, octet2, octet3, octet4);
                result.family = AddressFamily::iPv6;
                result.success = true;
                return result;
            }
            
            struct in6_addr in6{};
            for(size_t i = 0; i < ipv6ChunkCount; ++i) {
                auto chunkValue = utils::parseIntegerNoThrow<uint32_t>(ipHex.substr(i * ipv6ChunkLen, ipv6ChunkLen), hexBase);
                if (!chunkValue) return result;
                // Assign to the 32-bit representation of the IPv6 address, swapping bytes if needed.
                in6.s6_addr32[i] = *chunkValue;
            }

            std::array<char, INET6_ADDRSTRLEN> buf{};
            if (inet_ntop(AF_INET6, &in6, buf.data(), buf.size())) {
                result.ip = std::string(buf.data());
                result.family = AddressFamily::iPv6;
                result.success = true;
            }
        }
        return result;
    }

    std::string getTcpState(const std::string& stateHex) {
        if (auto stateInt = utils::parseIntegerNoThrow<int>(stateHex, hexBase)) {
            switch (*stateInt) {
                case TCP_ESTABLISHED: return "ESTABLISHED";
                case TCP_SYN_SENT: return "SYN_SENT";
                case TCP_SYN_RECV: return "SYN_RECV";
                case TCP_FIN_WAIT1: return "FIN_WAIT1";
                case TCP_FIN_WAIT2: return "FIN_WAIT2";
                case TCP_TIME_WAIT: return "TIME_WAIT";
                case TCP_CLOSE: return "CLOSE";
                case TCP_CLOSE_WAIT: return "CLOSE_WAIT";
                case TCP_LAST_ACK: return "LAST_ACK";
                case TCP_LISTEN: return "LISTEN";
                case TCP_CLOSING: return "CLOSING";
                default: return "UNKNOWN";
            }
        }
        return "UNKNOWN";
    }

    constexpr size_t kMinNetFileColumns = 10; // Minimum columns expected, inode is at index 9.
    constexpr size_t kNetFileInodeColumn = 9;
    
    std::vector<NetworkConnection> parseNetFile(const std::filesystem::path& filePath, std::string_view protocolName) {
        std::vector<NetworkConnection> connections;
        auto content = utils::readTextFile(filePath);
        if (!content) {
            return connections;
        }

        std::stringstream ss(*content);
        std::string line;
        // Skip header line.
        if (!std::getline(ss, line)) {
            return connections;
        }

        const bool isUdp = protocolName.starts_with("UDP");

        while (std::getline(ss, line)) {
            std::stringstream lineSs(line);
            std::vector<std::string> tokens;
            std::string token;
            while(lineSs >> token) {
                tokens.push_back(token);
            }

            constexpr size_t tcpStateTokenIndex = 3;
            constexpr size_t tcpTimeoutTokenIndex = 10;
            constexpr size_t tcpInodeTokenIndex = 11;
            constexpr size_t udpTimeoutTokenIndex = 8;
            constexpr size_t udpInodeTokenIndex = 9;

            size_t stateTokenIndex = tcpStateTokenIndex;
            size_t timeoutTokenIndex = tcpTimeoutTokenIndex;
            size_t inodeTokenIndex = tcpInodeTokenIndex;

            if (isUdp) {
                // UDP format differs:
                // sl local_address rem_address rx_queue tr tm->when retrnsmt uid timeout inode ...
                // Indices: 0  1               2           3     4  5        6        7   8       9
                stateTokenIndex = -1; // No direct state field for UDP
                timeoutTokenIndex = udpTimeoutTokenIndex;
                inodeTokenIndex = udpInodeTokenIndex;
            }

            // Ensure we have enough tokens for the required fields.
            // Min size for UDP is 10 (up to inode).
            // Min size for TCP is 12 (up to timeout, then inode).
            if (isUdp) {
                if (tokens.size() <= inodeTokenIndex) continue;
            } else { // TCP
                if (tokens.size() <= timeoutTokenIndex) continue;
            }
            
            const auto& localAddrStr = tokens[1];
            const auto& remoteAddrStr = tokens[2];
            
            auto localInfo = parseIpPort(localAddrStr);
            if (!localInfo.success) continue;

            auto remoteInfo = parseIpPort(remoteAddrStr);
            // Allow "any" addresses but continue if parsing fails for other addresses.
            if (!remoteInfo.success && remoteAddrStr != anyIpV4AddrPort && remoteAddrStr != anyIpV6AddrPort) {
                 continue;
            }

            NetworkConnection conn;
            conn.protocol = std::string(protocolName);
            conn.addressFamily = localInfo.family;
            conn.socketType = isUdp ? SocketType::udp : SocketType::tcp;
            conn.localAddress = localInfo.ip;
            conn.localPort = localInfo.port;

            // Correctly handle "any" addresses for remote IP.
            if (remoteAddrStr == anyIpV4AddrPort || remoteAddrStr == anyIpV6AddrPort) {
                conn.remoteAddress = "*";
                conn.remotePort = 0;
            } else {
                conn.remoteAddress = remoteInfo.ip;
                conn.remotePort = remoteInfo.port;
            }
            
            // Assign state
            if (isUdp) {
                conn.state = "UNKNOWN";
            } else { // TCP
                // stateStr is tokens[stateTokenIndex] which is tokens[3]
                conn.state = getTcpState(tokens[stateTokenIndex]); 
            }

            // Parse timeout
            if (auto timeoutVal = utils::parseIntegerNoThrow<uint64_t>(tokens[timeoutTokenIndex], decimalBase)) {
                conn.timeout = *timeoutVal; // Assuming NetworkConnection has a 'timeout' member
            } else {
                conn.timeout = 0; // Default or error value
            }

            // Parse inode
            if (auto inodeVal = utils::parseIntegerNoThrow<uint64_t>(tokens[inodeTokenIndex], decimalBase)) {
                conn.inode = *inodeVal;
            } else {
                conn.inode = 0; // Default or error value
            }
            
            connections.push_back(conn);
        }
        return connections;
    }
}

utils::Result<std::vector<NetworkConnection>> ProcessAnalyzer::getNetworkConnections(int pid) const {
    auto fdsResult = getProcessOpenFileDetails(pid);
    if (!fdsResult) {
        return std::unexpected(fdsResult.error());
    }

    std::set<uint64_t> socketInodes;
    constexpr int socketInodePrefixLen = 8; // "socket:["
    constexpr int socketInodeSuffixLen = 1; // "]"

    for (const auto& fdInfo : *fdsResult) {
        if (fdInfo.type == OpenFileType::Socket) {
            if (fdInfo.path.starts_with("socket:[") && fdInfo.path.back() == ']' &&
                fdInfo.path.length() > (socketInodePrefixLen + socketInodeSuffixLen))
            {
                std::string_view inodeView(fdInfo.path.data() + socketInodePrefixLen, fdInfo.path.length() - socketInodePrefixLen - socketInodeSuffixLen);
                if (auto parsedInode = utils::parseIntegerNoThrow<uint64_t>(inodeView, decimalBase)) {
                    socketInodes.insert(*parsedInode);
                }
            }
        }
    }

    if (socketInodes.empty()) {
        return std::vector<NetworkConnection>{};
    }

    std::vector<NetworkConnection> connections;
    std::filesystem::path netPath = procPath / "net";

    auto processConnection = [&](const std::filesystem::path& path, std::string_view protoName) {
        auto parsedConnections = parseNetFile(path, protoName);
        for(const auto& conn : parsedConnections) {
            if(socketInodes.contains(conn.inode)) {
                connections.push_back(conn);
            }
        }
    };

    processConnection(netPath / "tcp", "TCP");
    processConnection(netPath / "tcp6", "TCP6");
    processConnection(netPath / "udp", "UDP");
    processConnection(netPath / "udp6", "UDP6");

    return connections;
}

utils::Result<std::vector<NetworkInterfaceStats>> ProcessAnalyzer::getNetworkInterfaceStats() const {
    std::vector<NetworkInterfaceStats> stats;
    std::filesystem::path netDevPath = procPath / "net" / "dev";
    auto contentOpt = utils::readTextFile(netDevPath);
    if (!contentOpt) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::fileNotFound));
    }

    std::stringstream ss(*contentOpt);
    std::string line;
    std::getline(ss, line); // header 1
    std::getline(ss, line); // header 2

    while(std::getline(ss, line)) {
        if (utils::trim(line).empty()) {
            continue;
        }
        std::stringstream lineSs(line);
        std::string interfaceName;
        std::getline(lineSs, interfaceName, ':');
        if (interfaceName.empty()) {
            continue;
        }
        interfaceName = utils::trim(interfaceName);
        
        NetworkInterfaceStats stat;
        stat.interfaceName = interfaceName;
        
        uint64_t dummy1;
        uint64_t dummy2;
        uint64_t dummy3;
        uint64_t dummy4; // For skipping fifo, frame, compressed, multicast
        if (!(lineSs >> stat.rxBytes >> stat.rxPackets >> stat.rxErrors >> stat.rxDropped
                     >> dummy1 >> dummy2 >> dummy3 >> dummy4
                     >> stat.txBytes >> stat.txPackets >> stat.txErrors >> stat.txDropped)) {
            continue;
        }
               
        stats.push_back(stat);
    }
    if (stats.empty()) {
        return std::unexpected(utils::make_error_code(utils::UtilsError::analyzerParsingError));
    }
    return stats;
}

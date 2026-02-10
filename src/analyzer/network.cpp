// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "analyzer/core.h"
#include "utils/core.h"

#include <filesystem>
#include <sstream>
#include <vector>
#include <charconv>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <set>

namespace {
    template <typename TInt>
    std::optional<TInt> parseIntegerNoThrow(std::string_view text, int base = 10) {
        TInt value{};
        const char* begin = text.data();
        const char* end = begin + text.size();
        const auto [ptr, ec] = std::from_chars(begin, end, value, base);
        if (ec != std::errc{} || ptr != end) {
            return std::nullopt;
        }
        return value;
    }

    // Internal struct to temporarily hold NetworkConnection data along with its inode
    // for filtering purposes before converting to the public NetworkConnection struct.
    struct InternalNetworkConnection {
        NetworkConnection baseConn;
        int inode;
    };

    enum class TcpState : std::uint8_t {
        established = 1,
        synSent,
        synRecv,
        finWait1,
        finWait2,
        timeWait,
        close,
        closeWait,
        lastAck,
        listen,
        closing,
        unknown
    };

    std::vector<InternalNetworkConnection> parseNetFileHelper(const std::filesystem::path& filePath, std::string_view protocolPrefix) {
        std::vector<InternalNetworkConnection> internalConnections;
        auto content = utils::readTextFile(filePath.string());
        if (!content) {
            return internalConnections;
        }

        std::stringstream ss(*content);
        std::string line;
        std::getline(ss, line); // Skip header

        while (std::getline(ss, line)) {
            std::stringstream lineSs(line);
            std::string sl, localAddrStr, remoteAddrStr, stStr, txRxQueue, trTmWhen, retrnsmt, uid, timeout, inodeStr;
            
            if (!(lineSs >> sl >> localAddrStr >> remoteAddrStr >> stStr >> txRxQueue >> trTmWhen >> retrnsmt >> uid >> timeout >> inodeStr)) {
                continue;
            }

            InternalNetworkConnection internalConn;
            internalConn.baseConn.protocol = protocolPrefix;
            internalConn.inode = 0;
            if (auto inode = parseIntegerNoThrow<int>(inodeStr)) {
                internalConn.inode = *inode;
            }

            int stateInt = 0;
            if (auto parsedState = parseIntegerNoThrow<int>(stStr, 16)) {
                stateInt = *parsedState;
            }
            
            switch (static_cast<TcpState>(stateInt)) {
                case TcpState::established: internalConn.baseConn.state = "ESTABLISHED"; break;
                case TcpState::synSent: internalConn.baseConn.state = "SYN_SENT"; break;
                case TcpState::synRecv: internalConn.baseConn.state = "SYN_RECV"; break;
                case TcpState::finWait1: internalConn.baseConn.state = "FIN_WAIT1"; break;
                case TcpState::finWait2: internalConn.baseConn.state = "FIN_WAIT2"; break;
                case TcpState::timeWait: internalConn.baseConn.state = "TIME_WAIT"; break;
                case TcpState::close: internalConn.baseConn.state = "CLOSE"; break;
                case TcpState::closeWait: internalConn.baseConn.state = "CLOSE_WAIT"; break;
                case TcpState::lastAck: internalConn.baseConn.state = "LAST_ACK"; break;
                case TcpState::listen: internalConn.baseConn.state = "LISTEN"; break;
                case TcpState::closing: internalConn.baseConn.state = "CLOSING"; break;
                default: internalConn.baseConn.state = "UNKNOWN"; break;
            }

            auto parseIpPort = [](const std::string& addrStr) -> std::string {
                size_t colonPos = addrStr.find(':');
                if (colonPos == std::string::npos) return addrStr;
                
                std::string ipHex = addrStr.substr(0, colonPos);
                std::string portHex = addrStr.substr(colonPos + 1);
                
                long port = 0;
                if (auto parsedPort = parseIntegerNoThrow<long>(portHex, 16)) {
                    port = *parsedPort;
                }
                
                if (ipHex.length() == 8) { // IPv4
                    auto ipValue = parseIntegerNoThrow<unsigned int>(ipHex, 16);
                    if (!ipValue) return addrStr;
                    
                    const unsigned octet1 = *ipValue & 0xFFU;
                    const unsigned octet2 = (*ipValue >> 8U) & 0xFFU;
                    const unsigned octet3 = (*ipValue >> 16U) & 0xFFU;
                    const unsigned octet4 = (*ipValue >> 24U) & 0xFFU;
                    return std::to_string(octet1) + "." + std::to_string(octet2) + "." +
                           std::to_string(octet3) + "." + std::to_string(octet4) + ":" +
                           std::to_string(port);
                } else if (ipHex.length() == 32) { // IPv6
                    struct in6_addr in6{};
                    for(int i = 0; i < 4; ++i) {
                        auto chunkValue = parseIntegerNoThrow<uint32_t>(ipHex.substr(i * 8, 8), 16);
                        if (!chunkValue) return addrStr;
                        in6.s6_addr32[i] = __builtin_bswap32(*chunkValue);
                    }
                    char buf[INET6_ADDRSTRLEN];
                    if (inet_ntop(AF_INET6, &in6, buf, sizeof(buf))) {
                        return std::string(buf) + ":" + std::to_string(port);
                    }
                }
                return addrStr;
            };

            internalConn.baseConn.localAddress = parseIpPort(localAddrStr);
            internalConn.baseConn.remoteAddress = (remoteAddrStr == "00000000:0000" || remoteAddrStr == "00000000000000000000000000000000:0000") ? "*" : parseIpPort(remoteAddrStr);

            internalConnections.push_back(internalConn);
        }
        return internalConnections;
    }
}

utils::Result<std::vector<NetworkConnection>> ProcessAnalyzer::getNetworkConnections(int pid) const {
    auto fdsResult = getProcessOpenFileDetails(pid);
    if (!fdsResult) {
        return std::unexpected(fdsResult.error());
    }

    std::set<int> socketInodes;
    constexpr int socketInodePrefixLen = 8; // "socket:["
    constexpr int socketInodeSuffixLen = 1; // "]"

    for (const auto& fdInfo : *fdsResult) {
        if (fdInfo.type == OpenFileType::Socket) {
            if (fdInfo.path.starts_with("socket:[")) {
                std::string inodeStr = fdInfo.path.substr(socketInodePrefixLen, fdInfo.path.length() - socketInodePrefixLen - socketInodeSuffixLen);
                if(utils::isInteger(inodeStr)){
                    if (auto parsedInode = parseIntegerNoThrow<int>(inodeStr)) {
                        socketInodes.insert(*parsedInode);
                    }
                }
            }
        }
    }

    std::vector<NetworkConnection> connections;
    std::filesystem::path netPath = procPath / "net";

    auto tcp4_internal = parseNetFileHelper(netPath / "tcp", "TCP");
    for(const auto& conn : tcp4_internal) {
        if(socketInodes.contains(conn.inode)) connections.push_back(conn.baseConn);
    }

    auto tcp6_internal = parseNetFileHelper(netPath / "tcp6", "TCP6");
    for(const auto& conn : tcp6_internal) {
        if(socketInodes.contains(conn.inode)) connections.push_back(conn.baseConn);
    }

    auto udp4_internal = parseNetFileHelper(netPath / "udp", "UDP");
    for(const auto& conn : udp4_internal) {
        if(socketInodes.contains(conn.inode)) connections.push_back(conn.baseConn);
    }

    auto udp6_internal = parseNetFileHelper(netPath / "udp6", "UDP6");
    for(const auto& conn : udp6_internal) {
        if(socketInodes.contains(conn.inode)) connections.push_back(conn.baseConn);
    }

    return connections;
}

utils::Result<std::vector<NetworkInterfaceStats>> ProcessAnalyzer::getNetworkInterfaceStats() const {
    std::vector<NetworkInterfaceStats> stats;
    std::filesystem::path netDevPath = procPath / "net" / "dev";
    auto contentOpt = utils::readTextFile(netDevPath.string());
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
        
        unsigned long dummy1, dummy2, dummy3, dummy4; // For skipping fifo, frame, compressed, multicast
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

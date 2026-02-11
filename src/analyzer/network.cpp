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
#include <bit>
#include <cstring>

namespace {
    constexpr std::string_view anyIpV4AddrPort = "00000000:0000";
    constexpr std::string_view anyIpV6AddrPort = "00000000000000000000000000000000:0000";
    constexpr int hexBase = 16;
    constexpr size_t ipv4HexLen = 8;
    constexpr size_t ipv6HexLen = 32;
    constexpr size_t ipv6ChunkLen = 8;
    constexpr size_t ipv6ChunkCount = 4;

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
        auto content = utils::readTextFile(filePath);
        if (!content) {
            return internalConnections;
        }

        std::stringstream ss(*content);
        std::string line;
        std::getline(ss, line); // Skip header

        while (std::getline(ss, line)) {
            std::stringstream lineSs(line);
            std::string dummySl;
            std::string localAddrStr;
            std::string remoteAddrStr;
            std::string stStr;
            std::string dummyTxRxQueue;
            std::string dummyTrTmWhen;
            std::string dummyRetrnsmt;
            std::string dummyUid;
            std::string dummyTimeout;
            std::string inodeStr;
            
            if (!(lineSs >> dummySl >> localAddrStr >> remoteAddrStr >> stStr >> dummyTxRxQueue >> dummyTrTmWhen >> dummyRetrnsmt >> dummyUid >> dummyTimeout >> inodeStr)) {
                continue;
            }
            InternalNetworkConnection internalConn;
            internalConn.baseConn.protocol = protocolPrefix;
            internalConn.inode = 0;
            if (auto inode = utils::parseIntegerNoThrow<int>(inodeStr)) {
                internalConn.inode = *inode;
            }

            int stateInt = 0;
            if (auto parsedState = utils::parseIntegerNoThrow<int>(stStr, hexBase)) {
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
                if (auto parsedPort = utils::parseIntegerNoThrow<long>(portHex, hexBase)) {
                    port = *parsedPort;
                }
                
                if (ipHex.length() == ipv4HexLen) { // IPv4
                    auto ipValue = utils::parseIntegerNoThrow<unsigned int>(ipHex, hexBase);
                    if (!ipValue) return addrStr;
                    
                    const unsigned octet1 = *ipValue & 0xFFU;
                    const unsigned octet2 = (*ipValue >> 8U) & 0xFFU;
                    const unsigned octet3 = (*ipValue >> 16U) & 0xFFU;
                    const unsigned octet4 = (*ipValue >> 24U) & 0xFFU;
                    return std::to_string(octet1) + "." + std::to_string(octet2) + "." +
                           std::to_string(octet3) + "." + std::to_string(octet4) + ":" +
                           std::to_string(port);
                } 
                
                if (ipHex.length() == ipv6HexLen) { // IPv6
                    struct in6_addr in6{};
                    for(size_t i = 0; i < ipv6ChunkCount; ++i) {
                        auto chunkValue = utils::parseIntegerNoThrow<uint32_t>(ipHex.substr(i * ipv6ChunkLen, ipv6ChunkLen), hexBase);
                        if (!chunkValue) return addrStr;
                        
                        uint32_t val = *chunkValue;
                        if constexpr (std::endian::native == std::endian::big) {
                            val = std::byteswap(val);
                        }
                        std::memcpy(&in6.s6_addr[i * 4], &val, sizeof(val));
                    }
                    std::array<char, INET6_ADDRSTRLEN> buf{};
                    if (inet_ntop(AF_INET6, &in6, buf.data(), buf.size())) {
                        return std::string(buf.data()) + ":" + std::to_string(port);
                    }
                }
                return addrStr;
            };

            internalConn.baseConn.localAddress = parseIpPort(localAddrStr);
            internalConn.baseConn.remoteAddress = (remoteAddrStr == anyIpV4AddrPort || remoteAddrStr == anyIpV6AddrPort) ? "*" : parseIpPort(remoteAddrStr);

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
                            // Check if the path starts with "socket:[" and ends with "]"
                            // and has enough characters for an inode number between them.
                            if (fdInfo.path.starts_with("socket:[") && fdInfo.path.back() == ']' &&
                                fdInfo.path.length() > (socketInodePrefixLen + socketInodeSuffixLen))
                            {
                                // Extract the substring for the inode number.
                                // Use std::string_view for efficiency and safety.
                                std::string_view inodeView = std::string_view(fdInfo.path.data() + socketInodePrefixLen, fdInfo.path.length() - socketInodePrefixLen - socketInodeSuffixLen);
                                
                                if (utils::isInteger(inodeView)) { // Ensure it's a valid integer string
                                    if (auto parsedInode = utils::parseIntegerNoThrow<int>(inodeView)) {
                                        socketInodes.insert(*parsedInode);
                                    }
                                }
                            }        }
    }

    std::vector<NetworkConnection> connections;
    std::filesystem::path netPath = procPath / "net";

    auto tcp4Internal = parseNetFileHelper(netPath / "tcp", "TCP");
    for(const auto& conn : tcp4Internal) {
        if(socketInodes.contains(conn.inode)) connections.push_back(conn.baseConn);
    }

    auto tcp6Internal = parseNetFileHelper(netPath / "tcp6", "TCP6");
    for(const auto& conn : tcp6Internal) {
        if(socketInodes.contains(conn.inode)) connections.push_back(conn.baseConn);
    }

    auto udp4Internal = parseNetFileHelper(netPath / "udp", "UDP");
    for(const auto& conn : udp4Internal) {
        if(socketInodes.contains(conn.inode)) connections.push_back(conn.baseConn);
    }

    auto udp6Internal = parseNetFileHelper(netPath / "udp6", "UDP6");
    for(const auto& conn : udp6Internal) {
        if(socketInodes.contains(conn.inode)) connections.push_back(conn.baseConn);
    }

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
        
        unsigned long dummy1;
        unsigned long dummy2;
        unsigned long dummy3;
        unsigned long dummy4; // For skipping fifo, frame, compressed, multicast
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

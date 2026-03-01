// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef ANALYZER_NETWORK_MODEL_H
#define ANALYZER_NETWORK_MODEL_H

#include <string>
#include <cstdint>

// New for Iteration 9: Network Activity Monitoring
// Enum for network protocol families
enum class AddressFamily : std::uint8_t {
    iPv4,
    iPv6,
    unknown
};

// Enum for socket types
enum class SocketType : std::uint8_t {
    tcp,
    udp,
    raw,
    unixSocket, // Unix Domain Sockets
    unknown
};

struct NetworkConnection {
    std::string protocol;    // e.g., "TCP", "UDP", "TCP6", "UDP6"
    std::string localAddress;  // Local IP address, e.g., "127.0.0.1"
    std::string remoteAddress; // Remote IP address, e.g., "192.168.1.100" or "*"
    uint16_t localPort = 0;        // Local port number
    uint16_t remotePort = 0;       // Remote port number (0 if not connected/LISTEN)
    std::string state;       // Connection state, e.g., "ESTABLISHED", "LISTEN", "TIME_WAIT"
    uint64_t inode = 0;               // Socket inode number
    uint64_t timeout = 0;             // Connection timeout
    AddressFamily addressFamily = AddressFamily::unknown; // IPv4 or IPv6
    SocketType socketType = SocketType::unknown;     // TCP or UDP (or RAW/UNIX if expanded)
};

#endif // ANALYZER_NETWORK_MODEL_H

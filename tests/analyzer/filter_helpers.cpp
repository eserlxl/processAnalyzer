// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include <gtest/gtest.h>

#include "analyzer/internal/filter_helpers.h"
#include "analyzer/process_model.h"
#include "analyzer/network_model.h"

#include <vector>

namespace {

constexpr pid_t kProcPid = 100;
constexpr pid_t kProcPpid = 1;
constexpr uid_t kProcUid = 1000;
constexpr long kProcThreads = 8;
constexpr long long kProcRssKb = 4096;
constexpr long long kProcVmKb = 8192;
constexpr uint16_t kHttpsRemotePort = 443;
constexpr uint16_t kSshPort = 22;
constexpr uint16_t kAltHttpPort = 8080;
constexpr uint16_t kUnusedPort = 9999;

ProcessInfo makeProcess() {
    ProcessInfo p;
    p.pid = kProcPid;
    p.ppid = kProcPpid;
    p.uid = kProcUid;
    p.username = "alice";
    p.name = "firefox";
    p.state = "S";
    p.cmdline = "/usr/bin/firefox --no-remote";
    p.executablePath = "/usr/bin/firefox";
    p.threadCount = kProcThreads;
    p.residentMemory = kProcRssKb;
    p.virtualMemory = kProcVmKb;
    p.priority = 0;
    return p;
}

NetworkConnection makeConn(uint16_t localPort, const std::string& state,
                           const std::string& protocol, const std::string& remoteAddr) {
    NetworkConnection c;
    c.protocol = protocol;
    c.localAddress = "127.0.0.1";
    c.remoteAddress = remoteAddr;
    c.localPort = localPort;
    c.remotePort = kHttpsRemotePort;
    c.state = state;
    return c;
}

} // namespace

// --- passesStaticFilters ---------------------------------------------------

TEST(FilterHelpersTest, EmptyFilterPasses) {
    EXPECT_TRUE(Internal::passesStaticFilters(ProcessFilter{}, makeProcess()));
}

TEST(FilterHelpersTest, NameContainsHitAndMiss) {
    ProcessFilter hit;
    hit.nameContains = "fire";
    EXPECT_TRUE(Internal::passesStaticFilters(hit, makeProcess()));

    ProcessFilter miss;
    miss.nameContains = "chrome";
    EXPECT_FALSE(Internal::passesStaticFilters(miss, makeProcess()));
}

TEST(FilterHelpersTest, StateFilterMatchAndNonMatch) {
    ProcessFilter match;
    match.stateFilter = 'S';
    EXPECT_TRUE(Internal::passesStaticFilters(match, makeProcess()));

    ProcessFilter nonMatch;
    nonMatch.stateFilter = 'R';
    EXPECT_FALSE(Internal::passesStaticFilters(nonMatch, makeProcess()));
}

TEST(FilterHelpersTest, StateFilterEmptyStateIsNonMatch) {
    // A ProcessInfo whose state is empty must not match a state filter, and the
    // check must not call std::string::front() on an empty string (UB).
    ProcessInfo emptyState = makeProcess();
    emptyState.state.clear();

    ProcessFilter filter;
    filter.stateFilter = 'S';
    EXPECT_FALSE(Internal::passesStaticFilters(filter, emptyState));
}

TEST(FilterHelpersTest, UidFilterMatchAndNonMatch) {
    ProcessFilter match;
    match.uidFilter = static_cast<uint32_t>(kProcUid);
    EXPECT_TRUE(Internal::passesStaticFilters(match, makeProcess()));

    ProcessFilter nonMatch;
    nonMatch.uidFilter = 0U;
    EXPECT_FALSE(Internal::passesStaticFilters(nonMatch, makeProcess()));
}

TEST(FilterHelpersTest, ThreadRangeEdges) {
    // threadCount == kProcThreads: inclusive bounds pass.
    ProcessFilter within;
    within.minThreads = kProcThreads;
    within.maxThreads = kProcThreads;
    EXPECT_TRUE(Internal::passesStaticFilters(within, makeProcess()));

    ProcessFilter belowMin;
    belowMin.minThreads = kProcThreads + 1;
    EXPECT_FALSE(Internal::passesStaticFilters(belowMin, makeProcess()));

    ProcessFilter aboveMax;
    aboveMax.maxThreads = kProcThreads - 1;
    EXPECT_FALSE(Internal::passesStaticFilters(aboveMax, makeProcess()));
}

TEST(FilterHelpersTest, PpidFilterMatchAndNonMatch) {
    ProcessFilter match;
    match.ppidFilter = kProcPpid;
    EXPECT_TRUE(Internal::passesStaticFilters(match, makeProcess()));

    ProcessFilter nonMatch;
    nonMatch.ppidFilter = kProcPpid + 1;
    EXPECT_FALSE(Internal::passesStaticFilters(nonMatch, makeProcess()));
}

TEST(FilterHelpersTest, CustomPredicateGates) {
    ProcessFilter reject;
    reject.customPredicate = [](const ProcessInfo&) { return false; };
    EXPECT_FALSE(Internal::passesStaticFilters(reject, makeProcess()));

    ProcessFilter accept;
    accept.customPredicate = [](const ProcessInfo& p) { return p.pid == kProcPid; };
    EXPECT_TRUE(Internal::passesStaticFilters(accept, makeProcess()));
}

// --- passesNetworkFilter ---------------------------------------------------

TEST(FilterHelpersTest, NetworkFilterMatchesAnyConnection) {
    std::vector<NetworkConnection> conns{
        makeConn(kSshPort, "LISTEN", "TCP", "0.0.0.0"),
        makeConn(kAltHttpPort, "ESTABLISHED", "TCP", "10.0.0.5"),
    };
    ProcessFilter::NetworkFilterCriteria crit;
    crit.localPort = kAltHttpPort;
    EXPECT_TRUE(Internal::passesNetworkFilter(crit, conns));
}

TEST(FilterHelpersTest, NetworkFilterNoMatch) {
    std::vector<NetworkConnection> conns{
        makeConn(kSshPort, "LISTEN", "TCP", "0.0.0.0"),
    };
    ProcessFilter::NetworkFilterCriteria crit;
    crit.localPort = kUnusedPort;
    EXPECT_FALSE(Internal::passesNetworkFilter(crit, conns));
}

TEST(FilterHelpersTest, NetworkFilterEmptyConnectionsFails) {
    ProcessFilter::NetworkFilterCriteria crit;
    crit.state = "LISTEN";
    EXPECT_FALSE(Internal::passesNetworkFilter(crit, {}));
}

TEST(FilterHelpersTest, NetworkFilterRequiresAllCriteriaOnOneConnection) {
    // One connection matches the port, a different one matches the state;
    // no single connection matches both, so the filter must fail.
    std::vector<NetworkConnection> conns{
        makeConn(kAltHttpPort, "LISTEN", "TCP", "0.0.0.0"),
        makeConn(kSshPort, "ESTABLISHED", "TCP", "10.0.0.5"),
    };
    ProcessFilter::NetworkFilterCriteria crit;
    crit.localPort = kAltHttpPort;
    crit.state = "ESTABLISHED";
    EXPECT_FALSE(Internal::passesNetworkFilter(crit, conns));
}

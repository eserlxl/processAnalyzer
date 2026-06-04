// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#pragma once

#include "analyzer/process_model.h"
#include "analyzer/network_model.h"
#include <vector>

namespace Internal {

// Returns true if pInfo passes every ProcessFilter condition that does not
// require a network-connection lookup (i.e. everything except networkConnectionFilter).
[[nodiscard]] bool passesStaticFilters(const ProcessFilter& filter, const ProcessInfo& pInfo);

// Returns true if at least one connection in `conns` satisfies all criteria in netFilter.
[[nodiscard]] bool passesNetworkFilter(
    const ProcessFilter::NetworkFilterCriteria& netFilter,
    const std::vector<NetworkConnection>& conns
);

} // namespace Internal

// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#ifndef ANALYZER_CORE_H
#define ANALYZER_CORE_H

#include <vector>
#include <string>

enum class ProcessSortField {
    pid,
    ppid,
    owner,
    cpu,
    memory,
    name
};

#endif // ANALYZER_CORE_H

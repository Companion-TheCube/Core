/*
 ██████╗██╗  ██╗ █████╗ ████████╗██╗  ██╗██╗███████╗████████╗ ██████╗ ██████╗ ██╗   ██╗███████╗████████╗ ██████╗ ██████╗ ███████╗    ██████╗██████╗ ██████╗
██╔════╝██║  ██║██╔══██╗╚══██╔══╝██║  ██║██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗╚██╗ ██╔╝██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗██╔════╝   ██╔════╝██╔══██╗██╔══██╗
██║     ███████║███████║   ██║   ███████║██║███████╗   ██║   ██║   ██║██████╔╝ ╚████╔╝ ███████╗   ██║   ██║   ██║██████╔╝█████╗     ██║     ██████╔╝██████╔╝
██║     ██╔══██║██╔══██║   ██║   ██╔══██║██║╚════██║   ██║   ██║   ██║██╔══██╗  ╚██╔╝  ╚════██║   ██║   ██║   ██║██╔══██╗██╔══╝     ██║     ██╔═══╝ ██╔═══╝
╚██████╗██║  ██║██║  ██║   ██║   ██║  ██║██║███████║   ██║   ╚██████╔╝██║  ██║   ██║   ███████║   ██║   ╚██████╔╝██║  ██║███████╗██╗╚██████╗██║     ██║
 ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝╚══════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚══════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝ ╚═════╝╚═╝     ╚═╝
*/

/*
MIT License
Copyright (c) 2026 A-McD Technology LLC
*/

#include "chatHistoryStore.h"

#ifndef LOGGER_H
#include <logger.h>
#endif

#include <algorithm>
#include <chrono>

namespace {

long parseLongOrDefault(const std::string& value, long fallback = -1)
{
    try {
        return std::stol(value);
    } catch (...) {
        return fallback;
    }
}

int64_t parseInt64OrDefault(const std::string& value, int64_t fallback = 0)
{
    try {
        return std::stoll(value);
    } catch (...) {
        return fallback;
    }
}

bool olderThan(const ChatHistoryEntry& lhs, const ChatHistoryEntry& rhs)
{
    if (lhs.createdAtMs != rhs.createdAtMs) {
        return lhs.createdAtMs < rhs.createdAtMs;
    }
    return lhs.id < rhs.id;
}

} // namespace

ChatHistoryStore::ChatHistoryStore(std::shared_ptr<CubeDatabaseManager> dbManager)
    : dbManager_(std::move(dbManager))
{
}

std::vector<ChatHistoryEntry> ChatHistoryStore::getRecentHistory(const std::string& historyKey, size_t limit)
{
    std::scoped_lock lock(mutex_);
    if (historyKey.empty() || limit == 0) {
        return {};
    }

    auto* db = database();
    if (!db) {
        return {};
    }

    const auto rows = db->selectData(
        DB_NS::TableNames::TOOL_RESPONSE_HISTORY,
        { "id", "created_at_ms", "history_key", "intent_name", "capability_name", "request_text", "display_response" },
        { DB_NS::Predicate { "history_key", historyKey } });
    if (rows.empty()) {
        return {};
    }

    std::vector<ChatHistoryEntry> entries;
    entries.reserve(rows.size());
    for (const auto& row : rows) {
        if (row.size() < 7) {
            continue;
        }
        entries.push_back(ChatHistoryEntry {
            .id = parseLongOrDefault(row[0]),
            .createdAtMs = parseInt64OrDefault(row[1]),
            .historyKey = row[2],
            .intentName = row[3],
            .capabilityName = row[4],
            .requestText = row[5],
            .displayResponse = row[6]
        });
    }

    entries = sortOldestToNewest(std::move(entries));
    if (entries.size() <= limit) {
        return entries;
    }
    const auto startIndex = entries.size() - limit;
    return std::vector<ChatHistoryEntry>(
        entries.begin() + static_cast<std::ptrdiff_t>(startIndex),
        entries.end());
}

bool ChatHistoryStore::appendHistory(const ChatHistoryEntry& entry)
{
    if (entry.historyKey.empty() || entry.requestText.empty() || entry.displayResponse.empty()) {
        return false;
    }

    std::scoped_lock lock(mutex_);
    auto* db = database();
    if (!db) {
        return false;
    }

    const auto createdAtMs = entry.createdAtMs > 0 ? entry.createdAtMs : currentEpochMs();
    if (db->insertData(
        DB_NS::TableNames::TOOL_RESPONSE_HISTORY,
        { "created_at_ms", "history_key", "intent_name", "capability_name", "request_text", "display_response" },
        { std::to_string(createdAtMs), entry.historyKey, entry.intentName, entry.capabilityName, entry.requestText, entry.displayResponse }) < 0) {
        CubeLog::warning("ChatHistoryStore: failed to append history entry: " + db->getLastError());
        return false;
    }

    const auto rows = db->selectData(
        DB_NS::TableNames::TOOL_RESPONSE_HISTORY,
        { "id", "created_at_ms", "history_key", "intent_name", "capability_name", "request_text", "display_response" },
        { DB_NS::Predicate { "history_key", entry.historyKey } });
    std::vector<ChatHistoryEntry> entries;
    entries.reserve(rows.size());
    for (const auto& row : rows) {
        if (row.size() < 7) {
            continue;
        }
        entries.push_back(ChatHistoryEntry {
            .id = parseLongOrDefault(row[0]),
            .createdAtMs = parseInt64OrDefault(row[1]),
            .historyKey = row[2],
            .intentName = row[3],
            .capabilityName = row[4],
            .requestText = row[5],
            .displayResponse = row[6]
        });
    }
    entries = sortOldestToNewest(std::move(entries));
    if (entries.size() <= 10) {
        return true;
    }

    const auto pruneCount = entries.size() - 10;
    for (size_t i = 0; i < pruneCount; ++i) {
        if (entries[i].id < 0) {
            continue;
        }
        if (!db->deleteData(
                DB_NS::TableNames::TOOL_RESPONSE_HISTORY,
                { DB_NS::Predicate { "id", std::to_string(entries[i].id) } })) {
            CubeLog::warning("ChatHistoryStore: failed to prune history entry: " + db->getLastError());
            return false;
        }
    }

    return true;
}

Database* ChatHistoryStore::database() const
{
    if (!dbManager_) {
        return nullptr;
    }
    return dbManager_->getDatabase("chat_history");
}

int64_t ChatHistoryStore::currentEpochMs()
{
    const auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

std::vector<ChatHistoryEntry> ChatHistoryStore::sortOldestToNewest(std::vector<ChatHistoryEntry> entries)
{
    std::sort(entries.begin(), entries.end(), olderThan);
    return entries;
}

/*
 ██████╗██╗  ██╗ █████╗ ████████╗██╗  ██╗██╗███████╗████████╗ ██████╗ ██████╗ ██╗   ██╗███████╗████████╗ ██████╗ ██████╗ ███████╗   ██╗  ██╗
██╔════╝██║  ██║██╔══██╗╚══██╔══╝██║  ██║██║██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗╚██╗ ██╔╝██╔════╝╚══██╔══╝██╔═══██╗██╔══██╗██╔════╝   ██║  ██║
██║     ███████║███████║   ██║   ███████║██║███████╗   ██║   ██║   ██║██████╔╝ ╚████╔╝ ███████╗   ██║   ██║   ██║██████╔╝█████╗     ███████║
██║     ██╔══██║██╔══██║   ██║   ██╔══██║██║╚════██║   ██║   ██║   ██║██╔══██╗  ╚██╔╝  ╚════██║   ██║   ██║   ██║██╔══██╗██╔══╝     ██╔══██║
╚██████╗██║  ██║██║  ██║   ██║   ██║  ██║██║███████║   ██║   ╚██████╔╝██║  ██║   ██║   ███████║   ██║   ╚██████╔╝██║  ██║███████╗██╗██║  ██║
 ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝╚══════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝   ╚═╝   ╚══════╝   ╚═╝    ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝╚═╝  ╚═╝
*/

/*
MIT License
Copyright (c) 2026 A-McD Technology LLC
*/

#pragma once

#include "db.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct ChatHistoryEntry {
    long id = -1;
    int64_t createdAtMs = 0;
    std::string historyKey;
    std::string intentName;
    std::string capabilityName;
    std::string requestText;
    std::string displayResponse;
};

class ChatHistoryStore {
public:
    explicit ChatHistoryStore(std::shared_ptr<CubeDatabaseManager> dbManager);

    std::vector<ChatHistoryEntry> getRecentHistory(const std::string& historyKey, size_t limit = 10);
    bool appendHistory(const ChatHistoryEntry& entry);

private:
    Database* database() const;
    static int64_t currentEpochMs();
    static std::vector<ChatHistoryEntry> sortOldestToNewest(std::vector<ChatHistoryEntry> entries);

    std::shared_ptr<CubeDatabaseManager> dbManager_;
    mutable std::mutex mutex_;
};

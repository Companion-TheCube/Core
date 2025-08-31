/*
MIT License
Copyright (c) 2025 A-McD Technology LLC
*/

#include "wakeWordClient.h"
#ifndef LOGGER_H
#include <logger.h>
#endif

#include <cerrno>
#include <cstring>
#include <chrono>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static constexpr const char* kOpenWWSock = "/tmp/openww";
static constexpr int kWakeRetriggerMs = 5000;
static constexpr int kHealthLogIntervalSec = 5;

WakeWordClient::WakeWordClient() {}
WakeWordClient::~WakeWordClient() { stop(); }

void WakeWordClient::setOnWake(OnWake cb) { onWake_ = std::move(cb); }

void WakeWordClient::start()
{
    stop_.store(false);
    thread_ = std::jthread([this](std::stop_token st) { this->run(st); });
}

void WakeWordClient::stop()
{
    stop_.store(true);
    if (thread_.joinable()) thread_.request_stop();
}

void WakeWordClient::submit(const std::vector<int16_t>& block)
{
    txQueue_->push(block);
}

void WakeWordClient::run(std::stop_token st)
{
    using clock = std::chrono::steady_clock;
    size_t reconnects = 0;
    size_t wakeCount = 0;
    auto lastHealth = clock::now();
    while (!st.stop_requested() && !stop_.load()) {
        int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd < 0) {
            CubeLog::error("WakeWordClient: socket() failed");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }
        sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, kOpenWWSock, sizeof(addr.sun_path) - 1);
        if (connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            ++reconnects;
            CubeLog::error("WakeWordClient: connect() failed, reconnects=" + std::to_string(reconnects));
            close(sockfd);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        // Connected. Pump audio and poll for control messages.
        while (!st.stop_requested() && !stop_.load()) {
            auto opt = txQueue_->pop();
            if (!opt) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
            const auto& data = *opt;
            ssize_t wrote = write(sockfd, data.data(), data.size() * sizeof(int16_t));
            if (wrote < 0) {
                ++reconnects;
                CubeLog::error("WakeWordClient: write() failed, reconnects=" + std::to_string(reconnects));
                break; // reconnect loop
            }
            // Non-blocking read for control: expect small lines like "DETECTED____"
            char buf[128];
            ssize_t got = recv(sockfd, buf, sizeof(buf) - 1, MSG_DONTWAIT);
            if (got > 0) {
                buf[got] = '\0';
                std::string line(buf);
                if (line.find("DETECTED____") != std::string::npos) {
                    auto now = std::chrono::steady_clock::now();
                    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastWake_).count();
                    if (ms > kWakeRetriggerMs) {
                        lastWake_ = now;
                        ++wakeCount;
                        CubeLog::info("WakeWordClient: wake detected, count=" + std::to_string(wakeCount));
                        if (onWake_) {
                            try { onWake_(); } catch (...) { CubeLog::error("WakeWordClient: onWake exception"); }
                        }
                    }
                }
            }
            // periodic health log
            auto now = clock::now();
            if (now - lastHealth > std::chrono::seconds(kHealthLogIntervalSec)) {
                lastHealth = now;
                CubeLog::debug("WakeWordClient health: txQ=" + std::to_string(txQueue_->size()) +
                               ", reconnects=" + std::to_string(reconnects) +
                               ", wakes=" + std::to_string(wakeCount));
            }
        }
        close(sockfd);
    }
}

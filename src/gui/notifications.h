#pragma once
#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H
#include <logger.h>
#ifndef GUI_H
#include "gui.h"
#endif
#ifndef API_I_H
#include "../api_i.h"
#endif

class Notifications : public I_API_Interface {
public:
    enum NotificationType {
        NOTIFICATION_OKAY,
        NOTIFICATION_WARNING,
        NOTIFICATION_ERROR,
        NOTIFICATION_YES_NO,
        NOTIFICATION_TYPE_COUNT
    };
    Notifications();
    ~Notifications();
    static void showNotification(std::string title, std::string message, NotificationType type);
    static void showNotificationWithCallback(std::string title, std::string message, NotificationType type, std::function<void()> callback);
    static void showNotificationWithCallback(std::string title, std::string message, NotificationType type, std::function<void()> callbackYes, std::function<void()> callbackNo);
    std::string getIntefaceName() const override;
    HttpEndPointData_t getHttpEndpointData() override;
};

#endif// NOTIFICATIONS_H
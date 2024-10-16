#include "notifications.h"


void test(){
    GUI::showMessageBox("test", "test");
}

// TODO: we'll need an endpoint to show a notification.
// TODO: showing a notification will need to add a record to the notifications table in the DB.

Notifications::Notifications()
{
    CubeLog::info("Notifications initialized");
}

Notifications::~Notifications()
{
    CubeLog::info("Notifications destroyed");
}

void Notifications::showNotification(std::string title, std::string message, NotificationType type)
{
    CubeLog::info("Notification shown: " + title + " - " + message);
}

void Notifications::showNotificationWithCallback(std::string title, std::string message, NotificationType type, std::function<void()> callback)
{
    CubeLog::info("Notification shown with callback: " + title + " - " + message);
}

void Notifications::showNotificationWithCallback(std::string title, std::string message, NotificationType type, std::function<void()> callbackYes, std::function<void()> callbackNo)
{
    CubeLog::info("Notification shown with callback: " + title + " - " + message);
}

std::string Notifications::getIntefaceName() const
{
    return "Notifications";
}

HttpEndPointData_t Notifications::getHttpEndpointData()
{
    HttpEndPointData_t actions;
    actions.push_back(
        {PRIVATE_ENDPOINT | POST_ENDPOINT,
            [&](const httplib::Request& req, httplib::Response& res) {
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Notification shown with callback");
            },
            "showNotificationOkayWarningError",
            {},
            "Show a notification with an optional callback" });
    actions.push_back(
        {PRIVATE_ENDPOINT | POST_ENDPOINT,
            [&](const httplib::Request& req, httplib::Response& res) {
                return EndpointError(EndpointError::ERROR_TYPES::ENDPOINT_NO_ERROR, "Notification shown with callback");
            },
            "showNotificationYesNo",
            {},
            "Show a yes/no notification with two callbacks" });
    return actions;
}
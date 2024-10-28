#ifndef BLUETOOTH_H
#define BLUETOOTH_H
#include <gio/gio.h>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <logger.h>

/**
 * @brief An object to hold information about a Bluetooth device
 * 
 */
struct BTDevice{
    std::string mac;
    std::string name;
    std::string alias;
    std::string address;
    std::string icon;
    std::string modalias;
    std::string rssi;
    std::string txpower;
    bool paired;
    bool connected;
    bool trusted;
    bool blocked;
};

/**
 * @brief Control the Bluetooth hardware (turn on/off, scan for devices, connect to devices)
 * 
 */
class BTControl{
    std::vector<BTDevice> devices;
    std::vector<BTDevice> pairedDevices;
    std::vector<BTDevice> connectedDevices;
    std::vector<BTDevice> availableDevices;
    GDBusConnection* connection;
    void on_device_found(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_device_disappeared(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_device_changed(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_device_properties_changed(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_device_acquired(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_device_lost(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_device_error(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_device_unhandled(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_device_unhandled_error(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
public:
    BTControl();
    ~BTControl();
    void scanForDevices();
    bool connectToDevice(BTDevice& device);
    bool disconnectFromDevice(BTDevice& device);
    bool pairWithDevice(BTDevice& device);
    std::vector<BTDevice> getDevices();
    std::vector<BTDevice> getPairedDevices();
    std::vector<BTDevice> getConnectedDevices();
    std::vector<BTDevice> getAvailableDevices();
    
};

/**
 * @brief Communicate with mobile devices over Bluetooth
 * 
 */
class BTServices{
    std::string id_token = ""; // JWT from CubeServer API
    std::string access_token = ""; // JWT from CubeServer API
    std::string refresh_token = ""; // JWT from CubeServer API
    std::string userName = ""; // Get from user
    std::string userEmail = ""; // Derive from JWT
    std::string cubeName = ""; // Let the user set this
public:
    BTServices();
    ~BTServices();
    void on_characteristic_read(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_characteristic_write(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_characteristic_notify(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_characteristic_changed(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_characteristic_value_changed(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_characteristic_properties_changed(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_characteristic_acquired(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_characteristic_lost(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_characteristic_error(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_characteristic_unhandled(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
    void on_characteristic_unhandled_error(GDBusConnection *connection, const gchar *sender_name, const gchar *object_path, const gchar *interface_name, const gchar *signal_name, GVariant *parameters, gpointer user_data);
};

/**
 * @brief Manager for all things Bluetooth
 * 
 */
class BTManager{
    BTControl* control;
    BTServices* services;
    std::jthread loopThread;
    GMainLoop* gloop;
public:
    BTManager();
    ~BTManager();
};

#endif// BLUETOOTH_H

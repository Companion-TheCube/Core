#include "bluetooth.h"

// TODO: Implement the Bluetooth functions

/**
 * This file will need to have a class for controller the hardware such as turning BT on and off, scanning for devices, and connecting to devices.
 * There will also need to be a class for communicating with mobiles devices over BT.
 * Perhaps make a class to communicate with other TheCubes over BT which can allow auto configuring of the network settings on a new device.
 * 
 */

static void handle_method_call(GDBusConnection* connection, const gchar* sender, const gchar* object_path, const gchar* interface_name, const gchar* method_name, GVariant* parameters, GDBusMethodInvocation* invocation, gpointer user_data)
{
    if (g_strcmp0(method_name, "ReadValue") == 0) {
        GVariant* value = g_variant_new("s", "Hello, World!");
        g_dbus_method_invocation_return_value(invocation, value);
    } else if (g_strcmp0(method_name, "WriteValue") == 0) {
        const gchar* value;
        g_variant_get(parameters, "(&s)", &value);
        CubeLog::info("Received: " + std::string(value));
        g_dbus_method_invocation_return_value(invocation, NULL);
    } else {
        CubeLog::error("Unknown method: " + std::string(method_name));
        g_dbus_method_invocation_return_error(invocation, G_DBUS_ERROR, G_DBUS_ERROR_UNKNOWN_METHOD, "Unknown method");
    }
}

static GVariant* handle_get_property(GDBusConnection* connection, const gchar* sender, const gchar* object_path, const gchar* interface_name, const gchar* property_name, GError** error, gpointer user_data)
{
    if (g_strcmp0(property_name, "UUID") == 0) {
        return g_variant_new_string("87654321-4321-6789-4321-6789abcdef01");
    } else if (g_strcmp0(property_name, "Service") == 0) {
        return g_variant_new_object_path(object_path);
    } else if (g_strcmp0(property_name, "Flags") == 0) {
        const gchar* flags[] = { "read", "write" };
        return g_variant_new_strv(flags, 2);
    }

    // Property not found
    *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED, "Property %s not found", property_name);
    return NULL;
}

static GVariant* service_get_property(GDBusConnection* connection,
                                      const gchar* sender,
                                      const gchar* objectPath,
                                      const gchar* interfaceName,
                                      const gchar* propertyName,
                                      GError** error,
                                      gpointer userData) {
    if (g_strcmp0(propertyName, "UUID") == 0) {
        return g_variant_new_string("12345678-1234-5678-1234-56789abcdef0");
    } else if (g_strcmp0(propertyName, "Primary") == 0) {
        return g_variant_new_boolean(TRUE);
    } else if (g_strcmp0(propertyName, "Characteristics") == 0) {
        const gchar* charPaths[] = { objectPath };
        return g_variant_new("ao", charPaths, 1);
    }

    *error = g_error_new(G_IO_ERROR, G_IO_ERROR_FAILED, "Property %s not found", propertyName);
    return NULL;
}

BTControl::BTControl()
{
    GError* error = NULL;
    this->connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error != NULL) {
        g_error_free(error);
        CubeLog::error("Failed to get system bus for Bluetooth");
        return;
    }
    const char* servicePath = "/com/TheCube/service0";
    const char* serviceInterface = "com.TheCube.service0";
    const char* characterPath = "/com/TheCube/characteristic0";
    const char* characterInterface = "com.TheCube.characteristic0";

    const gchar* serviceIntrospectionXML = 
        "<node>"
        "  <interface name='com.TheCube.service0'>"
        "    <method name='ReadValue'>"
        "      <arg type='s' name='value' direction='out'/>"
        "    </method>"
        "    <method name='WriteValue'>"
        "      <arg type='s' name='value' direction='in'/>"
        "    </method>"
        "    <signal name='ValueChanged'>"
        "      <arg type='s' name='value'/>"
        "    </signal>"
        "  </interface>"
        "</node>";
    const gchar* characterIntrospectionXML = 
        "<node>"
        "  <interface name='com.TheCube.characteristic0'>"
        "    <method name='ReadValue'>"
        "      <arg type='s' name='value' direction='out'/>"
        "    </method>"
        "    <method name='WriteValue'>"
        "      <arg type='s' name='value' direction='in'/>"
        "    </method>"
        "    <signal name='ValueChanged'>"
        "      <arg type='s' name='value'/>"
        "    </signal>"
        "  </interface>"
        "</node>";

    GDBusNodeInfo* serviceIntrospection = g_dbus_node_info_new_for_xml(serviceIntrospectionXML, &error);
    if (error != NULL) {
        g_error_free(error);
        CubeLog::error("Failed to create service introspection");
        return;
    }
    GDBusNodeInfo* characterIntrospection = g_dbus_node_info_new_for_xml(characterIntrospectionXML, &error);
    if (error != NULL) {
        g_error_free(error);
        CubeLog::error("Failed to create characteristic introspection");
        return;
    }
    static const GDBusInterfaceVTable charInterfaceVTable = {
        handle_method_call,
        handle_get_property,
        NULL  // No set_property implementation
    };

    static const GDBusInterfaceVTable serviceInterfaceVTable = {
        NULL,             // No method calls for the service
        service_get_property,
        NULL
    };

    guint serviceRegId = g_dbus_connection_register_object(this->connection,
                                                           servicePath,
                                                           serviceIntrospection->interfaces[0],
                                                           &serviceInterfaceVTable,
                                                           NULL,
                                                           NULL,
                                                           &error);
    if (error != NULL) {
        g_error_free(error);
        CubeLog::error("Failed to register service object");
        return;
    }
    guint charRegId = g_dbus_connection_register_object(
        connection,
        characterPath,
        characterIntrospection->interfaces[0],
        &charInterfaceVTable,
        NULL,  // user_data
        NULL,  // user_data_free_func
        &error);

    if (error) {
        g_printerr("Failed to register characteristic object: %s\n", error->message);
        g_error_free(error);
        return;
    }

}

BTControl::~BTControl()
{
}

void BTControl::scanForDevices()
{
}

bool BTControl::connectToDevice(BTDevice& device)
{
    return false;
}

bool BTControl::disconnectFromDevice(BTDevice& device)
{
    return false;
}

bool BTControl::pairWithDevice(BTDevice& device)
{
    return false;
}

std::vector<BTDevice> BTControl::getDevices()
{
    return std::vector<BTDevice>();
}

std::vector<BTDevice> BTControl::getPairedDevices()
{
    return std::vector<BTDevice>();
}

std::vector<BTDevice> BTControl::getConnectedDevices()
{
    return std::vector<BTDevice>();
}

std::vector<BTDevice> BTControl::getAvailableDevices()
{
    return std::vector<BTDevice>();
}

void BTControl::on_device_found(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTControl::on_device_disappeared(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTControl::on_device_changed(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTControl::on_device_properties_changed(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTControl::on_device_acquired(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTControl::on_device_lost(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTControl::on_device_error(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTControl::on_device_unhandled(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTControl::on_device_unhandled_error(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

//////////////////////////////////////////////////////////////////////////////////

BTServices::BTServices()
{
}

BTServices::~BTServices()
{
}

void BTServices::on_characteristic_read(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTServices::on_characteristic_write(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTServices::on_characteristic_notify(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTServices::on_characteristic_changed(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTServices::on_characteristic_value_changed(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTServices::on_characteristic_properties_changed(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTServices::on_characteristic_acquired(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTServices::on_characteristic_lost(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTServices::on_characteristic_error(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTServices::on_characteristic_unhandled(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

void BTServices::on_characteristic_unhandled_error(GDBusConnection* connection, const gchar* sender_name, const gchar* object_path, const gchar* interface_name, const gchar* signal_name, GVariant* parameters, gpointer user_data)
{
}

//////////////////////////////////////////////////////////////////////////////////

BTManager::BTManager()
{
    this->loopThread = std::jthread([&]() {
        this->gloop = g_main_loop_new(NULL, FALSE);
    });

    this->control = new BTControl();
    this->services = new BTServices();
    
}

BTManager::~BTManager()
{
    g_main_loop_quit(this->gloop);
    delete this->control;
    delete this->services;
}
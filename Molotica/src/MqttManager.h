#pragma once

#include "Core.h"
#include <WiFi.h>
#include <PubSubClient.h>

#undef MQTT_CONNECTED
#undef MQTT_DISCONNECTED
#undef MQTT_CONNECTION_TIMEOUT
#undef MQTT_CONNECTION_LOST
#undef MQTT_CONNECT_FAILED
#undef MQTT_CONNECT_BAD_PROTOCOL
#undef MQTT_CONNECT_BAD_CLIENT_ID
#undef MQTT_CONNECT_UNAVAILABLE
#undef MQTT_CONNECT_BAD_CREDENTIALS
#undef MQTT_CONNECT_UNAUTHORIZED

class MqttManager {
public:
    MqttManager(StateMachine* sm, ErrorHandler* err, MessageBuffer* msg_buf);
    bool init();
    bool connect();
    bool disconnect();
    bool publish(const char* topic, const char* data, bool retain = false);
    bool subscribe(const char* topic);
    bool isConnected();
    bool publishStatus(const char* status);
    bool publishSensorData(const SensorReading& reading);
    bool publishAlert(const AlertMessage& alert);
    bool publishLog(const char* message);
    void processIncomingCommand(const char* data, int data_len);
    void update();
    void getDeviceMac(char* mac_out, size_t max_len);
private:
    StateMachine* m_sm;
    ErrorHandler* m_err;
    MessageBuffer* m_msg_buf;
    WiFiClient m_wifi_client;
    PubSubClient m_mqtt_client;
    bool m_connected;
    unsigned long m_last_reconnect;
    unsigned long m_reconnect_delay;
    char m_mac[18];
    void handleConnected();
    void handleDisconnected();
    void handleData(const char* topic, const uint8_t* data, unsigned int len);
    void subscribeToCommandTopic();
    static void mqttCallback(char* topic, uint8_t* payload, unsigned int len);
    static MqttManager* s_instance;
};

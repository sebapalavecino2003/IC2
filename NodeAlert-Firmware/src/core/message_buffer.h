#pragma once

#include <cstdint>
#include <cstring>

#define MQTT_BUFFER_CAPACITY    20
#define MQTT_MESSAGE_MAX_LEN    256

struct MqttMessage {
    char        topic[64];
    char        payload[MQTT_MESSAGE_MAX_LEN];
    int         qos;
    uint32_t    enqueue_ms;
};

class MessageBuffer {
public:
    MessageBuffer();

    bool enqueue(const char* topic, const char* payload, int qos);
    bool dequeue(MqttMessage& msg);

    int  count() const;
    bool isEmpty() const;
    void clear();

private:
    MqttMessage m_messages[MQTT_BUFFER_CAPACITY];
    int         m_head;
    int         m_tail;
    int         m_count;
};

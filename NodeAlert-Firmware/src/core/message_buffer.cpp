#include "message_buffer.h"
#include "esp_timer.h"

MessageBuffer::MessageBuffer()
    : m_head(0)
    , m_tail(0)
    , m_count(0)
{
}

bool MessageBuffer::enqueue(const char* topic, const char* payload, int qos)
{
    if (m_count == MQTT_BUFFER_CAPACITY) {
        m_head = (m_head + 1) % MQTT_BUFFER_CAPACITY;
        m_count--;
    }

    MqttMessage& slot = m_messages[m_tail];

    size_t tlen = strlen(topic);
    if (tlen >= sizeof(slot.topic)) {
        tlen = sizeof(slot.topic) - 1;
    }
    memcpy(slot.topic, topic, tlen);
    slot.topic[tlen] = '\0';

    size_t plen = strlen(payload);
    if (plen >= sizeof(slot.payload)) {
        plen = sizeof(slot.payload) - 1;
    }
    memcpy(slot.payload, payload, plen);
    slot.payload[plen] = '\0';

    slot.qos = qos;
    slot.enqueue_ms = (uint32_t)(esp_timer_get_time() / 1000);

    m_tail = (m_tail + 1) % MQTT_BUFFER_CAPACITY;
    m_count++;

    return true;
}

bool MessageBuffer::dequeue(MqttMessage& msg)
{
    if (m_count == 0) {
        return false;
    }

    msg = m_messages[m_head];
    m_head = (m_head + 1) % MQTT_BUFFER_CAPACITY;
    m_count--;

    return true;
}

int MessageBuffer::count() const
{
    return m_count;
}

bool MessageBuffer::isEmpty() const
{
    return m_count == 0;
}

void MessageBuffer::clear()
{
    m_head  = 0;
    m_tail  = 0;
    m_count = 0;
}

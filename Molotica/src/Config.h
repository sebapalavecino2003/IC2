#pragma once

#include <Arduino.h>

/* ------------------------------------------------------------------------ */
/* Pin Definitions                                                          */
/* ------------------------------------------------------------------------ */
#define DHT22_PIN           4
#define MQ9_PIN             34
#define KY026_PIN           35
#define BUZZER_PIN          16
#define PIN_LED             2
#define MQ9_HEATER_PIN      25
#define MQ9_DIGITAL_PIN     26
#define KY026_DIGITAL_PIN   27

/* ------------------------------------------------------------------------ */
/* WiFi                                                                     */
/* ------------------------------------------------------------------------ */
#define WIFI_SSID           "IC-2.4G"
#define WIFI_PASS           "1nf0rm4t1c4_2025"
#define WIFI_MAX_RETRIES    20
#define WIFI_RETRY_MS       500

/* ------------------------------------------------------------------------ */
/* MQTT                                                                     */
/* ------------------------------------------------------------------------ */
#define MQTT_BROKER_IP      "10.1.1.10"
#define MQTT_BROKER_PORT    1883
#define MQTT_TOPIC_PREFIX   "nodealert"
#define MQTT_KEEPALIVE_S    60
#define MQTT_CLEAN_SESSION  true
#define MQTT_RECONNECT_MS   3000
#define MQTT_RECONNECT_MAX  60000

/* ------------------------------------------------------------------------ */
/* Device Identity                                                          */
/* ------------------------------------------------------------------------ */
#ifndef DEVICE_ID
#define DEVICE_ID           "nodealert-01"
#endif

#ifndef MQTT_USER
#define MQTT_USER           ""
#endif

#ifndef MQTT_PASS
#define MQTT_PASS           ""
#endif

/* ------------------------------------------------------------------------ */
/* Sampling                                                                 */
/* ------------------------------------------------------------------------ */
#define SAMPLING_INTERVAL_MS       2000

/* ------------------------------------------------------------------------ */
/* Thresholds (aligned with frontend thresholds)                            */
/* ------------------------------------------------------------------------ */
typedef struct {
    float temp_min, temp_max, humidity_min, humidity_max;
    int gas_threshold, flame_threshold;
} ThresholdConfig;

extern ThresholdConfig g_thresholds;

#define DEFAULT_TEMP_MIN        10.0f
#define DEFAULT_TEMP_MAX        50.0f
#define DEFAULT_HUM_MIN         20.0f
#define DEFAULT_HUM_MAX         90.0f
#define DEFAULT_GAS_THRESHOLD   2200
#define DEFAULT_FLAME_THRESHOLD 2000

/* ------------------------------------------------------------------------ */
/* FreeRTOS                                                                 */
/* ------------------------------------------------------------------------ */
#define SENSOR_TASK_STACK       4096
#define SENSOR_TASK_PRIO        2
#define MQTT_TASK_STACK         8192
#define MQTT_TASK_PRIO          1
#define SENSOR_QUEUE_LENGTH     5
#define BUZZER_LEDC_CHANNEL     0
#define BUZZER_LEDC_TIMER       8

/* ------------------------------------------------------------------------ */
/* FreeRTOS Handle Declarations                                             */
/* ------------------------------------------------------------------------ */
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

extern QueueHandle_t sensorDataQueue;
extern SemaphoreHandle_t configMutex;

/**
 * @file mqtt_config.h
 * @brief MQTT broker connection parameters
 *
 * All constants have #ifndef guards for compile-time override.
 * Broker URI and credentials will be populated in Phase 3.
 */

#pragma once

/* ========================================================================== */
/* MQTT Broker                                                                */
/* ========================================================================== */
#ifndef MQTT_BROKER_URI
#define MQTT_BROKER_URI         ""
#endif

#ifndef MQTT_PORT
#define MQTT_PORT               1883
#endif

#ifndef MQTT_KEEPALIVE
#define MQTT_KEEPALIVE          60
#endif

/* ========================================================================== */
/* MQTT Topics                                                                */
/* ========================================================================== */
#ifndef MQTT_TOPIC_PREFIX
#define MQTT_TOPIC_PREFIX       "nodealert/"
#endif

/* ========================================================================== */
/* MQTT Credentials                                                           */
/* ========================================================================== */
#ifndef MQTT_USER
#define MQTT_USER               ""
#endif

#ifndef MQTT_PASS
#define MQTT_PASS               ""
#endif

/* ========================================================================== */
/* MQTT Device Identity                                                       */
/* ========================================================================== */
#ifndef DEVICE_ID
#define DEVICE_ID               "nodealert-01"
#endif

/**
 * @file pins_config.h
 * @brief Centralized GPIO pin definitions for NodeAlert IoT
 *
 * Maps all sensor and actuator pins to descriptive constants.
 * Modify pin assignments here when hardware configuration changes.
 */

#pragma once

#include "driver/gpio.h"
/* ADC headers will be added when MQ-9/KY-026 drivers need them */

/* ========================================================================== */
/* DHT22 — Temperature and Humidity Sensor                                   */
/* ========================================================================== */
#define PIN_DHT22_DATA          GPIO_NUM_4

/* ========================================================================== */
/* MQ-9 — Gas / Carbon Monoxide Sensor                                       */
/* ========================================================================== */
#define PIN_MQ9_ANALOG          GPIO_NUM_34
#define PIN_MQ9_ADC_CHANNEL     ADC1_CHANNEL_6

/* ========================================================================== */
/* KY-026 — Flame / Fire Sensor                                              */
/* ========================================================================== */
#define PIN_KY026_DIGITAL       GPIO_NUM_5
#define PIN_KY026_ANALOG        GPIO_NUM_35
#define PIN_KY026_ADC_CHANNEL   ADC_CHANNEL_7      // GPIO 35 → ADC1_CH7

/* ========================================================================== */
/* Actuators                                                                  */
/* ========================================================================== */
#define PIN_RELAY_ACTUATOR      GPIO_NUM_2

/* ========================================================================== */
/* I2C (reserved for future expansion)                                        */
/* ========================================================================== */
#define PIN_I2C_SDA             GPIO_NUM_21
#define PIN_I2C_SCL             GPIO_NUM_22

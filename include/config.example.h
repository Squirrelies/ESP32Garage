#ifndef ESP32GARAGE_CONFIG_H
#define ESP32GARAGE_CONFIG_H

// Definitions for user-specific configuration. Rename this file to config.h.

// Your local offset from UTC for the network timestamps.
#define ESP32GARAGE_UTC_OFFSET -6.0
// Whether your time zone observes Daylight Savings Time (DST).
#define ESP32GARAGE_DST_OBSERVED 1

// The amount of time in milliseconds(ms) to wait before retrying the Wifi connection in the event of a failed connection.
#define ESP32GARAGE_WIFI_RETRY_INTERVAL 5000
// Your Wifi network's SSID.
#define ESP32GARAGE_WIFI_SSID "My Wifi SSID"
// Your Wifi network's password.
#define ESP32GARAGE_WIFI_PASS "Abc12345!"

// The hostname to assign this device.
// #define ESP32GARAGE_HOSTNAME "ESP32Garage"

// The OTA password you'd like to use.
#define ESP32GARAGE_OTA_PASS "Abc12345!"

// The OTA port to use.
#define ESP32GARAGE_OTA_PORT 8632

// Whether to log to a remote syslog server.
#define ESP32GARAGE_SYSLOG_ENABLE

#define ESP32GARAGE_LOG_ID_SYS 0
#define ESP32GARAGE_LOG_ID_MAIN 1
#define ESP32GARAGE_LOG_ID_WIFI 2
#define ESP32GARAGE_LOG_ID_TIME 3
#define ESP32GARAGE_LOG_ID_GPIO 4
#define ESP32GARAGE_LOG_ID_OTA 5
#define ESP32GARAGE_LOG_ID_FAUXMO 6

// The syslog server address to send logs to.
#define ESP32GARAGE_SYSLOG_ADDRESS "10.0.0.152"

// The syslog server port to send logs to.
#define ESP32GARAGE_SYSLOG_PORT 514

// The amount of time in milliseconds(ms) to put the GPIO pin HIGH before setting it LOW again.
#define ESP32GARAGE_DEVICE_GPIO_SIGNAL_DURATION 200

// Your device's name as it will appear by default in the Alexa app.
#define ESP32GARAGE_DEVICE_1_NAME "Garage Door"
// Your device's GPIO pin that will be signaled when an event is trigger.
#define ESP32GARAGE_DEVICE_1_GPIO GPIO_NUM_23

// Your device's name as it will appear by default in the Alexa app.
#define ESP32GARAGE_DEVICE_2_NAME "Garage Light 1"
// Your device's GPIO pin that will be signaled when an event is trigger.
#define ESP32GARAGE_DEVICE_2_GPIO GPIO_NUM_22

// Your device's name as it will appear by default in the Alexa app.
#define ESP32GARAGE_DEVICE_3_NAME "Garage Light 2"
// Your device's GPIO pin that will be signaled when an event is trigger.
#define ESP32GARAGE_DEVICE_3_GPIO GPIO_NUM_21

// Your device's reed switch GPIO pin which will indicate whether the door is currently open or closed.
#define ESP32GARAGE_DOOR_REED_SWITCH_GPIO GPIO_NUM_20

#endif

#ifndef ESP32GARAGE_MAIN_H
#define ESP32GARAGE_MAIN_H

// Definitions

// The baud rate of the serial port.
#define ESP32GARAGE_BAUD_RATE 115200
// The address of the NTP server to query.
#define ESP32GARAGE_NTP_ADDRESS "pool.ntp.org"
// The port number to use for the Fauxmo server.
#define ESP32GARAGE_FAUXMO_PORT 80

#define ESP32GARAGE_DOOR_STATE_CLOSED 0
#define ESP32GARAGE_DOOR_STATE_OPEN 1

typedef void (*voidFuncPtr)(void);

// Includes

#include "config.h"
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Elog.h>
#include <WiFi.h>
#include <fauxmoESP.h>
#include <memory>
#include <stdio.h>
#include <string>
#include <time.h>

// Forward declarations

/// @brief Performs a debouncing check and, if we're not bouncing, execute the void function pointer stored within the argument.
/// @param voidFunc A void function pointer.
void DebouncingCheck(void *voidFunc);

/// @brief Executes when the reed switch detects that the door has switched from an open to a closing state.
void GarageDoorClosing();

/// @brief Executes when the reed switch detects that the door has switched from a closed to an opening state.
void GarageDoorOpening();

/// @brief Attempt to initialize wifi and setup network time.
bool TryInitializeWifi();

/// @brief Initialize the Fauxmo framework.
void InitializeFauxmo();

/// @brief Register log id/name for serial and syslog.
/// @param logId The log number.
/// @param logName The log name.
/// @param logLevel The log level.
/// @param facility The log facility.
/// @param logFlags (Optional) The log flags.
/// @param serial (Optional) The log output stream.
void RegisterLogID(const uint8_t logId, const char *logName, const LogLevel logLevel, const LogFacility facility, const LogFlags logFlags = LogFlags::FLAG_NONE, Stream &serial = Serial);

/// @brief The handler for when an OTA update begins.
void OtaOnStartHandler();

/// @brief The handler for when an OTA update ends.
void OtaOnEndHandler();

/// @brief The handler for when an OTA update has made progress.
/// @param progress The progress amount.
/// @param total The total amount.
void OtaOnProgressHandler(unsigned int progress, unsigned int total);

/// @brief The handler for when an OTA update encounters an error.
/// @param error The error code enumeration.
void OtaOnErrorHandler(ota_error_t error);

/// @brief Adds a device to the Fauxmo framework with the given name and GPIO pin.
/// @param deviceName The name for the new device.
/// @param pin The GPIO pin for the new device.
/// @return The device id of the newly added device.
uint8_t AddFauxmoDevice(const char *deviceName, const uint8_t pin);

/// @brief Handles an incoming event in the Fauxmo framework.
/// @param device_id The device's id that the event belongs to.
/// @param device_name The device's name that the event belongs to.
/// @param state The event's state. This usuaully indicates an ON or OFF state.
/// @param value The event's value. This usually corresponds to the brightness value.
void HandleFauxmoOnSetStateEvent(unsigned char device_id, const char *device_name, bool state, unsigned char value);

/// @brief Prints information about the current system to standard output.
void PrintSystemInfo();

/// @brief Checks Wifi connection status.
/// @param wifiStatus A pointer to a wl_status_t structure containing the status of the Wifi adapter.
/// @return True if we're connected to a network or False if we're not connected to a network.
bool IsWifiConnected(wl_status_t *wifiStatus);

/// @brief Try to connect to a Wifi network with the given credentials.
/// @param wifiSSID The Wifi SSID to connect to.
/// @param wifiPassword The Wifi Password to use.
/// @param retryConnection Whether to retry the connection in certain cases such as disconnection or other transient errors.
/// @return Whether the connection succeeded or failed. Certain Wifi states will cause this method to wait and retry if retryConnection is true.
bool TryConnectWifi(const char *wifiSSID, const char *wifiPassword, const bool retryConnection = true);

/// @brief Initialize the network time protocol.
/// @param utcOffset The offset from UTC we're currently in.
/// @param isDSTObserved Whether Daylight Savings Time (DST) is observed.
/// @param ntpAddress The NTP address to query.
void InitializeNetworkTime(float utcOffset, bool isDSTObserved, const char *ntpAddress);

/// @brief Signals a GPIO pin for the given duration.
/// @param gpioPin The GPIO pin to signal.
/// @param duration The amount of time in milliseconds(ms) to signal for.
void SignalGPIOPin(uint8_t gpioPin, uint32_t duration);

#endif

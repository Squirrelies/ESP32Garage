#ifndef NEONBLUESP32_NEONBLUESP32_H
#define NEONBLUESP32_NEONBLUESP32_H

// Includes

#include <WiFi.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// Forward declarations

/// @brief Sends a printf to standard output but with the section prepended.
/// @param section The section to prepend to printf.
/// @param format The printf format string.
/// @param ... The printf arguments.
void PrintFormat(const char *section, const char *format, ...);

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

/// @brief Retrieves the current date/time from the network time protocol formatted as YYYY-MM-dd HH\:mm:ss.
/// @return The current date/time from the NTP server or NULL if there was a failure.
const char *GetNetworkTimeString();

/// @brief Signals a GPIO pin for the given duration.
/// @param gpioPin The GPIO pin to signal.
/// @param duration The amount of time in milliseconds(ms) to signal for.
void SignalGPIOPin(uint8_t gpioPin, uint32_t duration);

#endif

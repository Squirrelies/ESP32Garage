#ifndef ESP32GARAGE_MAIN_H
#define ESP32GARAGE_MAIN_H

// Includes

#include "NeonbluESP32.h"
#include "config.h"
#include <fauxmoESP.h>

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

// Forward declarations

/// @brief Performs a debouncing check and, if we're not bouncing, execute the void function pointer stored within the argument.
/// @param voidFunc A void function pointer.
void IRAM_ATTR DebouncingCheck(void *voidFunc);

/// @brief Executes when the reed switch detects that the door has switched from an open to a closing state.
void IRAM_ATTR GarageDoorClosing();

/// @brief Executes when the reed switch detects that the door has switched from a closed to an opening state.
void IRAM_ATTR GarageDoorOpening();

/// @brief Initialize the Fauxmo framework.
void InitializeFauxmo();

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

#endif

#include "main.h"

wl_status_t wifiStatus;
fauxmoESP fauxmo;
bool garageDoorOpen; // Closed/'Off' = false (0), Open/'On' = true (1).

void setup()
{
	Serial.begin(ESP32GARAGE_BAUD_RATE);
	delay(1000);
	PrintSystemInfo();

	// Attempt to connect to Wifi and get network time.
	while (!TryInitializeWifi())
	{
		// Add a 2500ms delay between initial wifi initialization so we're not hammering the CPU as bad.
		delay(2500);
	}

	// Initialize OTA.
	ArduinoOTA.setPassword(ESP32GARAGE_OTA_PASS);
	ArduinoOTA
	    .onStart(OtaOnStartHandler)
	    .onEnd(OtaOnEndHandler)
	    .onProgress(OtaOnProgressHandler)
	    .onError(OtaOnErrorHandler);

	// Start OTA.
	ArduinoOTA.begin();

	// Set LED pin mode
	pinMode(LED_BUILTIN, OUTPUT);

	// Setup the reed switch and interrupts.
	PrintFormat("Main", "Assigning pin %d for %s... ", "Garage Door Status", ESP32GARAGE_DOOR_REED_SWITCH_GPIO);
	pinMode(ESP32GARAGE_DOOR_REED_SWITCH_GPIO, INPUT_PULLUP);
	garageDoorOpen = digitalRead(ESP32GARAGE_DOOR_REED_SWITCH_GPIO) == HIGH ? ESP32GARAGE_DOOR_STATE_OPEN : ESP32GARAGE_DOOR_STATE_CLOSED;
	attachInterruptArg(ESP32GARAGE_DOOR_REED_SWITCH_GPIO, &DebouncingCheck, (void *)&GarageDoorClosing, FALLING);
	attachInterruptArg(ESP32GARAGE_DOOR_REED_SWITCH_GPIO, &DebouncingCheck, (void *)&GarageDoorOpening, RISING);
	PrintFormat(NULL, "done!\n");

	// Initialize Fauxmo.
	InitializeFauxmo();
}

unsigned long reedSwitchTime = 0UL;
unsigned long lastReedSwitchTime = 0UL;
void IRAM_ATTR DebouncingCheck(void *voidFunc)
{
	reedSwitchTime = millis();
	if (reedSwitchTime - lastReedSwitchTime > ESP32GARAGE_DEVICE_GPIO_SIGNAL_DURATION)
	{
		lastReedSwitchTime = reedSwitchTime;
		((voidFuncPtr)voidFunc)();
	}
}

void IRAM_ATTR GarageDoorClosing()
{
	garageDoorOpen = ESP32GARAGE_DOOR_STATE_CLOSED;
}

void IRAM_ATTR GarageDoorOpening()
{
	garageDoorOpen = ESP32GARAGE_DOOR_STATE_OPEN;
}

void loop()
{
	// Handle only if we're connected to Wifi. Otherwise wait and try again next loop.
	if (IsWifiConnected(&wifiStatus))
	{
		// Handle incoming OTA updates. If no events are pending, this returns immediately.
		ArduinoOTA.handle();

		// Handle incoming Fauxmo events. If no events are pending, this returns immediately.
		fauxmo.handle();

		// Add a 16ms delay between loops so we're not hammering the CPU as bad.
		delay(16);
	}
	else
	{
		// Stop servers while we reconnect.
		PrintFormat("OTA", "Stopping OTA...\n");
		ArduinoOTA.end();
		PrintFormat("Fauxmo", "Stopping Fauxmo...\n");
		fauxmo.enable(false);

		// Re-connect.
		if (TryInitializeWifi())
		{
			// Start the servers if we succeeded in reconnecting.
			PrintFormat("OTA", "Starting OTA...\n");
			ArduinoOTA.begin();
			PrintFormat("Fauxmo", "Starting Fauxmo...\n");
			fauxmo.enable(true);
		}
	}
}

bool TryInitializeWifi()
{
	// Connect to Wifi.
	PrintFormat("Wifi", "Connecting to Wifi... ");
	if (TryConnectWifi(ESP32GARAGE_WIFI_SSID, ESP32GARAGE_WIFI_PASS))
	{
		PrintFormat(NULL, "success!\n");

		// Initialize and get network time.
		PrintFormat("Time", "Getting current time...\n");
		InitializeNetworkTime(ESP32GARAGE_UTC_OFFSET, ESP32GARAGE_DST_OBSERVED, ESP32GARAGE_NTP_ADDRESS);

		return true;
	}
	else
	{
		PrintFormat(NULL, "failed!\n");
		return false;
	}
}

void InitializeFauxmo()
{
	PrintFormat("Fauxmo", "Initializing Fauxmo...\n");
	PrintFormat("Fauxmo", "Setting Fauxmo port to %d.\n", ESP32GARAGE_FAUXMO_PORT);
	fauxmo.setPort(ESP32GARAGE_FAUXMO_PORT);
	PrintFormat("Fauxmo", "Adding devices...\n");
#if defined(ESP32GARAGE_DEVICE_1_NAME) && defined(ESP32GARAGE_DEVICE_1_GPIO)
	AddFauxmoDevice(ESP32GARAGE_DEVICE_1_NAME, ESP32GARAGE_DEVICE_1_GPIO);
#endif
#if defined(ESP32GARAGE_DEVICE_2_NAME) && defined(ESP32GARAGE_DEVICE_2_GPIO)
	AddFauxmoDevice(ESP32GARAGE_DEVICE_2_NAME, ESP32GARAGE_DEVICE_2_GPIO);
#endif
#if defined(ESP32GARAGE_DEVICE_3_NAME) && defined(ESP32GARAGE_DEVICE_3_GPIO)
	AddFauxmoDevice(ESP32GARAGE_DEVICE_3_NAME, ESP32GARAGE_DEVICE_3_GPIO);
#endif
	PrintFormat("Fauxmo", "Registering OnSetState event handler callback...\n");
	fauxmo.onSetState(HandleFauxmoOnSetStateEvent);
	PrintFormat("Fauxmo", "Starting Fauxmo...\n");
	fauxmo.enable(true);
}

void OtaOnStartHandler()
{
	std::string type;
	if (ArduinoOTA.getCommand() == U_FLASH)
		type = "sketch";
	else // U_SPIFFS
		type = "filesystem";

	// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
	PrintFormat("OTA", "Update starting via %s...\n", type);
}

void OtaOnEndHandler()
{
	PrintFormat("OTA", "Update complete. Rebooting...");
	delay(1000);
	ESP.restart();
}

void OtaOnProgressHandler(unsigned int progress, unsigned int total)
{
	PrintFormat(NULL, "Progress: %u%%\r", (progress / (total / 100)));
}

void OtaOnErrorHandler(ota_error_t error)
{
	PrintFormat("OTA", "Error[%u]: ", error);
	if (error == OTA_AUTH_ERROR)
		PrintFormat(NULL, "Auth failed.\n");
	else if (error == OTA_BEGIN_ERROR)
		PrintFormat(NULL, "Begin failed.\n");
	else if (error == OTA_CONNECT_ERROR)
		PrintFormat(NULL, "Connect failed.\n");
	else if (error == OTA_RECEIVE_ERROR)
		PrintFormat(NULL, "Receive failed.\n");
	else if (error == OTA_END_ERROR)
		PrintFormat(NULL, "End failed.\n");
}

uint8_t AddFauxmoDevice(const char *deviceName, const uint8_t pin)
{
	PrintFormat("Fauxmo", "Adding device named %s on pin %d... ", deviceName, pin);
	pinMode(pin, OUTPUT);
	uint8_t deviceId = fauxmo.addDevice(deviceName);
	PrintFormat(NULL, "done! Device Id: %d.\n", deviceId);
	return deviceId;
}

void HandleFauxmoOnSetStateEvent(unsigned char device_id, const char *device_name, bool state, unsigned char value)
{
	PrintFormat("Fauxmo", "Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

#if defined(ESP32GARAGE_DEVICE_1_NAME) && defined(ESP32GARAGE_DEVICE_1_GPIO)
	if (strcmp(device_name, ESP32GARAGE_DEVICE_1_NAME) == 0)
	{
		if (state != garageDoorOpen)
		{
			PrintFormat("Fauxmo", "Device #%d (%s) is currently %s and we're %s now...\n", device_id, device_name, garageDoorOpen ? "open" : "closed", state ? "opening" : "closing");
			SignalGPIOPin(ESP32GARAGE_DEVICE_1_GPIO, ESP32GARAGE_DEVICE_GPIO_SIGNAL_DURATION);
		}
		else
		{
			PrintFormat("Fauxmo", "Device #%d (%s) is currently %s but we're already %s.\n", device_id, device_name, garageDoorOpen ? "open" : "closed", state ? "open" : "closed");
		}
	}
#endif

#if defined(ESP32GARAGE_DEVICE_2_NAME) && defined(ESP32GARAGE_DEVICE_2_GPIO)
	if (strcmp(device_name, ESP32GARAGE_DEVICE_2_NAME) == 0)
		SignalGPIOPin(ESP32GARAGE_DEVICE_2_GPIO, ESP32GARAGE_DEVICE_GPIO_SIGNAL_DURATION);
#endif

#if defined(ESP32GARAGE_DEVICE_3_NAME) && defined(ESP32GARAGE_DEVICE_3_GPIO)
	if (strcmp(device_name, ESP32GARAGE_DEVICE_3_NAME) == 0)
		SignalGPIOPin(ESP32GARAGE_DEVICE_3_GPIO, ESP32GARAGE_DEVICE_GPIO_SIGNAL_DURATION);
#endif
}

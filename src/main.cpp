#include "main.h"
#include <esp_chip_info.h>
#include <esp_flash.h>

wl_status_t wifiStatus;
fauxmoESP fauxmo;
bool isTimeInitialized = false;
// bool garageDoorOpen; // Closed/'Off' = false (0), Open/'On' = true (1).

void setup()
{
	Serial.begin(ESP32GARAGE_BAUD_RATE);
#ifdef ESP32GARAGE_SYSLOG_ENABLE
	logger.configureSyslog(ESP32GARAGE_SYSLOG_ADDRESS, ESP32GARAGE_SYSLOG_PORT, ESP32GARAGE_HOSTNAME);
#endif
	RegisterLogID(ESP32GARAGE_LOG_ID_SYS, "Sys", LogLevel::DEBUG, LogFacility::FAC_USER, LogFlags::FLAG_SERVICE_LONG);
	RegisterLogID(ESP32GARAGE_LOG_ID_MAIN, "Main", LogLevel::DEBUG, LogFacility::FAC_USER, LogFlags::FLAG_SERVICE_LONG);
	RegisterLogID(ESP32GARAGE_LOG_ID_WIFI, "Wifi", LogLevel::DEBUG, LogFacility::FAC_USER, LogFlags::FLAG_SERVICE_LONG);
	RegisterLogID(ESP32GARAGE_LOG_ID_TIME, "Time", LogLevel::DEBUG, LogFacility::FAC_USER, LogFlags::FLAG_SERVICE_LONG);
	RegisterLogID(ESP32GARAGE_LOG_ID_GPIO, "GPIO", LogLevel::DEBUG, LogFacility::FAC_USER, LogFlags::FLAG_SERVICE_LONG);
	RegisterLogID(ESP32GARAGE_LOG_ID_OTA, "OTA", LogLevel::DEBUG, LogFacility::FAC_USER, LogFlags::FLAG_SERVICE_LONG);
	RegisterLogID(ESP32GARAGE_LOG_ID_FAUXMO, "Fauxmo", LogLevel::DEBUG, LogFacility::FAC_USER, LogFlags::FLAG_SERVICE_LONG);
	delay(1000);
	PrintSystemInfo();

	// Attempt to connect to Wifi and get network time.
	while (!TryInitializeWifi())
	{
		// Add a 2500ms delay between initial wifi initialization so we're not hammering the CPU as bad.
		delay(2500);
	}

	// Initialize OTA.
	ArduinoOTA.setMdnsEnabled(false); // Unreliable. Ref: https://github.com/espressif/arduino-esp32/issues/4406#issuecomment-989011697
	ArduinoOTA.setPort(ESP32GARAGE_OTA_PORT);
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
	// logger.log(ESP32GARAGE_LOG_ID_MAIN, LogLevel::INFO, "Assigning pin %d for %s...", ESP32GARAGE_DOOR_REED_SWITCH_GPIO, "Garage Door Status");
	// pinMode(ESP32GARAGE_DOOR_REED_SWITCH_GPIO, INPUT_PULLUP);
	// garageDoorOpen = digitalRead(ESP32GARAGE_DOOR_REED_SWITCH_GPIO) == HIGH ? ESP32GARAGE_DOOR_STATE_OPEN : ESP32GARAGE_DOOR_STATE_CLOSED;
	// attachInterruptArg(ESP32GARAGE_DOOR_REED_SWITCH_GPIO, &DebouncingCheck, (void *)&GarageDoorClosing, FALLING);
	// attachInterruptArg(ESP32GARAGE_DOOR_REED_SWITCH_GPIO, &DebouncingCheck, (void *)&GarageDoorOpening, RISING);
	// logger.log(ESP32GARAGE_LOG_ID_MAIN, LogLevel::INFO, "Assigned pin %d for %s successfully!", ESP32GARAGE_DOOR_REED_SWITCH_GPIO, "Garage Door Status");

	// Initialize Fauxmo.
	InitializeFauxmo();
}

void loop()
{
	// logger.log(ESP32GARAGE_LOG_ID_MAIN, LogLevel::DEBUG, "loop() called.");

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
		logger.log(ESP32GARAGE_LOG_ID_OTA, LogLevel::INFO, "Stopping OTA...");
		ArduinoOTA.end();
		logger.log(ESP32GARAGE_LOG_ID_FAUXMO, LogLevel::INFO, "Stopping Fauxmo...");
		fauxmo.enable(false);

		// Re-connect.
		if (TryInitializeWifi())
		{
			// Start the servers if we succeeded in reconnecting.
			logger.log(ESP32GARAGE_LOG_ID_OTA, LogLevel::INFO, "Starting OTA...");
			ArduinoOTA.begin();
			logger.log(ESP32GARAGE_LOG_ID_FAUXMO, LogLevel::INFO, "Starting Fauxmo...");
			fauxmo.enable(true);
		}
	}
}

// unsigned long reedSwitchTime = 0UL;
// unsigned long lastReedSwitchTime = 0UL;
// void IRAM_ATTR DebouncingCheck(void *voidFunc)
//{
//	reedSwitchTime = millis();
//	if (reedSwitchTime - lastReedSwitchTime > ESP32GARAGE_DEVICE_GPIO_SIGNAL_DURATION)
//	{
//		lastReedSwitchTime = reedSwitchTime;
//		((voidFuncPtr)voidFunc)();
//	}
// }

// void IRAM_ATTR GarageDoorClosing()
//{
//  garageDoorOpen = ESP32GARAGE_DOOR_STATE_CLOSED;
//}

// void IRAM_ATTR GarageDoorOpening()
//{
//  garageDoorOpen = ESP32GARAGE_DOOR_STATE_OPEN;
//}

bool TryInitializeWifi()
{
	// Connect to Wifi.
	logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::INFO, "Connecting to Wifi...");
	if (TryConnectWifi(ESP32GARAGE_WIFI_SSID, ESP32GARAGE_WIFI_PASS))
	{
		logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::INFO, "Connected!");

		// Initialize and get network time.
		logger.log(ESP32GARAGE_LOG_ID_TIME, LogLevel::INFO, "Getting current time...");
		InitializeNetworkTime(ESP32GARAGE_UTC_OFFSET, ESP32GARAGE_DST_OBSERVED, ESP32GARAGE_NTP_ADDRESS);

		return true;
	}
	else
	{
		logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::ERROR, "Failed to connect!");
		return false;
	}
}

void InitializeFauxmo()
{
	logger.log(ESP32GARAGE_LOG_ID_FAUXMO, LogLevel::INFO, "Initializing Fauxmo...");

	logger.log(ESP32GARAGE_LOG_ID_FAUXMO, LogLevel::INFO, "Setting Fauxmo port to %d.", ESP32GARAGE_FAUXMO_PORT);
	fauxmo.setPort(ESP32GARAGE_FAUXMO_PORT);

	logger.log(ESP32GARAGE_LOG_ID_FAUXMO, LogLevel::INFO, "Adding devices...");
#if defined(ESP32GARAGE_DEVICE_1_NAME) && defined(ESP32GARAGE_DEVICE_1_GPIO)
	AddFauxmoDevice(ESP32GARAGE_DEVICE_1_NAME, ESP32GARAGE_DEVICE_1_GPIO);
#endif
#if defined(ESP32GARAGE_DEVICE_2_NAME) && defined(ESP32GARAGE_DEVICE_2_GPIO)
	AddFauxmoDevice(ESP32GARAGE_DEVICE_2_NAME, ESP32GARAGE_DEVICE_2_GPIO);
#endif
#if defined(ESP32GARAGE_DEVICE_3_NAME) && defined(ESP32GARAGE_DEVICE_3_GPIO)
	AddFauxmoDevice(ESP32GARAGE_DEVICE_3_NAME, ESP32GARAGE_DEVICE_3_GPIO);
#endif

	logger.log(ESP32GARAGE_LOG_ID_FAUXMO, LogLevel::INFO, "Registering OnSetState event handler callback...");
	fauxmo.onSetState(HandleFauxmoOnSetStateEvent);

	logger.log(ESP32GARAGE_LOG_ID_FAUXMO, LogLevel::INFO, "Starting Fauxmo...");
	fauxmo.enable(true);
}

void RegisterLogID(const uint8_t logId, const char *logName, const LogLevel logLevel, const LogFacility facility, const LogFlags logFlags, Stream &serial)
{
#ifdef ESP32GARAGE_SYSLOG_ENABLE
	logger.registerSyslog(logId, logLevel, facility, logName);
#endif
	logger.registerSerial(logId, logLevel, logName, serial, logFlags);
}

unsigned long progressTime = 0UL;
unsigned long lastProgressTime = 0UL;
void OtaOnStartHandler()
{
	std::string type;
	if (ArduinoOTA.getCommand() == U_FLASH)
		type = "sketch";
	else // U_SPIFFS
		type = "filesystem";

	// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
	logger.log(ESP32GARAGE_LOG_ID_OTA, LogLevel::INFO, "Update starting via %s...", type.c_str());
}

void OtaOnEndHandler()
{
	progressTime = 0UL;
	lastProgressTime = 0UL;
	logger.log(ESP32GARAGE_LOG_ID_OTA, LogLevel::INFO, "Update complete. Rebooting...");
}

void OtaOnProgressHandler(unsigned int progress, unsigned int total)
{
	progressTime = millis();
	if (progressTime - lastProgressTime > 500)
	{
		lastProgressTime = progressTime;
		logger.log(ESP32GARAGE_LOG_ID_OTA, LogLevel::INFO, "Progress: %u%%", (progress / (total / 100)));
	}
}

void OtaOnErrorHandler(ota_error_t error)
{
	if (error == OTA_AUTH_ERROR)
		logger.log(ESP32GARAGE_LOG_ID_OTA, LogLevel::INFO, "Error[%u]: Auth failed.", error);
	else if (error == OTA_BEGIN_ERROR)
		logger.log(ESP32GARAGE_LOG_ID_OTA, LogLevel::INFO, "Error[%u]: Begin failed.", error);
	else if (error == OTA_CONNECT_ERROR)
		logger.log(ESP32GARAGE_LOG_ID_OTA, LogLevel::INFO, "Error[%u]: Connect failed.", error);
	else if (error == OTA_RECEIVE_ERROR)
		logger.log(ESP32GARAGE_LOG_ID_OTA, LogLevel::INFO, "Error[%u]: Receive failed.", error);
	else if (error == OTA_END_ERROR)
		logger.log(ESP32GARAGE_LOG_ID_OTA, LogLevel::INFO, "Error[%u]: End failed.", error);
}

uint8_t AddFauxmoDevice(const char *deviceName, const uint8_t pin)
{
	logger.log(ESP32GARAGE_LOG_ID_FAUXMO, LogLevel::INFO, "Adding device named %s on pin %d...", deviceName, pin);
	pinMode(pin, OUTPUT);
	uint8_t deviceId = fauxmo.addDevice(deviceName);
	logger.log(ESP32GARAGE_LOG_ID_FAUXMO, LogLevel::INFO, "Adding device named %s on pin %d success! Device Id: %d.", deviceName, pin, deviceId);
	return deviceId;
}

void HandleFauxmoOnSetStateEvent(unsigned char device_id, const char *device_name, bool state, unsigned char value)
{
	logger.log(ESP32GARAGE_LOG_ID_FAUXMO, LogLevel::INFO, "Device #%d (%s) state: %s value: %d", device_id, device_name, state ? "ON" : "OFF", value);

#if defined(ESP32GARAGE_DEVICE_1_NAME) && defined(ESP32GARAGE_DEVICE_1_GPIO)
	if (strcmp(device_name, ESP32GARAGE_DEVICE_1_NAME) == 0)
		SignalGPIOPin(ESP32GARAGE_DEVICE_1_GPIO, ESP32GARAGE_DEVICE_GPIO_SIGNAL_DURATION);
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

void PrintSystemInfo()
{
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	unsigned major_rev = chip_info.revision / 100;
	unsigned minor_rev = chip_info.revision % 100;
	logger.log(ESP32GARAGE_LOG_ID_SYS, LogLevel::INFO, "This is an %s chip with %d CPU core(s), %s%s%s%s, silicon revision v%d.%d, ",
	           CONFIG_IDF_TARGET,
	           chip_info.cores,
	           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
	           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
	           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
	           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "",
	           major_rev,
	           minor_rev);

	uint32_t flash_size;
	if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
	{
		logger.log(ESP32GARAGE_LOG_ID_SYS, LogLevel::ERROR, "Get flash size failed!.");
		return;
	}
	logger.log(ESP32GARAGE_LOG_ID_SYS, LogLevel::INFO, "%" PRIu32 "MB %s flash", flash_size / (uint32_t)(1024 * 1024), (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
	logger.log(ESP32GARAGE_LOG_ID_SYS, LogLevel::INFO, "Minimum free heap size: %" PRIu32 " bytes", esp_get_minimum_free_heap_size());
	logger.log(ESP32GARAGE_LOG_ID_SYS, LogLevel::INFO, "Sizeof: %s (%d), %s (%d), %s (%d), %s (%d)",
	           "char", sizeof(char),          // 1
	           "short", sizeof(short),        // 2
	           "long", sizeof(long),          // 4
	           "long long", sizeof(long long) // 8
	);
}

bool IsWifiConnected(wl_status_t *wifiStatus)
{
	*wifiStatus = WiFi.status();
	switch (*wifiStatus)
	{
		case WL_IDLE_STATUS:
		{
			logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::INFO, "Adapter is in an idle state.");
			return false;
		}

		case WL_NO_SSID_AVAIL:
		{
			logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::WARNING, "No networks available. This usually happens when the SSID was not found. Check that the SSID is correct and try again.");
			return false;
		}

		case WL_SCAN_COMPLETED:
		{
			logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::INFO, "Scan completed.");
			return false;
		}

		case WL_CONNECTED:
		{
			return true;
		}

		case WL_CONNECT_FAILED:
		{
			logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::ERROR, "Unable to connect! Check your credentials and try again!");
			return false;
		}

		case WL_CONNECTION_LOST:
		{
			logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::WARNING, "Connection lost. Please check signal strength. RSSI: %d.", WiFi.RSSI());
			return false;
		}

		case WL_DISCONNECTED:
		{
			logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::WARNING, "Disconnected! Check your credentials and try again!");
			return false;
		}

		default:
		{
			logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::ERROR, "Unknown status code: %d.", wifiStatus);
			return false;
		}
	}
}

bool TryConnectWifi(const char *wifiSSID, const char *wifiPassword, const bool retryConnection)
{
	logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::INFO, "Setting adapter to station mode.");
	WiFi.mode(WIFI_STA);
	logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::INFO, "Disconnecting from any currently connected networks...");
	WiFi.disconnect();
	delay(100);
	logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::INFO, "Connecting to %s...", wifiSSID);
	WiFi.begin(wifiSSID, wifiPassword);
	delay(2500); // Give the adapter some time to connect before we check the status.
	wl_status_t wifiStatus;
	while (!IsWifiConnected(&wifiStatus))
	{
		switch (wifiStatus)
		{
			case WL_CONNECT_FAILED:
				return false;

			default:
				break;
		}

		if (!retryConnection)
			return false;

		logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::INFO, "Rechecking in 2500ms...");
		delay(2500);
	}

	logger.log(ESP32GARAGE_LOG_ID_WIFI, LogLevel::INFO, "Successfully connected to %s!", wifiSSID);
	return true;
}

void InitializeNetworkTime(float utcOffset, bool isDSTObserved, const char *ntpAddress)
{
	logger.log(ESP32GARAGE_LOG_ID_TIME, LogLevel::INFO, "Initializing NTP with the following parameters: (%f, %d, %s)", utcOffset, isDSTObserved, ntpAddress);
	configTime(utcOffset * 60 * 60, isDSTObserved * 3600, ntpAddress);
	isTimeInitialized = true;
}

void SignalGPIOPin(uint8_t gpioPin, uint32_t duration)
{
	logger.log(ESP32GARAGE_LOG_ID_GPIO, LogLevel::INFO, "Signaling GPIO pin %d for %d milliseconds...", gpioPin, duration);
	digitalWrite(LED_BUILTIN, HIGH);
	digitalWrite(gpioPin, HIGH);
	delay(duration);
	digitalWrite(gpioPin, LOW);
	digitalWrite(LED_BUILTIN, LOW);
}

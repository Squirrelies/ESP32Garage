#include "NeonbluESP32.h"

bool isTimeInitialized = false;

void PrintFormat(const char *section, const char *format, ...)
{
	if (section != NULL)
	{
		std::unique_ptr<std::string> currTime;
		if ((currTime = GetNetworkTimeString())->data() != nullptr)
			printf("[%s] ", currTime->data());
		printf("(%-8s) ", section);
	}
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void PrintSystemInfo()
{
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	PrintFormat("Sys", "This is an %s chip with %d CPU core(s), %s%s%s%s, ",
	            CONFIG_IDF_TARGET,
	            chip_info.cores,
	            (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
	            (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
	            (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
	            (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

	unsigned major_rev = chip_info.revision / 100;
	unsigned minor_rev = chip_info.revision % 100;
	PrintFormat(NULL, "silicon revision v%d.%d, ", major_rev, minor_rev);

	uint32_t flash_size;
	if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
	{
		PrintFormat(NULL, "\n");
		PrintFormat("Sys", "Get flash size failed!\n");
		return;
	}
	PrintFormat(NULL, "%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024), (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
	PrintFormat("Sys", "Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

	PrintFormat("Sys", "Sizeof: %s (%d), %s (%d), %s (%d), %s (%d)\n",
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
			PrintFormat("Wifi", "Adapter is in an idle state.\n");
			return false;
		}

		case WL_NO_SSID_AVAIL:
		{
			PrintFormat("Wifi", "No networks available. This usually happens when the SSID was not found. Check that the SSID is correct and try again.\n");
			return false;
		}

		case WL_SCAN_COMPLETED:
		{
			PrintFormat("Wifi", "Scan completed.\n");
			return false;
		}

		case WL_CONNECTED:
		{
			return true;
		}

		case WL_CONNECT_FAILED:
		{
			PrintFormat("Wifi", "Unable to connect! Check your credentials and try again!\n");
			return false;
		}

		case WL_CONNECTION_LOST:
		{
			PrintFormat("Wifi", "Connection lost. Please check signal strength. RSSI: %d.\n", WiFi.RSSI());
			return false;
		}

		case WL_DISCONNECTED:
		{
			PrintFormat("Wifi", "Disconnected! Check your credentials and try again!\n");
			return false;
		}

		default:
		{
			PrintFormat("Wifi", "Unknown status code: %d.\n", wifiStatus);
			return false;
		}
	}
}

bool TryConnectWifi(const char *wifiSSID, const char *wifiPassword, const bool retryConnection)
{
	PrintFormat("Wifi", "Setting adapter to station mode.\n");
	WiFi.mode(WIFI_STA);
	PrintFormat("Wifi", "Disconnecting from any currently connected networks...\n");
	WiFi.disconnect();
	delay(100);
	PrintFormat("Wifi", "Connecting to %s...\n", wifiSSID);
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

		PrintFormat("Wifi", "Rechecking in 2500ms...\n");
		delay(2500);
	}

	PrintFormat("Wifi", "Successfully connected to %s!\n", wifiSSID);
	return true;
}

void InitializeNetworkTime(float utcOffset, bool isDSTObserved, const char *ntpAddress)
{
	PrintFormat("Time", "Initializing NTP with the following parameters: (%f, %d, %s)\n", utcOffset, isDSTObserved, ntpAddress);
	configTime(utcOffset * 60 * 60, isDSTObserved * 3600, ntpAddress);
	isTimeInitialized = true;
}

std::unique_ptr<std::string> GetNetworkTimeString()
{
	if (!isTimeInitialized)
		return nullptr;

	struct tm timeinfo;
	if (!getLocalTime(&timeinfo))
		return nullptr;

	size_t charsWritten;
	std::unique_ptr<std::string> returnValue = std::make_unique<std::string>(20, '\0'); // YYYY-MM-dd HH:mm:ss (19 + 1)
	if (!(charsWritten = strftime(returnValue->data(), returnValue->size(), "%F %T", &timeinfo)))
		return nullptr;

	returnValue->resize(charsWritten);
	return returnValue;
}

void SignalGPIOPin(uint8_t gpioPin, uint32_t duration)
{
	PrintFormat("GPIO", "Signaling GPIO pin %d for %d milliseconds...\n", gpioPin, duration);
	digitalWrite(LED_BUILTIN, HIGH);
	digitalWrite(gpioPin, HIGH);
	delay(duration);
	digitalWrite(gpioPin, LOW);
	digitalWrite(LED_BUILTIN, LOW);
}

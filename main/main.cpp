/* 
		Minihub Rạng Đông
		TTS
*/

#include "nvs_flash.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "Log.h"

#include <driver/uart.h>
#include <driver/gpio.h>

#include "Util.h"
#include "Wifi.h"
#include "Sntp.h"
#include "Config.h"
#include "Gateway.h"
#include "BleProtocol.h"
#include "ButtonSignal.h"
#include "Led.h"
#include "QrProtocol.h"
#include "i2c_master.h"

extern void gpio_init(void);

extern "C" void app_main(void)
{
	vTaskDelay(pdMS_TO_TICKS(100));
	LOGI("[APP] Startup..");
	LOGI("[APP] Free memory: %d bytes", esp_get_free_heap_size());
	LOGI("[APP] IDF version: %s", esp_get_idf_version());

	esp_log_level_set("*", ESP_LOG_INFO);
	log_set_level(XLOG_DEBUG);

	srand(time(NULL));

	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
		if (err != ESP_OK)
		{
			LOGE("NVS Init error %s", esp_err_to_name(err));
		}
	}

	Wifi::init();
	// Sntp::init(Wifi::WaitConnecting());
	Led_init();

	i2c_master_init();

	for (int i = 0; i < 5; i++)
	{
		SetLedInternet(true);
		SetLedService(true);
		usleep(400000);
		SetLedInternet(false);
		SetLedService(false);
		usleep(400000);
	}
	SetLedService(true);
	// SetLedInternet(true);

	config = new Config();
	config->ReadConfig();

	// buttonSignal = new ButtonSignal();
	// buttonSignal->init();
	gpio_init();

#ifdef CONFIG_BLE_MESH
	bleProtocol = new BleProtocol();
#else
	bleProtocol = new BleProtocol(UART_NUM_1, GPIO_NUM_23, GPIO_NUM_22, 115200);
#endif
	bleProtocol->init();
	bleProtocol->ResetBle();

	// qrProtocol = new QrProtocol(UART_NUM_2, GPIO_NUM_21, GPIO_NUM_20, 115200);

	// qrProtocol->init();
	

	// gpioProtocol = new GPIOProtocol();
	// gpioProtocol->gpio_init();
	
	string mac = Wifi::GetMacAddressHasDot();
	string host = "10.10.10.1";
	int port = 1883;
	string clientId = mac;
	string username = "RD";
	string password = "DD7443285C99DCC2F6F41509C010EC3C5747D71D4FC5CAB9E99826C605653187";
	int keepAlive = 120;

	
	LOGI("mac: %s", mac.c_str());
	gateway = new Gateway(mac, host, port, mac, username, password, keepAlive);
	gateway->init();

// 	gpio_set_direction(GPIO_NUM_7, GPIO_MODE_OUTPUT);
// // 	// gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);
// 	while(1)
// 	{
// 		// LOGE("task test blink led");
// 		SLEEP_MS(1000);
// 		gpio_set_level(GPIO_NUM_7, 0);
// 		SLEEP_MS(1000);
// 		gpio_set_level(GPIO_NUM_7, 1);
// 	}
}

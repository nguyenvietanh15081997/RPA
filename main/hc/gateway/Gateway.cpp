#include "Gateway.h"
#include "Log.h"
#include <unistd.h>
#include <stdio.h>
#include <algorithm>
#include <string.h>
#include <fstream>
#include <iostream>
#include <thread>
#include "json.h"
#include "Util.h"
#include "Wifi.h"
#include "Base64.h"
#include "Config.h"

#ifdef ESP_PLATFORM
#include "Config.h"
#include "Led.h"
#include "esp_spiffs.h"
#endif

#include "QrProtocol.h"
#include "RelayProtocol.h"
#include "i2c_master.h"

Gateway *gateway = NULL;
static bool check_connect_cloud = false;
static uint32_t tick_count_qr_scan = 0;
Gateway::Gateway(string mac, string address, int port, string clientId, string username, string password, int keepalive,
				 string localAddress, int localPort, string localUsername, string localPassword, int localKeepalive)
	: CloudProtocol(mac, address, port, clientId, username, password, keepalive, false),
	  LocalProtocol(mac, localAddress, localPort, mac, localUsername, localPassword, localKeepalive, false),
	  Udp(8181)
{
	this->mac = mac;
	this->id = "";
	this->dormitoryId = "";
	this->refresh_token = "";
	this->ble_addr = 0;
	this->ble_iv_index = 0;
	this->ble_netkey = "";
	this->ble_appkey = "";
	this->ble_devicekey = "";
	this->data = "";
}

Gateway::~Gateway()
{
}

#ifdef ESP_PLATFORM

static void TestSwitchThread(void *data)
{
	Gateway *gateway = (Gateway *)data;
	gateway->TestSwitch();
	vTaskDelete(NULL);
}
#endif

void Gateway::init()
{
	gpio_set_direction(GPIO_NUM_1, GPIO_MODE_OUTPUT);
	// Device::InitDeviceModelList();
	// LocalProtocol::init();
	CloudProtocol::init();
	Udp::init();

	InitUdpMessage();
	// InitMqttMessageDevice();
	// InitMqttMessageGroup();
	// InitMqttMessageRoom();
	// InitMqttMessageRule();
	// InitMqttMessageScene();
	// InitMqttMessageHc();

#ifdef ESP_PLATFORM
	LOGI("Free memory: %d bytes, internal: %d bytes", esp_get_free_heap_size(), esp_get_free_internal_heap_size());
	if (xTaskCreate(TestSwitchThread, "CheckOnline", 5120, this, 7, NULL) != pdPASS)
	{
		LOGE("Failed to create task");
		SetLedService(false);
	}
	vTaskDelay(10);
#else
	thread testSwitchThread(bind(&Gateway::TestSwitch, this));
	testSwitchThread.detach();
#endif
	// LocalConnect();
	CloudConnect();
}

void Gateway::OnCloudConnect(bool isConnected, bool isReconnect)
{
	LOGI("OnCloudConnect: %d", isConnected);

	if (isConnected)
	{
		gpio_set_level(GPIO_NUM_1, 0);
		check_connect_cloud = true;
#ifdef ESP_PLATFORM
		tick_count_qr_scan = xTaskGetTickCount();
#endif
		Util::LedInternet(true);
		// gpioProtocol->set_led_warning(0);
#ifdef ESP_PLATFORM
		SetLedInternet(true);
#endif
	}
	else
	{
		gpio_set_level(GPIO_NUM_1, 1);
		check_connect_cloud = false;
		Util::LedInternet(false);
		// gpioProtocol->set_led_warning(1);
#ifdef ESP_PLATFORM
		if (GetModeLedInternet() != LED_BLINK && GetModeLedInternet() != LED_FLASH)
			SetLedInternet(false);
#endif
	}
}

void Gateway::OnLocalConnect(bool isConnected, bool isReconnect)
{
	LOGI("OnLocalConnect: %d", isConnected);
}

void Gateway::DelDatabase()
{
}

void Gateway::ResetFactory()
{
	LOGI("ResetFactory");
	if (bleProtocol)
	{
		bleProtocol->ResetDelAll();
		// bleProtocol->ResetFactory();
	}
	else
		LOGW("BleProtocol null");

	DelDatabase();
}

int Gateway::RestartBleGw()
{
	LOGW("Restart Gateway Ble");
#ifdef __ANDROID__
	Util::ExecuteCMD("su");
	Util::ExecuteCMD("echo 100 > /sys/class/gpio/export");
	Util::ExecuteCMD("echo out > /sys/class/gpio/gpio100/direction");
	Util::ExecuteCMD("echo 1 > /sys/class/gpio/gpio100/value");
	sleep(1);
	Util::ExecuteCMD("echo 0 > /sys/class/gpio/gpio100/value");
	Util::ExecuteCMD("echo 100 > /sys/class/gpio/unexport");
#elif defined(__OPENWRT__)
	Util::ExecuteCMD("echo '0' > /sys/class/gpio/gpio1/value");
	sleep(1);
	Util::ExecuteCMD("echo '1' > /sys/class/gpio/gpio1/value");
#elif defined(ESP_PLATFORM)
	SetGpioResetGwBle();
#endif
	return CODE_OK;
}

int Gateway::TestSwitch()
{
	while (1)
	{
		LOGI("TestSwitch");
		StartTestPCBA();
		sleep(1);
	}
}

uint16_t Gateway::getBleAddr()
{
	return ble_addr;
}

uint32_t Gateway::getBleIvIndex()
{
	return ble_iv_index;
}

string Gateway::getBleNetKey()
{
	return ble_netkey;
}

string Gateway::getBleAppKey()
{
	return ble_appkey;
}

string Gateway::getBleDeviceKey()
{
	return ble_devicekey;
}

string Gateway::getDormitory()
{
	return dormitoryId;
}

string Gateway::getId()
{
	return id;
}

string Gateway::getVersion()
{
	return version;
}

string Gateway::getName()
{
	return "";
}

string Gateway::getRefreshToken()
{
	return refresh_token;
}

string Gateway::getData()
{
	return data;
}

string Gateway::getMac()
{
	return mac;
}
void Gateway::setBleAddr(uint16_t addr)
{
	this->ble_addr = addr;
}

void Gateway::setBleIvIndex(uint32_t ivIndex)
{
	this->ble_iv_index = ivIndex;
}

void Gateway::setBleNetkey(string netkey)
{
	this->ble_netkey = netkey;
}

void Gateway::setBleAppkey(string appkey)
{
	this->ble_appkey = appkey;
}

void Gateway::setBleDevicekey(string devicekey)
{
	this->ble_devicekey = devicekey;
}

void Gateway::setDormitory(string dormitory)
{
	this->dormitoryId = dormitory;
}

void Gateway::setRefreshToken(string refresh_token)
{
	this->refresh_token = refresh_token;
}
void Gateway::setData(string data)
{
	this->data = data;
}

void Gateway::setId(string id)
{
	this->id = id;
}

void Gateway::setMac(string mac)
{
	this->mac = mac;
}

void Gateway::setVersion(string version)
{
	this->version = version;
}

void Gateway::setName(string name)
{
}

int Gateway::OnRpcSetPwMqttOnline(Json::Value &reqValue, Json::Value &respValue)
{
	if (reqValue.isMember("DATA") && reqValue["DATA"].isObject())
	{
		respValue["CMD"] = "SET_PASSWD_MQTT_ONLINE";
		Json::Value dataJsonRsp = Json::objectValue;
		int status = 0;
		Json::Value dataValue = reqValue["DATA"];
		if (dataValue.isMember("PASSWD") && dataValue["PASSWD"].isString())
		{
			string password = dataValue["PASSWD"].asString();
			string user = "";

#ifdef ESP_PLATFORM
			user = "minihub-" + mac;
#else
			user = "hc-" + mac;
#endif

			if (config->SetClientId(user))
			{
				if (config->SetUsername(user))
				{
					if (config->SetPort(8883))
					{
						if (config->SetPassword(password))
						{
							status = 1;
						}
						else
						{
							status = 0;
						}
					}
					else
					{
						status = 0;
					}
				}
				else
				{
					status = 0;
				}
			}
			else
			{
				status = 0;
			}
			dataJsonRsp["STATUS"] = status;
			respValue["DATA"] = dataJsonRsp;
			return CODE_EXIT;
		}
	}
	return CODE_ERROR;
}

int Gateway::pushDeviceUpdateLocal(Json::Value &dataValue)
{
	return PublishToLocalMessage("deviceUpdate", dataValue, "deviceUpdateRsp", NULL, 0);
}

int Gateway::pushDeviceUpdateCloud(Json::Value &dataValue)
{
	return PublishToCloudMessage("deviceUpdate", dataValue, "deviceUpdateRsp", NULL);
}

int Gateway::pushNewDeviceCloud(Json::Value &dataValue)
{
	return PublishToCloudMessage("newDev", dataValue, "newDevRsp", NULL);
}

int Gateway::pushNewDeviceLocal(Json::Value &dataValue)
{
	return PublishToLocalMessage("newDev", dataValue, "newDevRsp", NULL, 0);
}

int Gateway::pushStartAddHc(Json::Value &dataValue)
{
	return PublishToLocalMessage("startAddHc", dataValue, "startAddHcRsp", NULL, 0);
}

int Gateway::pushStopAddHc(Json::Value &dataValue)
{
	return PublishToLocalMessage("stopAddHc", dataValue, "stopAddHcRsp", NULL, 0);
}

int Gateway::pushNotify(Json::Value &dataValue)
{
	return PublishToLocalMessage("newNotify", dataValue, "newNotifyRsp", NULL, 0);
}

int Gateway::Req_StartTest_BLEMesh(string mac)
{
	Json::Value root;
	Json::Value data;

	root["process"] = "pcba";

	data["mac"] = mac;
	data["cmd"] = "startTestBleMesh";
	data["type"] = "request";

	root["data"] = data;

	string message = root.toString();

	string bc_ip = Wifi::GetBroadcastIP();

	return SendBroadcast(message, bc_ip, Udp::port);
}

int Gateway::GetStatusConnectWifi(string mac)
{
	Json::Value root;
	Json::Value data;

	root["process"] = "pcba";

	data["mac"] = mac;
	data["cmd"] = "wifiReport";
	data["type"] = "request";

	root["data"] = data;

	string message = root.toString();

	string bc_ip = Wifi::GetBroadcastIP();

	return SendBroadcast(message, bc_ip, Udp::port);
}

typedef struct __attribute__((packed))
{
	string macWifi;
	string ssid;
	string rssiWf;
	string macBle;
	string rssiBle;
	uint32_t type;
} wifi_inf_test_pcba_t;

wifi_inf_test_pcba_t inf;
bool hasRspUDP = false;
uint8_t pos = 0;
uint8_t stt_on_pos[4] = {0, 0, 0, 0};
uint8_t stt_off_pos[4] = {0, 0, 0, 0};

int ParseInfPCBA(string s_macWifi, string s_ssid, string s_rssiWf, string s_macBle, string s_rssiBle, uint32_t s_type)
{
	inf.macWifi = s_macWifi;
	inf.ssid = s_ssid;
	inf.rssiWf = s_rssiWf;
	inf.macBle = s_macBle;
	inf.rssiBle = s_rssiBle;
	inf.type = s_type;
	hasRspUDP = true;
	return CODE_OK;
}

int ResetInfPCBA()
{
	inf.macWifi = "";
	inf.ssid = "";
	inf.rssiWf = "";
	inf.macBle = "";
	inf.rssiBle = "";
	inf.type = 0;
	return CODE_OK;
}

// uint16_t getLast4HexAsUint16(const std::string &mac)
// {
// 	if (mac.length() < 4)
// 	{
// 		return 0; // Tránh lỗi nếu chuỗi quá ngắn
// 	}

// 	std::string last4 = mac.substr(mac.length() - 4, 4); // Lấy 4 ký tự cuối
// 	uint16_t addr;

// 	std::stringstream ss;
// 	ss << std::hex << last4; // Chuyển chuỗi thành số hex
// 	ss >> addr;

// 	return addr;
// }

void rd_reporting_proc_ctcu(uint8_t num_ele, uint8_t pos, bool wifi_status, bool ble_status)
{
	Json::Value rs;
	// rs["mac"] = qrProtocol->mac;
	// rs["addr"] = qrProtocol->addr;
	rs["version"] = "1.2";
	rs["serial"] = "000000001-50787D552750";
	rs["deviceType"] = Json::UInt(inf.type);
	// rs["rssi"] = checkRssi ? bleProtocol->rssi : 0;

	rs["wifi"] = wifi_status;
	rs["wifiMac"] = inf.macWifi;
	rs["wifiRssi"] = inf.rssiWf;
	rs["bluetooth"] = ble_status;
	rs["bluetoothMac"] = inf.macBle;
	rs["bluetoothRssi"] = inf.rssiBle;

	rs["pos"] = pos;

	rs["on_relay1"] = (stt_on_pos[0]) ? true : false;
	rs["off_relay1"] = (stt_off_pos[0]) ? false : true;
	if (num_ele > 1)
	{
		rs["on_relay2"] = (stt_on_pos[1]) ? true : false;
		rs["off_relay2"] = (stt_off_pos[1]) ? false : true;
	}
	if (num_ele > 2)
	{
		rs["on_relay3"] = (stt_on_pos[2]) ? true : false;
		rs["off_relay3"] = (stt_off_pos[2]) ? false : true;
	}
	if (num_ele > 3)
	{
		rs["on_relay4"] = (stt_on_pos[3]) ? true : false;
		rs["off_relay4"] = (stt_off_pos[3]) ? false : true;
	}

	Json::Value deviceJson = Json::arrayValue;
	deviceJson.append(rs);
	Json::Value dataPush;
	Json::Value devJson;

	devJson["device"] = deviceJson;
	dataPush["cmd"] = "hcReportLogPcbaSmtModuleDC";
	dataPush["rqi"] = Util::genRandRQI(16);
	dataPush["data"] = devJson;

	LOGE("%s", dataPush.toString().c_str());
	gateway->CloudPublish(dataPush.toString());

	SLEEP_MS(10000);
}

enum
{
	STEP_INIT = 0,
	STEP_GET_INF_POS1,
	STEP_GET_INF_POS2,
	STEP_GET_INF_POS3,
	STEP_CTRL_BLE,
};

void ButtonInit()
{
	gpio_config_t io_conf;
    // Cấu hình chân GPIO làm output
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_NUM_12); // Chọn chân GPIO số 2
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);
    gpio_set_direction(GPIO_NUM_12, GPIO_MODE_OUTPUT);
}

void ButtonSet(bool on)
{
	gpio_set_level(GPIO_NUM_12, on ? 1 : 0);
}

int Gateway::StartTestPCBA()
{
	// SetPowerOnPos(1);
	// for (int i = 0;i < 3; i++)
	// {
	// 	GetSttGroupRelayPos(i);
	// }

	for (int i = 0; i < 3; i++)
	{
		ResetInfPCBA();
		SetPowerOnPos(i);
		SLEEP_MS(5000);

		uint8_t count = 10;
		hasRspUDP = false;
		while (!hasRspUDP && count)
		{
			GetStatusConnectWifi("0");
			SLEEP_MS(1000);
			count--;
		}
		if (count == 0)
		{
			LOGE("GetStatusConnectWifi timeout pos: %d", pos);
			rd_reporting_proc_ctcu(1, i, false, false);
			continue;
		}
		else
		{
			Req_StartTest_BLEMesh("0");
			SLEEP_MS(1000);
			Req_StartTest_BLEMesh("0");
			SLEEP_MS(1000);

			if (inf.type == DEVICE_TYPE_SOCKET_1)
			{
				ButtonInit();
				ButtonSet(true);
				// TODO: Set button On
				int ble_status = bleProtocol->RPA_Test_Socket(0xffff, 1, 1, 1); // ble_status = CODE_OK -> button OK

				// TODO: Read stt Relay + led

				// TODO: Set button On
				ble_status = bleProtocol->RPA_Test_Socket(0xffff, 0, 0, 0); // ble_status = CODE_OK -> button OK

				// TODO: Read stt Relay + led
			}
			else if (inf.type == DEVICE_TYPE_SWITCH_1 || inf.type == DEVICE_TYPE_SWITCH_2 || inf.type == DEVICE_TYPE_SWITCH_3 || inf.type == DEVICE_TYPE_SWITCH_4)
			{
				int ble_status = bleProtocol->ControlRelayOfSwitch(5 + i, 4, 0xFF, 1);
				SLEEP_MS(1500);
				for (int j = 0; j < 3; j++)
				{
					stt_on_pos[j] = (GetSttGroupRelayPos(i) >> j) & 0x01;
					LOGI("stt_on_pos[%d]: %d", j, stt_on_pos[j]);
				}

				ble_status = bleProtocol->ControlRelayOfSwitch(5 + i, 4, 0xFF, 0);
				SLEEP_MS(1500);
				for (int j = 0; j < 3; j++)
				{
					stt_off_pos[j] = (GetSttGroupRelayPos(i) >> j) & 0x01;
					LOGI("stt_off_pos[%d]: %d", j, stt_off_pos[j]);
				}

				rd_reporting_proc_ctcu(3, i, true, ble_status == CODE_OK ? true : false);
			}
			else
			{
				LOGI("Device type: Unknown");
			}
		}
		SLEEP_MS(1000);
	}
	return CODE_OK;
}
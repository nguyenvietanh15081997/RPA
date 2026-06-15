#include "QrProtocol.h"
#include <stdlib.h>
#include <thread>
#include <functional>
#include <byteswap.h>
#include "Log.h"
#include "Util.h"
#include <string.h>
#include <algorithm>
#include "Config.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "Product.h"
#include "CloudProtocol.h"
#include "Gateway.h"
#include "Wifi.h"
static QueueHandle_t qr_code_queue = NULL;
TaskHandle_t qr_code_handle = NULL;
QrProtocol *qrProtocol = NULL;

#ifdef ESP_PLATFORM
QrProtocol::QrProtocol(uart_port_t num, int txPin, int rxPin, int baudrate) : Uart(num, txPin, rxPin, baudrate)
#else
QrProtocol::QrProtocol(char *uartPort, int baudrate) : Uart(uartPort, baudrate, 100000)
#endif
{
	this->mac = "";
	this->prod_code = "";
	this->prod_num = "";
	this->serial = "";
	this->startTest = false;
	this->mac_k9b = config->GetMacK9B();
	this->mac_k9b_int = strtoul(this->mac_k9b.c_str(), NULL, 16);
	LOGI("K9B MAC int: %X", this->mac_k9b_int);
	this->isMac_k9b = false;
	this->time_start_test = 0;
}

QrProtocol::~QrProtocol()
{
}

bool parseQRCode(const std::string &code, std::string &prod_num, std::string &prod_code, std::string &serial)
{
	LOGD("QrProtocol::len: %d", code.length());
	if (code.length() != 30)
		return false; // check length

	prod_num = code.substr(0, 13);
	prod_code = code.substr(13, 8);
	serial = code.substr(21, 9);
	return true;
}

void send_data_cloud(string serial, string dev_type)
{
	Json::Value root;
	root["cmd"] = "hcReportTestLight";
	root["rqi"] = Util::genRandRQI(16);
	; // hoặc Util::genRandRQI(16);

	Json::Value data;
	data["deviceType"] = dev_type; // "12345"
	data["serial"] = serial;	   // "30 ky tu"

	// Chuyển vector<ErrorDevice> thành mảng JSON
	vector<ErrorDevice> errorList = GetErrorList();
	Json::Value errors(Json::arrayValue);
	for (const auto &err : errorList)
	{
		Json::Value errJson;
		errJson["code"] = err.code;
		errJson["name"] = err.name;
		errJson["description"] = err.description;
		errors.append(errJson);
	}
	data["errors"] = errors;

	root["data"] = data;

	LOGE("%s", root.toString().c_str());
	gateway->CloudPublish(root.toString());

	SLEEP_MS(3000);
}

bool starts_with_ssid(const char *input) {
    return input && strncmp(input, "SSID", 4) == 0;
}


bool parse_wifi_info(const char *input, char *ssid, char *password)
{
    const char *ssid_start = input + 5;
    const char *pass_start = strstr(input, ";PASSWORD:");

    if (!pass_start) return false;

    size_t ssid_len = pass_start - ssid_start;
    size_t pass_len = strlen(pass_start + 10);

    strncpy(ssid, ssid_start, ssid_len);
    ssid[ssid_len] = '\0';
    strncpy(password, pass_start + 10, pass_len);
    password[pass_len] = '\0';

    return true;
}

void task_qr_code(void *param)
{
	char *msg;
	while (1)
	{
		if (xQueueReceive(qr_code_queue, &msg, portMAX_DELAY) == pdTRUE)
		{
			LOGI("task_qr_code: %s", msg);

			if(starts_with_ssid(msg))
			{
				char ssid[32]={0};
				char password[64] ={0};
				if(parse_wifi_info(msg,ssid,password))
				{
					Wifi::WifiReStartSta(ssid,password);
					esp_restart();
					SLEEP_MS(1000);
				}
				free(msg);
				continue;
			}
			string code(msg, 30);
			string prod_num;
			string prod_code;
			string serial;
			if (!parseQRCode(code, prod_num, prod_code, serial))
			{
				free(msg);
				continue;
			}
			ProductInfo prod;
			if (!is_product_exist(prod_code, prod))
			{
				free(msg);
				continue;
			}
			send_data_cloud(code, prod.dev_type);
			free(msg);
		}
		SLEEP_MS(100); // Prevent busy loop
	}
}

void QrProtocol::init()
{
	Uart::init();
	qr_code_queue = xQueueCreate(10, sizeof(char *));
	if (xTaskCreate(task_qr_code, "QR_CODE", 5120, NULL, 10, &qr_code_handle) == pdPASS)
	{
		LOGI(TAG, "success active task qr code");
	}
}

std::string extractMac(const std::string &input)
{
	size_t pos = input.find("MAC"); // Tìm vị trí "MAC"
	if (pos != std::string::npos && pos + 15 <= input.length())
	{									  // Đảm bảo có đủ 12 ký tự sau "MAC"
		return input.substr(pos + 3, 12); // Lấy 12 ký tự sau "MAC"
	}
	return "Not found";
}

std::string extractCnNumber(const std::string &input)
{
	size_t pos = input.find("CN.");
	if (pos != std::string::npos && pos + 5 <= input.length())
	{
		return input.substr(pos + 4, 1); // Lấy 2 ký tự sau "CN."
	}
	return ""; // Trả về chuỗi rỗng nếu không tìm thấy
}

uint16_t getLast4HexAsUint16(const std::string &mac)
{
	if (mac.length() < 4)
	{
		return 0; // Tránh lỗi nếu chuỗi quá ngắn
	}

	std::string last4 = mac.substr(mac.length() - 4, 4); // Lấy 4 ký tự cuối
	uint16_t addr;

	std::stringstream ss;
	ss << std::hex << last4; // Chuyển chuỗi thành số hex
	ss >> addr;

	return addr;
}

int countHyphens(const std::string &str)
{
	int count = 0;
	for (char c : str)
	{
		if (c == '-')
		{
			count++;
		}
	}
	return count;
}



int QrProtocol::OnMessage(unsigned char *data, int len)
{
	this->startTest = true;
	if (data[len - 1] != 0x0D)
	{
		return len; // ignore messages without 0x0D at the end
	}
	uint8_t *d = data;
	int l = len - 1;
	std::string s(reinterpret_cast<char *>(d), l);
	LOGD("QrProtocol::OnMessage: %s", s.c_str());
	if (l > 0)
	{
		if(starts_with_ssid((char *)data))
		{
			char *msg = (char *)malloc(sizeof(char) * len + 1);
			memcpy(msg, s.c_str(), sizeof(char) * len);
			msg[len] = '\0'; // Ensure null-termination
			if (xQueueSend(qr_code_queue, &msg, (TickType_t)0) != pdTRUE)
			{
				LOGE("QrProtocol::OnMessage: Failed to send QR code to queue");
				free(msg);
			}
		}
		size_t pos = s.rfind('?');
		if (pos != std::string::npos && pos + 1 < s.length())
		{
			std::string lastPart = s.substr(pos + 1);
			if (lastPart.length() == 30)
			{
				char *msg = (char *)malloc(sizeof(char) * 31);
				memcpy(msg, lastPart.c_str(), sizeof(char) * 30);
				msg[30] = '\0'; // Ensure null-termination
				if (xQueueSend(qr_code_queue, &msg, (TickType_t)0) != pdTRUE)
				{
					LOGE("QrProtocol::OnMessage: Failed to send QR code to queue");
					free(msg);
				}
				else
				{
					LOGI("QrProtocol::OnMessage: QR code sent to queue successfully");
				}
			}
			else
			{
				LOGI("QrProtocol::OnMessage: QR code fail");
			}
		}
		// if (parseQRCode(s, this->prod_num, this->prod_code, this->serial))
		// {
		// 	LOGI("QrProtocol::OnMessage: Product Number: %s", this->prod_num.c_str());
		// 	LOGI("QrProtocol::OnMessage: Product Code: %s", this->prod_code.c_str());
		// 	LOGI("QrProtocol::OnMessage: Serial: %s", this->serial.c_str());
		// 	LOGI("QrProtocol::OnMessage: MAC: %s", this->mac.c_str());
		// 	char *msg = (char *)malloc(sizeof(char) * 31);
		// 	memcpy(msg, data, sizeof(char) * 30);
		// 	msg[30] = '\0'; // Ensure null-termination
		// 	if (xQueueSend(qr_code_queue, &msg, (TickType_t)0) != pdTRUE)
		// 	{
		// 		LOGE("QrProtocol::OnMessage: Failed to send QR code to queue");
		// 		free(msg);
		// 	}
		// 	else
		// 	{
		// 		LOGI("QrProtocol::OnMessage: QR code sent to queue successfully");
		// 	}
		// }
		else
		{
			LOGE("QrProtocol::OnMessage: Invalid QR code");
		}
	}
	return l;
}

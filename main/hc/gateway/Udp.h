#pragma once

#include <functional>
#include "json.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include "ErrorCode.h"

using namespace std;

class Udp
{
private:
	typedef function<int(Json::Value &reqValue, Json::Value &respValue)> OnRpcCallbackFunc;
	map<string, OnRpcCallbackFunc> onRpcCallbackFuncList;

public:
	int fd = -1;
	int port;
	bool isRunning;

	Udp(int port);

	void init();
	void stop();

	int UdpCmdCallbackRegister(string cmd, OnRpcCallbackFunc onRpcCallbackFunc);
	void UdpOnMessage(string message, struct sockaddr_in *si_other, int slen);
	int send(string message, struct sockaddr_in *si_other, int slen);
	int SendTo(const std::string &message, const std::string &ip, uint16_t port);
	int SendBroadcast(const std::string &message, const std::string &broadcast_ip, uint16_t port);
};

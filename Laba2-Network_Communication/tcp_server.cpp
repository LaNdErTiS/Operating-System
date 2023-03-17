#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <windows.h>
#include <winsock2.h>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <vector>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

int init()
{
	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));
}

int sock_err(const char* function, int s)
{
	int err;
	err = WSAGetLastError();
	cout << function << ": socket error: " << err << endl;
	return -1;
}

int set_non_block_mode(int s)
{
	unsigned long mode = 1;
	return ioctlsocket(s, FIONBIO, &mode);
}

void deinit() {
	WSACleanup();
}

struct msg_format {
	int number;
	unsigned short int AA;
	int BBB;
	unsigned char hh;
	unsigned char mm;
	unsigned char ss;
	int size;
	string msg;
};

struct client {
	sockaddr_in addr;
	int socket;
	vector<msg_format> messages;
	int sentOk;
	int col;
};

string to_string_my(unsigned char c) {
	string str;
	if (c < 10) {
		str.push_back('0');
		str.push_back(c + '0');
	}
	else {
		str = to_string((long long)c);
	}
	return str;
}
int print_msg(client client, msg_format msg_protocol) {
	unsigned int netip = client.addr.sin_addr.s_addr;
	unsigned int netport = client.addr.sin_port;
	string AA = to_string((long long)msg_protocol.AA);
	string BBB = to_string((long long)msg_protocol.BBB);
	string hh = to_string_my(msg_protocol.hh);
	string mm = to_string_my(msg_protocol.mm);
	string ss = to_string_my(msg_protocol.ss);
	string message = msg_protocol.msg;
	int port = htons(netport);

	ofstream out;
	out.open("msg.txt", ios_base::out | ios_base::app);
	out << (netip & 0xff) << "." << (netip >> 8 & 0xff) << "." << (netip >> 16 & 0xff) << "." << (netip >> 24 & 0xff) << ":" << port << " ";
	out << AA << " " << BBB << " " << hh << ":" << mm << ":" << ss << " " << message << endl;
	out.close();
	if (message == "stop") {
		return 1;
	}
	return 0;
}

int main(int argc, char * argv[]) {
	if (argc != 2) {
		cout << "Wrong number of args";
		return 0;
	}
	//int port = 9000;
	int port = atoi(argv[1]);
	init();

	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		return sock_err("socket", s);
	}

	int k = 0;
	set_non_block_mode(s);

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		return sock_err("bind", s);
	}

	WSAEVENT events[2];
	events[0] = WSACreateEvent();
	events[1] = WSACreateEvent();

	WSAEventSelect(s, events[0], FD_ACCEPT);

	if (listen(s, 1) < 0) {
		return sock_err("listen", s);
	}

	vector<client> clients;

	k = 0;
	while (!k) {
		WSANETWORKEVENTS ne;
		DWORD dw = WSAWaitForMultipleEvents(1, events, false, 1000, false);
		WSAResetEvent(events[0]);
		WSAResetEvent(events[1]);
		if (0 == WSAEnumNetworkEvents(s, events[0], &ne) && (ne.lNetworkEvents & FD_ACCEPT)) {
			client temp;
			memset(&temp, 0, sizeof(temp));
			int addrlen = sizeof(temp.addr);
			temp.socket = accept(s, (struct sockaddr*) &temp.addr, (int *)&addrlen);
			clients.insert(clients.end(), temp);
			WSAEventSelect(temp.socket, events[1], FD_READ | FD_WRITE | FD_CLOSE);
		}
		for (int i = 0; i < clients.size(); i++) {
			if (0 == WSAEnumNetworkEvents(clients[i].socket, events[1], &ne)) {
				if((ne.lNetworkEvents & FD_READ)) {
					int rcv = 0;
					if (clients[i].messages.size() == 0) {
						char modebuff[3] = { 0 };
						while (rcv < 3)
							rcv += recv(clients[i].socket, modebuff, sizeof(modebuff), 0);
						clients[i].col = 0;
					}
					static unsigned char masString[100][500000] = { 0 };

					static char buff[500000] = { 0 };
					msg_format msg;
					clients[i].messages.push_back(msg);
					int flag = 1; int start = 0;
					while (flag) {
						rcv = recv(clients[i].socket, buff, sizeof(buff), 0);
						if (rcv > 0) {
							//clients[i].messages.push_back(msg);
							for (int q = start, r = 0; q < rcv + start; q++, r++) {
								masString[clients[i].col][q] = buff[r];
							}
							start += rcv;
						}
						else {
							flag = 0;
						}
					}
					if (rcv == 0) {
						clients.erase(clients.begin() + i);
						i = 0;
						continue;
					}
					
					unsigned int num;
					num = masString[clients[i].col][0] * 16777216 + masString[clients[i].col][1] * 65536 + masString[clients[i].col][2] * 256
						+ masString[clients[i].col][3];
					clients[i].messages[num].number = num;

					clients[i].messages[num].AA = (unsigned short)((unsigned char)masString[clients[i].col][4] * 256 +
						(unsigned char)masString[clients[i].col][5]);
					clients[i].messages[num].BBB = (masString[clients[i].col][6] * 16777216) + (masString[clients[i].col][7] * 65536)
						+ (masString[clients[i].col][8] * 256) + (masString[clients[i].col][9]);
					clients[i].messages[num].hh = (unsigned char)masString[clients[i].col][10];
					clients[i].messages[num].mm = (unsigned char)masString[clients[i].col][11];
					clients[i].messages[num].ss = (unsigned char)masString[clients[i].col][12];
					clients[i].messages[num].size = masString[clients[i].col][13] * 16777216 + masString[clients[i].col][14] * 65536 +
						masString[clients[i].col][15] * 256 + masString[clients[i].col][16];
					int zam = clients[i].messages[num].size + 17;
					string res;
					clients[i].messages[num].msg.reserve(zam + 1);
					for (int t = 17; t < zam; t++) {
						clients[i].messages[num].msg.push_back(masString[clients[i].col][t]);
					}

					k = print_msg(clients[i], clients[i].messages[clients[i].col]);
					clients[i].col++;
					int msg_num = clients[i].messages.size();
					for (; clients[i].sentOk < msg_num; clients[i].sentOk++) {
						char buff[2];
						buff[0] = 'o';
						buff[1] = 'k';
						int sent = 0;
						int ret = 0;

						while (sent < 2) {
							ret = send(clients[i].socket, buff, 2, 0);
							if (ret <= 0) {
								return sock_err("send", clients[i].socket);
							}
							sent += ret;
						}
						memset(&buff, 0, sizeof(char) * 2);
					}
					for (int j = 0; j < 10000000; j++) {}
					i--;
				}
			}
		}
	}
	for (int i = 0; i < clients.size(); i++) {
		closesocket(clients[i].socket);
	}
	closesocket(s);
	deinit();
}

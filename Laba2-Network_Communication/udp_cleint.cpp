#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include<fstream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/types.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <vector>
#pragma comment(lib, "ws2_32.lib")
using namespace std;

int sock_err(const char* function, int s)
{
	int err;
	err = errno;
	fprintf(stderr, "%s: socket error: %d\n", function, err);
	return -1;
}

sockaddr_in sock_init(char* server) {
	char* sav = server;
	char* ip = strtok(server, ":");
	char* port = strtok(NULL, ":");

	sockaddr_in sock;
	memset(&sock, 0, sizeof(sock));
	sock.sin_family = AF_INET;
	sock.sin_port = htons(atoi(port));
	sock.sin_addr.s_addr = inet_addr(ip);
	return sock;
}


int str_convert(int number, char* data, char** buf) {
	char *AA = strtok(data, " ");
	char* BBB = strtok(NULL, " ");
	char* time1 = strtok(NULL, " ");
	char* msg = strtok(NULL, "\n");

	if (time1 == NULL || AA == NULL || BBB == NULL || msg == NULL) { return -1; }

	size_t data_size = 4 + 2 + 4 + 3 + 4 + strlen(msg) + 1;
	*buf = (char*)calloc(data_size, sizeof(char));

	char* bytes = *buf;

	int* bytes_num = (int*)bytes;
	bytes_num[0] = htonl(number);
	short* kek = (short*)(bytes + 4);

	kek[0] = htons(atoi(AA));

	bytes_num = (int*)(bytes + 6);
	bytes_num[0] = htonl(atoi(BBB));

	char* h = strtok(time1, ":");
	char* m = strtok(NULL, ":");
	char* s = strtok(NULL, ":");

	bytes[10] = atoi(h);
	bytes[11] = atoi(m);
	bytes[12] = atoi(s);

	bytes_num = (int*)(bytes + 13);
	bytes_num[0] = htonl(strlen(msg));

	strncpy(bytes + 17, msg, strlen(msg));
	return data_size;
}

int init() {
	WSADATA wsa_data;
	return (0 == WSAStartup(MAKEWORD(2, 2), &wsa_data));
}

bool findElem(int index, vector<int> &mas) {
	for (int i = 0; i < mas.size(); i++) {
		if (mas[i] == index)return false;
	}
	mas.insert(mas.end(), index);
	return true;
}

bool isExist(int index, vector<int> &mas) {
	for (int i = 0; i < mas.size(); i++) {
		if (mas[i] == index)return true;
	}
	return false;
}

int main(int argc, char** argv)
{
	init();
	if (argc < 3) return printf("ERR: 3 arguments required\n");
	ifstream f;
	f.open(argv[2]);
	if (!f.is_open()) {
		return printf("ERR: can't open file %s\n", argv[2]);
	}

	int s = 1;
	struct sockaddr_in addr = sock_init(argv[1]);

	// Создание TCP-сокета 
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		return sock_err("socket", s);
	}

	string check;
	vector<string> lines;
	while (getline(f, check, '\n')) {
		if (check.size() >= 13) {
			lines.insert(lines.end(), check);
		}
	}

	f.close();

	int col = lines.size();// < 20 ? lines.size() : 20;

	vector<int> index;
	int t = 0;
	while (col) {
		char buf[1024] = { '\0' };
		char response[4 * 20 * sizeof(char)] = { '\0' };
		unsigned int nums[20] = { 0 };
		struct timeval tv = { 0, 100 * 1000 };
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(s, &fds);
		struct sockaddr_in resaddr;
		socklen_t addrlen = sizeof(resaddr);
		int res, received;

		int j = 0;
		if (t == lines.size()) { t = 0; }
		while (isExist(t, index)) { t++; }
		for (; j < lines[t].size(); j++) {
			buf[j] = lines[t][j];
		}
		buf[j] = '\0';
		char* dat = NULL;
		int s_len = str_convert(t, buf, &dat);
		if (s_len > 0) {
			t++;
			res = sendto(s, dat, s_len, 0, (const sockaddr*)&addr, sizeof(sockaddr));
			if (res < 0) { return sock_err("send", s); }
		}
		int u = select(s + 1, &fds, 0, 0, &tv);
		if (u == 0) continue;
		if (u < 0) { return sock_err("select", s); }
		received = (int)recvfrom(s, response, sizeof(response), 0, (struct sockaddr *) &resaddr, &addrlen);

		if (received <= 0) {
			return sock_err("recvfrom", s);
		}
		for (int i = 0; i < received; i += 4) {
			nums[i / 4] =
				(unsigned int)(response[i] * 16777216 + response[i + 1] * 65536 + response[i + 2] * 256 +
					response[i + 3]);
		}
		for (int j = 0; j < received / 4; j++) {
			if (findElem(nums[j], index)) {
				col--;
			}
		}
	}
	// Закрытие соединения 
	closesocket(s);
	return 0;
}

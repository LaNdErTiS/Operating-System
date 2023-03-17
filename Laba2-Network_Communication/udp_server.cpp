#include <string>
#include <iostream> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h> 
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h> 
#include <vector> 
#include <fstream>
#include <map>

using namespace std;

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
	vector<int> indexString;
	int col;
};

int s_open() {
	int s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		return -1;
	}
	else {
		return s;
	}
}

int set_non_block_mode(int s) {
	int fl = fcntl(s, F_GETFL, 0);
	return fcntl(s, F_SETFL, fl | O_NONBLOCK);
}

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
	string AA = to_string(msg_protocol.AA);
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

int main(int argc, char*argv[]) {
	int start, finish;
	if (argc != 3) {
		cout << ("Wrong number of parameters");
		return -1;
	}
	start = atoi(argv[1]);
	finish = atoi(argv[2]);


	vector<int> sockets;
	vector<sockaddr_in> addr;
	int minSocket = -1;

	for (int i = 0; i < finish - start + 1; i++) {
		sockets.push_back(s_open());
		set_non_block_mode(sockets[i]);
		sockaddr_in temp;
		memset(&temp, 0, sizeof(temp));
		temp.sin_family = AF_INET;
		temp.sin_port = htons(start + i);
		temp.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.push_back(temp);
		if (bind(sockets[i], (struct sockaddr*) &addr[i], sizeof(addr[i])) < 0) {
			cout << "start";
			return -1;
		}
	}

	fd_set rfd;
	int i;
	struct timeval tv = { 1, 0 };
	int k = 0;
	unordered_map<string, vector<int>> allStrings;
	while (!k) {
		FD_ZERO(&rfd);

		for (int i = 0; i < sockets.size(); i++) {
			FD_SET(sockets[i], &rfd);
			if (sockets[i] > minSocket)minSocket = sockets[i];
		}

		int check = select(minSocket + 1, &rfd, 0, 0, &tv);
		int col = 0;

		if (check > 0) {
			for (int i = 0; i < sockets.size(); i++) {
				if (FD_ISSET(sockets[i], &rfd)) {
					sockaddr_in temp;
					unsigned char buffer[65536] = { 0 };
					memset(&temp, 0, sizeof(temp));
					int addrlen = sizeof(temp);
					int recv = recvfrom(sockets[i], buffer, sizeof(buffer), 0, (struct sockaddr*) &temp, (socklen_t *)&addrlen);
					if (recv <= 0) {
						continue;
					}
					string ip = to_string(ntohl(temp.sin_addr.s_addr));
					string port = to_string(ntohs(temp.sin_port));
					port += ip;
					string rezult = (port);
					unsigned int num;
					num = buffer[0] * 16777216 + buffer[1] * 65536 + buffer[2] * 256
						+ buffer[3];
					map<string, vector<int>>::iterator it;
					it = allStrings.find(rezult);
					if (it != allStrings.end()) {
						it->second.push_back(num);
					}
					else {
						vector<int> temp;
						temp.push_back(num);
						allStrings.insert(make_pair(rezult, temp));
					}
					it = allStrings.find(rezult);
					int mas[100] = { 0 };
					int t = 0;
					vector<int>::iterator zam = it->second.begin();
					while (zam != it->second.end()) {
						mas[t] = htonl(*zam);
						t++; zam++;
					}
					sendto(sockets[i], &mas, sizeof(mas), MSG_NOSIGNAL, (struct sockaddr*) &temp, socklen_t(sizeof(temp)));
					client client;
					client.addr = temp;
					client.indexString.push_back(num);
					msg_format p;
					client.messages.push_back(p);

					client.messages[0].AA = (unsigned short)((unsigned char)buffer[4] * 256 +
						(unsigned char)buffer[5]);
					client.messages[0].BBB = (buffer[6] * 16777216) + (buffer[7] * 65536)
						+ (buffer[8] * 256) + (buffer[9]);
					client.messages[0].hh = (unsigned char)buffer[10];
					client.messages[0].mm = (unsigned char)buffer[11];
					client.messages[0].ss = (unsigned char)buffer[12];
					client.messages[0].size = buffer[13] * 16777216 + buffer[14] * 65536 +
						buffer[15] * 256 + buffer[16];
					string res;
					client.messages[0].msg.reserve(client.messages[0].size + 17 + 1);
					for (int t = 17; t < client.messages[0].size + 17; t++) {
						client.messages[0].msg.push_back(buffer[t]);
					}
					k = print_msg(client, client.messages[0]);
				}
				if (k == 1)break;
			}
		}
	}
	for (int i = 0; i < finish - start + 1; i++) {
		close(sockets[i]);
	}
	return 0;
}

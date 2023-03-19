#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream> 
#include <vector>
#include <time.h>

using namespace std;

unsigned int sum = 0;
int N;
int index = 1;
HANDLE mutex, mutexTwo;
vector<HANDLE> checkEvents;
vector<HANDLE> forMain;
vector<unsigned int> rez;

void decomposition(int quanOfRank, int constRank, int final, int curSum, int start, int index) {
	if (curSum > final || quanOfRank > final)return;
	if (quanOfRank == 0) {
		if (curSum == final) {
			//WaitForSingleObject(mutex, INFINITE);
			rez[index]++;
			//ReleaseMutex(mutex);
		}
		return;
	}
	for (int i = start; i <= final - constRank + 1; i++) {
		if (curSum + i * quanOfRank > final) { break; }
		decomposition(quanOfRank - 1, constRank, final, curSum + i, i, index);
		if (quanOfRank > final / 2) { break; }
	}
}

DWORD WINAPI infinite(void * tid) {
	int id = (char*)tid - (char*)0;
	while (true) {
		WaitForSingleObject(checkEvents[id], INFINITE);
		WaitForSingleObject(mutexTwo, INFINITE);
		index++;
		ReleaseMutex(mutexTwo);
		//cout << "id " << id << endl;
		decomposition(index, index, N, 0, 1, id);
		SetEvent(forMain[id]);
	}
}

void readFile(char * path, unsigned int &T) {
	ifstream in(path, ifstream::in);
	char * temp; size_t size = 0;
	string stroka;
	(getline(in, stroka));
	T = stoi(stroka);
	(getline(in, stroka));
	N = stoi(stroka);
	in.close();
}

void writeFile(string path, unsigned int &T) {
	ofstream out(path, ofstream::out);
	out << T << endl;
	out << N << endl;
	out << sum << endl;
	out.close();
}

void writeFileTime(string path, unsigned int &T) {
	ofstream out(path, ofstream::out);
	out << T << endl;
	out.close();
}

int main() {
	unsigned int T;
	char kek[] = "input.txt";
	readFile(kek, T);
	vector<HANDLE> mas;
	mutex = CreateMutex(NULL, FALSE, NULL);
	mutexTwo = CreateMutex(NULL, FALSE, NULL);
	if (N - 1 < T) {
		T = N - index;
	}
	int zam = T;
	for (int i = 0; i < zam; i++) {
		rez.push_back(0);
		checkEvents.push_back(CreateEvent(NULL, FALSE, FALSE, NULL));
		forMain.push_back(CreateEvent(NULL, FALSE, TRUE, NULL));
	}

	for (int j = 0; j < zam; j++) {
		WaitForSingleObject(forMain[j], INFINITE);
		mas.push_back(CreateThread(0, 0, infinite, (char*)0 + j, 0, 0));// thread(infinite, j));
	}
	srand(time(0));
	unsigned int start_time = clock();
	for (int i = 0; i < zam; i++) {
		SetEvent(checkEvents[i]);
	}
	while (true) {
		for (int i = 0; i < forMain.size(); i++) {
			WaitForSingleObject(forMain[i], INFINITE);
			sum += rez[i]; rez[i] = 0;
			SetEvent(checkEvents[i]);
			WaitForSingleObject(mutexTwo, INFINITE);
			if (index > N) {
				ReleaseMutex(mutexTwo);
				break;
			}
			else {
				ReleaseMutex(mutexTwo);
			}
		}
		WaitForSingleObject(mutexTwo, INFINITE);
		if (index > N) {
			ReleaseMutex(mutexTwo);
			break;
		}
		else {
			ReleaseMutex(mutexTwo);
		}
		//Sleep(50);
	}

	int col = 0;
	while (col != forMain.size()) {
		for (int i = 0; i < forMain.size(); i++) {
			if (WaitForSingleObject(forMain[i], 100) != WAIT_TIMEOUT) {
				sum += rez[i];
				CloseHandle(mas[i]); CloseHandle(checkEvents[i]);
				col++;
			}
		}
	}
	unsigned int end_time = clock();
	unsigned int search_time = end_time - start_time;
	writeFile("output.txt", T);
	writeFileTime("time.txt", search_time);
	for (int i = 0; i < forMain.size(); i++) {
		CloseHandle(forMain[i]);
	}
	CloseHandle(mutex);
	CloseHandle(mutexTwo);
	//cout << "kek" << endl;
	//cin >> col;
	return 0;
}

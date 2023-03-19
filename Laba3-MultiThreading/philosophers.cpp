#include <Windows.h>
#include <iostream>
#include <string>
#include <fstream> 
#include <vector>
#include <time.h>

using namespace std;

unsigned timeAll, philTime;
unsigned int start_time;
int positions[5][3] = { {1,3, 0}, {2,4, 0}, {3, 5, 0}, {4, 1, 0}, {5, 2, 0} };
int index = 0;
CRITICAL_SECTION cs;
HANDLE sem;

DWORD WINAPI infinite(void * tid) {
	int id = (char*)tid - (char*)0;
	long val;
	unsigned int start_phil;
	while (true) {
		WaitForSingleObject(sem, INFINITE);
		start_phil = clock();
		if (start_phil - start_time >= timeAll) {
			ReleaseSemaphore(sem, 1, &val);
			break;
		}
		EnterCriticalSection(&cs);
		if (id == positions[index][0] || id == positions[index][1]) {
			start_phil = clock();
			cout << start_phil - start_time << ":" << id << ":T->E" << endl;
			LeaveCriticalSection(&cs);
			Sleep(philTime);

			EnterCriticalSection(&cs);
			start_phil = clock();
			cout << start_phil - start_time << ":" << id << ":E->T" << endl;
			positions[index][2]++;
			if (positions[index][2] == 2) {
				positions[index][2] = 0; index++;
				if (index >= 5) { index = 0; }
				ReleaseSemaphore(sem, 2, &val);
			}
			LeaveCriticalSection(&cs);
		}
		else {
			LeaveCriticalSection(&cs);
			ReleaseSemaphore(sem, 1, &val);
			Sleep(philTime);
		}
	}
	return 0;
}

int main(int argc, char *args[]) {
	timeAll = stoi(args[1]);
	philTime = stoi(args[2]);
	vector<HANDLE> phils;
	InitializeCriticalSection(&cs);
	sem = CreateSemaphore(0, 2, 2, 0);

	srand(time(0));
	unsigned int start_time = clock();

	for (int j = 0; j < 5; j++) {
		phils.push_back(CreateThread(0, 0, infinite, (char*)0 + j + 1, 0, 0));
	}

	for (int i = 0; i < 5; i++) {
		WaitForSingleObject(phils[i], INFINITE);
		CloseHandle(phils[i]);
	}

	CloseHandle(sem);
	DeleteCriticalSection(&cs);
	return 0;
}

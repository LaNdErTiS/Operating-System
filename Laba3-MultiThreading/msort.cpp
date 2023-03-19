#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <pthread.h>
#include <time.h>
#include <malloc.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sstream>
#include <unistd.h>

using namespace std;

struct ready_data {
	int L;
	int R;
};

vector<pthread_t> threads;
vector<sem_t> waits;
vector<int> arrTw0;
vector<ready_data> data;
vector<ready_data> tempData;
int index = 0;
sem_t queue;
sem_t all;

void checkMergeSort(int L, int R) {
	if (L < R)
	{
		checkMergeSort(L, (L + R) / 2); //сортировка левой части
		checkMergeSort((L + R) / 2 + 1, R); //сортировка правой части
		struct ready_data item;
		item.L = L;
		item.R = R;
		data.push_back(item);
	}
}

void merge(int first, int last)
{
	int middle, start, final, j;
	int *mas = new int[arrTw0.size()];
	middle = (first + last) / 2; //вычисление среднего элемента
	start = first; //начало левой части
	final = middle + 1; //начало правой части
	for (j = first; j <= last; j++) //выполнять от начала до конца
		if ((start <= middle) && ((final > last) || (arrTw0[start] < arrTw0[final])))
		{
			mas[j] = arrTw0[start];
			start++;
		}
		else
		{
			mas[j] = arrTw0[final];
			final++;
		}
	//возвращение результата в список
	for (j = first; j <= last; j++) arrTw0[j] = mas[j];
	delete[]mas;
};

void mergeSort(int L, int R){
	for (int i = L; i <= R; i++) {
		sem_wait(&waits[i]);
	}
	merge(L, R);
	for (int i = L; i <= R; i++) {
		sem_post(&waits[i]);
	}
}

bool checkCurrentData(int L, int R) {
	for (int i = 0; i < tempData.size(); i++) {
		if ((L >= tempData[i].L && L <= tempData[i].R) || (R <= tempData[i].R && R >= tempData[i].L) || (L <= tempData[i].L && R >= tempData[i].R))
			return false;
	}
	return true;
}

void deleteElem(int L, int R) {
	for (int i = 0; i < tempData.size(); i++) {
		if (tempData[i].L == L && tempData[i].R == R) {
			tempData.erase(tempData.begin() + i, tempData.begin() + i + 1);
			return;
		}
	}
}

void *start(void *num) {
	int L = -1, R = -1;
	while (1) {
		sem_wait(&queue);
		if (!data.empty()) {
			if (index >= data.size()) { index = 0; }
			L = data[index].L;
			R = data[index].R;
			if (checkCurrentData(L, R)) {
				tempData.push_back(data[index]);
				data.erase(data.begin() + index, data.begin() + index + 1);
				sem_post(&queue);
				mergeSort(L, R);
				sem_wait(&queue);
				deleteElem(L, R);
				index = 0;
				sem_post(&queue);
			}
			else {
				index++;
				sem_post(&queue);
			}
		}
		else {
			sem_post(&queue);
			break;
		}
	}
}

unsigned long to_ms(struct timespec* tm) {
	return ((unsigned long)tm->tv_sec * 1000 + (unsigned long)tm->tv_nsec / 1000000);
}

void readFile(char * path, unsigned int &T, unsigned int &size) {
	ifstream in(path, ifstream::in);
	string stroka;
	(getline(in, stroka));
	T = stoi(stroka);
	(getline(in, stroka));
	size = stoi(stroka);
	(getline(in, stroka));
	stringstream x;
	x << stroka;
	string zam;
	while (x >> zam) {
		arrTw0.push_back(stoi(zam));
	}
	in.close();
}

void writeFile(string path, unsigned int &T, unsigned &size) {
	ofstream out(path, ofstream::out);
	out << T << endl;
	out << size << endl;
	for (int i = 0; i < arrTw0.size(); i++) {
		if (i == arrTw0.size() - 1)
			out << arrTw0[i];
		else
			out << arrTw0[i] << " ";
	}
	out << endl;
	out.close();
}

void writeFileTime(string path, unsigned long T) {
	ofstream out(path, ofstream::out);
	out << T << endl;
	out.close();
}

int main() {
	struct timespec tm_start, tm_end;
	unsigned int T, size;
	readFile("input.txt", T, size);
	waits.resize(size);
	threads.resize(T);
	checkMergeSort(0, arrTw0.size() - 1);
	int k = sem_init(&queue, 0, 1);
	k = sem_init(&all, 0, 1);

	for (int i = 0; i < size; i++) {
		k = sem_init(&waits[i], 0, 1);
	}

	for (int i = 0; i < T; i++) {
		pthread_create(&threads[i], 0, start, (char *)0 + i);
	}
	clock_gettime(CLOCK_REALTIME, &tm_start);
	for (int i = 0; i < threads.size(); i++) {
		pthread_join(threads[i], 0);
	}
	clock_gettime(CLOCK_REALTIME, &tm_end);
	writeFileTime("time.txt", (to_ms(&tm_end) - to_ms(&tm_start)));
	writeFile("output.txt", T, size);
	sem_destroy(&queue);
	sem_destroy(&all);
	for (int i = 0; i < size; i++) {
		sem_destroy(&waits[i]);
	}
	return 0;
}

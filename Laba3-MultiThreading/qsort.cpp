#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <pthread.h>
#include <time.h>
#include <malloc.h>
#include <stdlib.h>
#include <sstream>
#include <unistd.h>

using namespace std;

vector<pthread_t> threads;
vector<bool> ready;
vector<int> arrTw0;
pthread_mutex_t g_queue_mutex;
pthread_mutex_t pushBack;
pthread_cond_t g_cond_data_ready;

struct ready_data {
	struct ready_data volatile *next;
	int L;
	int R;
};

struct ready_data volatile *g_queue_first = NULL;
struct ready_data volatile *g_queue_last = NULL;

int queue_pop_first(int *L, int *R) {
	struct ready_data volatile *item;
	int ret = 0;
	item = g_queue_first;
	if (item) {
		g_queue_first = item->next;
		if (!g_queue_first)
			g_queue_last = NULL;
		*L = item->L;
		*R = item->R;
		free((void *)item);
		ret = 1;
	}
	return ret;
}

void queue_push_back(int L, int R)
{
	struct ready_data volatile *item = (struct ready_data *) malloc(sizeof(struct ready_data));
	item->next = NULL;
	item->L = L;
	item->R = R;

	// Добавление элемента в список. Атомарность операции гарантируется
	// мутекс, который должен быть захвачен во время вызова этой функции
	if (g_queue_last) {
		g_queue_last->next = item;
		g_queue_last = item;
	}
	else {
		g_queue_first = item;
		g_queue_last = item;
	}
}

void quickSortR(int L, int R)
{
	int i, j, w, x;
	if (L < R)
	{
		i = L; j = R; x = arrTw0[(i + j) / 2];
		do
		{
			while (arrTw0[i] < x) { i++; };
			while (x < arrTw0[j]) { j--; };
			if (i <= j)
			{
				w = arrTw0[i]; arrTw0[i] = arrTw0[j]; arrTw0[j] = w;
				i++; j--;
			}
		} while (i <= j);

		if (j - L > 1000) {
			pthread_mutex_lock(&pushBack);
			queue_push_back(L, j);
			pthread_mutex_unlock(&pushBack);
			pthread_mutex_lock(&g_queue_mutex);
			pthread_cond_signal(&g_cond_data_ready);
			pthread_mutex_unlock(&g_queue_mutex);
		}
		else {
			if (L < j) quickSortR(L, j);
		}
		if (R - i > 1000) {
			pthread_mutex_lock(&pushBack);
			queue_push_back(i, R);
			pthread_mutex_unlock(&pushBack);
			pthread_mutex_lock(&g_queue_mutex);
			pthread_cond_signal(&g_cond_data_ready);
			pthread_mutex_unlock(&g_queue_mutex);
		}
		else {
			if (i < R)
				quickSortR(i, R);
		}
	}
}

bool checkArr() {
	for (int i = 0; i < ready.size(); i++) {
		if (ready[i] == true)
			return false;
	}
	return true;
}

void *start(void *num) {
	int id = (char*)num - (char*)0;
	int L, R;
	while (1) {
		pthread_mutex_lock(&g_queue_mutex);
		if (queue_pop_first(&L, &R)) {
			pthread_mutex_unlock(&g_queue_mutex);
			quickSortR(L, R);
		}
		else {
			ready[id] = false;
			if (checkArr()) {
				pthread_mutex_unlock(&g_queue_mutex);
				break;
			}
			pthread_cond_wait(&g_cond_data_ready, &g_queue_mutex);
			pthread_mutex_unlock(&g_queue_mutex);
		}
	}

	pthread_mutex_lock(&g_queue_mutex);
	pthread_cond_broadcast(&g_cond_data_ready);
	pthread_mutex_unlock(&g_queue_mutex);
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
	threads.resize(T);
	ready.resize(T);
	pthread_mutex_init(&g_queue_mutex, 0);
	pthread_mutex_init(&pushBack, 0);
	pthread_cond_init(&g_cond_data_ready, 0);
	queue_push_back(0, arrTw0.size() - 1);
	for (int i = 0; i < T; i++) {
		ready[i] = true;
		pthread_create(&threads[i], 0, start, (char *)0 + i);
	}
	clock_gettime(CLOCK_REALTIME, &tm_start);
	for (int i = 0; i < threads.size(); i++) {
		pthread_join(threads[i], 0);
	}
	clock_gettime(CLOCK_REALTIME, &tm_end);
	writeFileTime("time.txt", (to_ms(&tm_end) - to_ms(&tm_start)));
	writeFile("output.txt", T, size);
	pthread_mutex_destroy(&g_queue_mutex);
	pthread_mutex_destroy(&pushBack);
	pthread_cond_destroy(&g_cond_data_ready);
	return 0;
}

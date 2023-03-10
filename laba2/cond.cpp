#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <pthread.h> 

using namespace std;
#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
 << endl; exit(EXIT_FAILURE); }

enum store_state {EMPTY, FULL};
store_state state = store_state::EMPTY;
int goods_id = 0; //номер товара
pthread_mutex_t my_mutex;
pthread_cond_t my_cond;

int store;

void *producer(void *arg)
{
	int err;
	//
	while(true) {
		// Захватываем мьютекс и ожидаем освобождения склада
		err = pthread_mutex_lock(&my_mutex);
		if(err != 0)
			err_exit(err, "Cannot lock mutex");
		while(state == FULL) {
			err = pthread_cond_wait(&my_cond, &my_mutex);
			if(err != 0)
				err_exit(err, "Cannot wait on condition variable");
		}
		// Получен сигнал, что в массиве не осталось заданий.
		// Производим новое зание.
		goods_id++;
		store = goods_id;
		cout << "Added task " << store << "!" << endl;

		state = FULL;
		// Посылаем сигнал, что на складе появился товар.
		err = pthread_cond_signal(&my_cond);
		if(err != 0)
			err_exit(err, "Cannot send signal");
		err = pthread_mutex_unlock(&my_mutex);
		if(err != 0)
			err_exit(err, "Cannot unlock mutex");
	} 
 }

void *consumer(void *arg)
{
	int err;
	//
	while(true) {
		// Захватываем мьютекс и ожидаем появления товаров на складе
		err = pthread_mutex_lock(&my_mutex);
		if(err != 0)
			err_exit(err, "Cannot lock mutex");
		while(state == EMPTY) {
			err = pthread_cond_wait(&my_cond, &my_mutex);
			if(err != 0)
				err_exit(err, "Cannot wait on condition variable");
		}
		// Получен сигнал, что на складе имеется товар.
		// Потребляем его.
		cout << "Processing task " << store << "...";
		sleep(1);
		cout << "done" << endl;
		state = EMPTY;
		// Посылаем сигнал, что на складе не осталось товаров.
		err = pthread_cond_signal(&my_cond);
		if(err != 0)
			err_exit(err, "Cannot send signal");
		err = pthread_mutex_unlock(&my_mutex);
		if(err != 0)
			err_exit(err, "Cannot unlock mutex");
	}
}

int main()
{
	int err;
	//
	pthread_t thread1, thread2; // Идентификаторы потоков
	// Инициализируем мьютекс и условную переменную
	err = pthread_cond_init(&my_cond, NULL); 
	if(err != 0)
		err_exit(err, "Cannot initialize condition variable");
	err = pthread_mutex_init(&my_mutex, NULL);
	if(err != 0)
		err_exit(err, "Cannot initialize mutex");
	// Создаём потоки
	err = pthread_create(&thread1, NULL, producer, NULL);
	if(err != 0)
		err_exit(err, "Cannot create thread 1");
	err = pthread_create(&thread2, NULL, consumer, NULL);
	if(err != 0)
		err_exit(err, "Cannot create thread 2");
	// Дожидаемся завершения потоков
	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	// Освобождаем ресурсы, связанные с мьютексом
	// и условной переменной
	pthread_mutex_destroy(&my_mutex);
	pthread_cond_destroy(&my_cond);
} 
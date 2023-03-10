#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <chrono>
#include "header.h"

using namespace std;

#define MICROSECONDS_IN_SECOND 1e-6

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
		<< endl; exit(EXIT_FAILURE); }
const int TASKS_COUNT = 1000;

int task_list[TASKS_COUNT]; // Массив заданий

int current_task = 0; // Указатель на текущее задание
pthread_mutex_t my_mutex; // Мьютекс
pthread_spinlock_t my_lock;    // Спинлок
int sync_posix_type; //тип синхронизации, 0-mutex, 1-spinlock

void do_task(int task_no, int thread_id)
{
	/* Сюда необходимо поместить код, выполняющий задание */
	string res = "thread: " + to_string(thread_id) + " took task: " + to_string(task_no) + "\n";
	// cout << res;

	double dummy = 1e6;
	for (int i = 0; i < 1e6; ++i){
			dummy /= 2;
			dummy *= 2;

	}
}

void *thread_job(void *arg)
{
	int *thread_id = (int *) arg;
	int task_no;
	int err;
	// Перебираем в цикле доступные задания
	while(true) {
		// Захватываем мьютекс или спинлок для исключительного доступа
		// к указателю текущего задания (переменная
		// current_task)
		if (sync_posix_type == 0) {
			err = pthread_mutex_lock(&my_mutex);
			if(err != 0)
				err_exit(err, "Cannot lock mutex");
			}
		else {
			err = pthread_spin_lock(&my_lock);
			if(err != 0)
			err_exit(err, "Cannot lock spinlock");
		}
		
		// Запоминаем номер текущего задания, которое будем исполнять
		task_no = current_task;
		// Сдвигаем указатель текущего задания на следующее
		current_task++;
		// Освобождаем мьютекс или спинлок
		if (sync_posix_type == 0) {
			err = pthread_mutex_unlock(&my_mutex);
			if(err != 0)
				err_exit(err, "Cannot unlock mutex");
		}
		else {
			err = pthread_spin_unlock(&my_lock);
			if(err != 0)
				err_exit(err, "Cannot unlock spinlock");
		}
		
		// Если запомненный номер задания не превышает
		// количества заданий, вызываем функцию, которая
		// выполнит задание.
		// В противном случае завершаем работу потока
		if(task_no < TASKS_COUNT)
			do_task(task_no, *thread_id);
		else
			return NULL;
	}
} 

//argv[1] = threads_count;
//argv[2] = тип примитива синхронизации: 0-mutex, 1-spinlock
int main(int argc, char *argv[])
{
	if(argc != 3) {
		cout << "Wrong number of arguments" << endl;
		exit(-1);
	} 
	cout << fixed; // print out without using scientific notation

	int threads_count = atoi(argv[1]);
	sync_posix_type = atoi(argv[2]);
	// Определяем переменные: идентификаторы потоков и код ошибки
	pthread_t* threads = new pthread_t[threads_count];

	//для идентификации какой поток взял задачу на исполнение
	int* thread_ids = new int[threads_count];
	for (int i = 0; i < threads_count; ++i){
		thread_ids[i] = i + 1;
	}
	
	int err; // Код ошибки
	// Инициализируем массив заданий случайными числами
	for(int i=0; i<TASKS_COUNT; ++i)
		task_list[i] = rand() % TASKS_COUNT;

	if (sync_posix_type == 0) {
		// Инициализируем мьютекс
		err = pthread_mutex_init(&my_mutex, NULL);
		if(err != 0)
			err_exit(err, "Cannot initialize mutex");
		}
	else {
		err = pthread_spin_init(&my_lock, PTHREAD_PROCESS_PRIVATE);
		if(err != 0)
			err_exit(err, "Cannot initialize spinlock");
	}

  	auto begin = chrono::steady_clock::now(); //время начала работы функции потока
	
	// Создаём потоки
	for (int i = 0; i < threads_count; ++i){
		err = pthread_create(&threads[i], NULL, thread_job, &thread_ids[i]);
		if(err != 0)
			err_exit(err, "Cannot create thread");
	}

	//ожидаем завершения всех порожденных потоков
	
	for (int i = 0; i < threads_count; ++i){
		pthread_join(threads[i], NULL);
	}
	//время исполнения программы
	auto end = chrono::steady_clock::now();
  	auto time_difference = chrono::duration_cast<chrono::microseconds>(end - begin).count() * MICROSECONDS_IN_SECOND;
    cout << "Execution time: " << time_difference  << " seconds" << endl;
	
	
	// Освобождаем ресурсы, связанные с мьютексом
	if (sync_posix_type == 0) {
		pthread_mutex_destroy(&my_mutex);
	}
	else {
		// Освобождаем ресурсы, связанные со спинлоком
 		pthread_spin_destroy(&my_lock);
	}

	//освобождаем остальную память
	delete[] threads;
	delete[] thread_ids;
	return 0;
} 

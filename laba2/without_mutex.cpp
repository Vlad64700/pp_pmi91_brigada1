#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>

#include <unistd.h>
using namespace std;
#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
    << endl; exit(EXIT_FAILURE); }
const int TASKS_COUNT = 15;

int task_list[TASKS_COUNT]; // Массив заданий

int current_task = 0; // Указатель на текущее задание
pthread_mutex_t my_mutex; // Мьютекс

void do_task(int thread_id)
{
    /* Сюда необходимо поместить код, выполняющий задание */
    string res = "thread: " + to_string(thread_id) + " took task: " + to_string(current_task) + "\n";
    cout << res;

    double dummy = 1e6;
    for (int i = 0; i < 1e5; ++i){
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
        // Захватываем мьютекс для исключительного доступа
        // к указателю текущего задания (переменная
        // current_task)
        //err = pthread_mutex_lock(&my_mutex);
//        if(err != 0)
//            err_exit(err, "Cannot lock mutex");
        // Запоминаем номер текущего задания, которое будем исполнять
        
        // Сдвигаем указатель текущего задания на следующее
        
        current_task=current_task+=1;
        // Освобождаем мьютекс
        //err = pthread_mutex_unlock(&my_mutex);
//        if(err != 0)
//            err_exit(err, "Cannot unlock mutex");
        // Если запомненный номер задания не превышает
        // количества заданий, вызываем функцию, которая
        // выполнит задание.
        // В противном случае завершаем работу потока
        if(current_task < TASKS_COUNT)
            do_task(*thread_id);
        else
            return NULL;
    }
}
int main()
{
    pthread_t thread1, thread2; // Идентификаторы потоков
    //для идентификации какой поток взял задачу на исполнение
    int thread1_id = 1;
    int thread2_id = 2;
    
    
    int err; // Код ошибки
    // Инициализируем массив заданий случайными числами
    for(int i=0; i<TASKS_COUNT; ++i)
        task_list[i] = rand() % TASKS_COUNT;
    // Инициализируем мьютекс
    err = pthread_mutex_init(&my_mutex, NULL);
    if(err != 0)
        err_exit(err, "Cannot initialize mutex");
    // Создаём потоки
    err = pthread_create(&thread1, NULL, thread_job, &thread1_id);
    if(err != 0)
        err_exit(err, "Cannot create thread 1");
    err = pthread_create(&thread2, NULL, thread_job, &thread2_id);
    if(err != 0)
        err_exit(err, "Cannot create thread 2");
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    // Освобождаем ресурсы, связанные с мьютексом
    pthread_mutex_destroy(&my_mutex);
}


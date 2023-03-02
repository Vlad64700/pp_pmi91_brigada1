#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <chrono>

#define MICROSECONDS_IN_SECOND 1e-6

using namespace std;

/* Функция, которую будет исполнять созданный поток */
void *thread_job(void *arg)
{
  auto begin = chrono::steady_clock::now(); //время начала работы функции потока
  // cout << "Thread is running..." << endl;
  // Преобразуем указатель на параметр потока к правильному типу
  int *param = (int *) arg; 

  double big_number = 1;

  //выполняем заданное количество арифметических действий
  for (int i = 0; i < *param; ++i){
    big_number *= 1.99;
  }

  auto end = chrono::steady_clock::now();//время завершения работы функции потока
  auto time_difference = chrono::duration_cast<chrono::microseconds>(end - begin).count() * MICROSECONDS_IN_SECOND;
  
  cout << "Thread runtime: " << time_difference << " seconds" << endl;
  return 0; 
}

int main(int argc, char *argv[])
{

  if(argc != 2) {
    cout << "Wrong number of arguments" << endl;
    exit(-1);
  } 
  cout << fixed; // print out without using scientific notation

  //Функция atoi() конвертирует строку, на которую указывает параметр str, в int
  int operations_count = atoi(argv[1]);
  // Определяем переменные: идентификатор потока и код ошибки
  pthread_t thread;

  //Задание 5 - запуск потоков с разными атрибутами
  pthread_attr_t thread_attr; // Атрибуты потока
  int err;

  auto begin = chrono::steady_clock::now(); //время перед созданием потока

  // Создаём поток и передаем ему параметр
  err = pthread_create(&thread, NULL, thread_job,  (void *) &operations_count);
  // Если при создании потока произошла ошибка, выводим
  // сообщение об ошибке и прекращаем работу программы
  if(err != 0) {
    cout << "Cannot create a thread: " << strerror(err) << endl;
    exit(-1);
  }

  //время после создания потока
  auto end = chrono::steady_clock::now();
  auto time_difference = chrono::duration_cast<chrono::microseconds>(end - begin).count() * MICROSECONDS_IN_SECOND;
  cout << "Thread creation time: " << time_difference  << " seconds" << endl;
  // Ожидаем завершения всех созданных потоков перед завершением
  // работы программы

  pthread_exit(NULL);
}
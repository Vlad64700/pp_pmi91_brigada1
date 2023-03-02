#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
using namespace std;

/* Функция, которую будет исполнять созданный поток */
void *thread_job(void *arg)
{
  cout << "Thread is running..." << endl;
  return 0;
}

int main(int argc, char *argv[])
{
  if(argc != 2) {
    cout << "Wrong number of arguments" << endl;
    exit(-1);
  } 
  //Функция atoi() конвертирует строку, на которую указывает параметр str, в int
  int threads_count = atoi(argv[1]);
  // Определяем переменные: идентификаторы потоков и код ошибки
  pthread_t* threads = new pthread_t[threads_count];
  int err;

  //создаем n потоков
  for (int i = 0; i < threads_count; ++i){
    // Создаём поток
    err = pthread_create(&threads[i], NULL, thread_job, NULL);
     // Если при создании потока произошла ошибка, выводим
    // сообщение об ошибке и прекращаем работу программы
    if(err != 0) {
      cout << "Cannot create a thread: " << strerror(err) << endl;
      exit(-1);
    }
  }
  // Ожидаем завершения всех созданных потоков перед завершением
  // работы программы

  pthread_exit(NULL);
}

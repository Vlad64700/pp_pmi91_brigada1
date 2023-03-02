#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <chrono>

#define MICROSECONDS_IN_SECOND 1e-6

using namespace std;

//параметры потока
struct params {
  //элемент массива над которым будет проводиться параллельная операция
  int *array_element; 
  int operation_type; // 0 - прибавить, 1 - умножить
};

void thread_operation_sum(int &array_element) {
  array_element += 9;
}

void thread_operation_mul(int &array_element) {
  array_element *= 2;
}

/* Функция, которую будет исполнять созданный поток */
void *thread_job(void *arg)
{
  auto begin = chrono::steady_clock::now(); //время начала работы функции потока
  // Преобразуем указатель на параметр потока к правильному типу

  params *parameters = (params *) arg; 

  string out_res = "Thread parameters: " + to_string(*(parameters->array_element)) +
    ", " + to_string(parameters->operation_type) + "\n";
  cout << out_res;

  void (*func_poiner) (int&);
  if (parameters->operation_type == 0){
    func_poiner = thread_operation_sum;
  }
  else {
    func_poiner = thread_operation_mul;
  }
  

  func_poiner(*(parameters->array_element));

  auto end = chrono::steady_clock::now();//время завершения работы функции потока
  double time_difference = chrono::duration_cast<chrono::microseconds>(end - begin).count() * MICROSECONDS_IN_SECOND;
  out_res =  "Thread runtime: " + to_string(time_difference) + " seconds \n";
  cout << out_res;
  return 0; 
}


//argv[1] = threads_count;
//argv[2] = type of operation;
int main(int argc, char *argv[])
{
  if(argc != 3) {
    cout << "Wrong number of arguments" << endl;
    exit(-1);
  } 
  cout << fixed; // print out without using scientific notation

  //Функция atoi() конвертирует строку, на которую указывает параметр str, в int
  int threads_count = atoi(argv[1]);
  int type_of_operation = atoi(argv[2]);

  params *thread_params = new params[threads_count];;
  int my_array[threads_count]; // all elements will be initialized with 1

  // Определяем переменные: идентификаторы потоков и код ошибки
  pthread_t* threads = new pthread_t[threads_count];
  pthread_attr_t* thread_attrs = new pthread_attr_t[threads_count]; // Атрибуты потока

  int err; //код ошибки

  auto begin = chrono::steady_clock::now(); //время перед созданием потоков

  //создаем threads_count потоков
  for (int i = 0; i < threads_count; ++i){
    my_array[i] = 1;
    //каждый поток параллельно обрабатывает свой элемент массива
    thread_params[i].array_element = &(my_array[i]);
    thread_params[i].operation_type = type_of_operation;


    // Инициализируем переменную для хранения атрибутов потока
    err = pthread_attr_init(&thread_attrs[i]); 
    if(err != 0) {
      cout << "Cannot create thread attribute: " << strerror(err) << endl;
      exit(-1);
    }

    // Устанавливаем минимальный размер стека для потока (в байтах)
    err = pthread_attr_setstacksize(&thread_attrs[i], 5*1024*1024); 
    if(err != 0) {
      cout << "Setting stack size attribute failed: " << strerror(err) << endl;
      exit(-1);
    }

    // Создаём поток
    err = pthread_create(&threads[i], &thread_attrs[i], thread_job, &thread_params[i]);
    // Если при создании потока произошла ошибка, выводим
    // сообщение об ошибке и прекращаем работу программы
    if(err != 0) {
      cout << "Cannot create a thread: " << strerror(err) << endl;
      exit(-1);
    }
  }

  //время после создания потока
  auto end = chrono::steady_clock::now();
  auto time_difference = chrono::duration_cast<chrono::microseconds>(end - begin).count() * MICROSECONDS_IN_SECOND;
  // cout << "Thread creation time: " << time_difference  << " seconds" << endl;

  // Ожидаем завершения всех созданных потоков перед завершением
  // работы программы
  for (int i = 0; i < threads_count; i++) {
      pthread_join(threads[i], NULL);
  }

  // Освобождаем память, занятую под хранение атрибутов потока
  for (int i = 0; i < threads_count; i++) {
    pthread_attr_destroy(&(thread_attrs[i]));
  }

  //освобождаем остальную память
  delete[] threads;
  delete[] thread_params;

  //результат - выводим массив результатов выполнения параллельных потоков
  for (int i = 0; i < threads_count; ++i){
    string out_res = "arr["+ to_string(i) + "] = " +to_string(my_array[i]) + " ;";
    cout<<out_res << endl;
  }
  pthread_exit(NULL);
}
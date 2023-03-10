#include <cstdlib>
#include <iostream>
#include <cstring>
#include <pthread.h>
#include <chrono>
#include <map>
#include <vector>

using namespace std;

#define err_exit(code, str) { cerr << str << ": " << strerror(code) \
    << endl; exit(EXIT_FAILURE); }

int* my_array;
// Указатель на текущий элемент массива для reduce
int curr_idx_reduce = 0;
pthread_mutex_t my_mutex; // Мьютекс
std::map <int, vector<int> > res_map; //результат mapreduce


//параметры потока
struct params {
  int array_size; //число элементов для обработки данным потоком
  // индекс начиная с которого будут обрабатываться эл-ты массива
  int current_arr_index;
  int operation_type; // 0 - прибавить, 1 - умножить
};

int thread_operation_sum(int array_element) {
  return array_element + rand()%10;
}

int thread_operation_mul(int array_element) {
  return array_element * 2;
}

/* Функция, которую будет исполнять созданный поток */
void *thread_job(void *arg)
{
  // Преобразуем указатель на параметр потока к правильному типу

  params *parameters = (params *) arg;

  // string out_res = "Thread parameters: arrsize: " + to_string(parameters->array_size) 
  // + ", curr_idx: " + to_string(parameters->current_arr_index) + " " + "\n";
  // cout << out_res;

  int (*func_poiner) (int);
  if (parameters->operation_type == 0){
    func_poiner = thread_operation_sum;
  }
  else {
    func_poiner = thread_operation_mul;
  }
  for (int i = parameters->current_arr_index; i < (parameters->current_arr_index + parameters->array_size); ++i) {
    my_array[i] = func_poiner(my_array[i]);
  }

  return 0; 
}

//функция Map будет к каждому элементу массива my_array
//прибавлять случайное число от 0 до 9 (из лабы 1) 
int* myMap(int threads_count, int array_size, int type_of_operation, int* my_array)
{
  cout << fixed; // print out without using scientific notation

  //параметры нового потока
  params *thread_params = new params[threads_count];
  

  // Определяем переменные: идентификаторы потоков и код ошибки
  pthread_t* threads = new pthread_t[threads_count];
  pthread_attr_t* thread_attrs = new pthread_attr_t[threads_count]; // Атрибуты потока

  int err; //код ошибки

  //создаем threads_count потоков
  for (int i = 0; i < threads_count; ++i){
    if (i != threads_count - 1 ){
      thread_params[i].array_size = array_size / threads_count;
    }
    //последний поток берет весь остаток массива
    else {
      thread_params[i].array_size = (array_size / threads_count) + (array_size % threads_count);
    }

    if (i == 0) {
      thread_params[i].current_arr_index = i;
    }
    else {
      thread_params[i].current_arr_index = thread_params[i-1].current_arr_index + thread_params[i-1].array_size; 
    }
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
  return my_array;
}


void do_task(int array_element)
{
    /* Сюда необходимо поместить код, выполняющий задание */
    // string res = " took array_element: " + to_string(array_element) + "\n";
    // cout << res;

    int div_res = array_element % 5;
    if (res_map.count(div_res)) {
      //key exists
      res_map[div_res].push_back(array_element);
    }
    //ключа нет
    else {
      vector<int> vect;
      vect.push_back(array_element);
      res_map.insert(pair<int,vector<int> >(div_res, vect));
    }
}

/* Функция, которую будет исполнять созданный поток */
void *thread_reducer_job(void *arg)
{
    int* array_size = (int*) arg;
    int arr_element_idx;
    int err;
    // Перебираем в цикле доступные элементы массива
    while(true) {
        // Захватываем мьютекс для исключительного доступа
        // к указателю текущего задания (переменная
        // current_task)
        err = pthread_mutex_lock(&my_mutex);
        if(err != 0)
            err_exit(err, "Cannot lock mutex");
        // Запоминаем номер текущего эл-та массива, которое будем исполнять
        arr_element_idx = curr_idx_reduce;
        // Сдвигаем указатель текущего элемента на следующее
        curr_idx_reduce++;
        // Освобождаем мьютекс
        err = pthread_mutex_unlock(&my_mutex);
        if(err != 0)
            err_exit(err, "Cannot unlock mutex");

        if(curr_idx_reduce <= *array_size)
            do_task(my_array[arr_element_idx]);
        else
            return NULL;
    }
}

//Функция reduce будет группировать элементы подданного на вход массива
// по значению остатка деления на 5
void reduce(int threads_count, int array_size, int type_of_operation, int* my_array) {

  // Определяем переменные: идентификаторы потоков и код ошибки
  pthread_t* threads = new pthread_t[threads_count];

  int err; //код ошибки
  // Инициализируем мьютекс
  err = pthread_mutex_init(&my_mutex, NULL);
  if(err != 0)
      err_exit(err, "Cannot initialize mutex");

  //создаем threads_count потоков
  for (int i = 0; i < threads_count; ++i){
    // Создаём потоки
    err = pthread_create(&threads[i], NULL, thread_reducer_job, &array_size);
    // Если при создании потока произошла ошибка, выводим
    // сообщение об ошибке и прекращаем работу программы
    if(err != 0) {
      cout << "Cannot create a thread: " << strerror(err) << endl;
      exit(-1);
    }
  }

  // Ожидаем завершения всех созданных потоков перед завершением
  // работы программы
  for (int i = 0; i < threads_count; i++) {
      pthread_join(threads[i], NULL);
  }

  delete[] threads;
  return;
}

std::map <int, vector<int> > mapReduce(int threads_count, int array_size, int type_of_operation, int* my_array) {
  //функция Map будет к каждому элементу массива my_array
  //прибавлять случайное число от 0 до 9 (из лабы 1) 
  my_array = myMap(threads_count, array_size, type_of_operation, my_array);
  //результат - выводим массив результатов выполнения параллельных потоков
  std::cout << "Map results: \n";
  for (int i = 0; i < array_size; ++i){
    std::string out_res = "arr["+ std::to_string(i) + "] = " +std::to_string(my_array[i]) + " ;";
    std::cout<<out_res << std::endl;
  }

  //Функция reduce будет группировать элементы подданного на вход массива
  // по значению остатка деления на 5
  reduce(threads_count, array_size, type_of_operation, my_array);
  return res_map;
}

//argv[1] = threads_count;
//argv[2] = array_size;
//argv[3] = type of operation;
int main(int argc, char *argv[])
{
  int threads_count = 3;
  int array_size = 10;
  int type_of_operation = 0;

  my_array = new int[array_size];
  //инициализируем массив единичками
  for (int i = 0; i < array_size; ++i){
    my_array[i] = 1;
  }

  auto results = mapReduce(threads_count, array_size, type_of_operation, my_array);
 
  std::cout << "Reduce results: \n";
  for (auto const& x : res_map)
  {
    std::string out_res = std::to_string(x.first) + " : {";
    for (auto const& el : x.second){
      out_res += std::to_string(el) + ", ";
    }
    //убираем лишние символы в конце
    out_res.pop_back();
    out_res.pop_back();


    std::cout <<out_res + "} " << std::endl;
  }

  delete[] my_array;
  return 0;
}
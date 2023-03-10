#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <vector>

#define err_exit(code, str) { std::cerr << str << ": " << strerror(code) \
  << std::endl; exit(EXIT_FAILURE); }

char recieve[500];
pthread_mutex_t my_mutex;
pthread_cond_t my_cond;

pthread_mutex_t my_mutex_2;
pthread_cond_t my_cond_2;

//структура данных которая описывает параметры для передачи в функцию
struct data_client {
  int client_fd;
  int count_of_connect;
};

int done =  0;

std::vector<data_client> queries; //запросы

void *thread_job(void *arg)
{
  int err;

  while(true) {
    // Захватываем мьютекс и ждем новый запрос для пула потоков
    // std::cout << "Locking: " << pthread_self() << std::endl;
    err = pthread_mutex_lock(&my_mutex);
    if(err != 0)
      err_exit(err, "Cannot lock mutex");
    while(queries.empty()) {
      //вызов pthread_cond_wait для блокировки до момента наступления события,
      // при этом мьютекс автоматически освобождается, а поток переводится в спящее состояние.
      // а при получении сигнала мьютекс будет захвачен автоматически.
      err = pthread_cond_wait(&my_cond, &my_mutex);
      if(err != 0)
        err_exit(err, "Cannot wait on condition variable");
    }
    // Получен сигнал, что в пуле появилось новое задание.
    // берем запрос из очереди
    data_client params = queries.back();
    queries.pop_back();
    // std::cout << "Unlocking: " << pthread_self() << std::endl;

    int client_fd= params.client_fd;
    int number = params.count_of_connect;

    //выходная строка для ответа на запрос
    std::string response = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=UTF-8\r\n\r\n"
    "<!DOCTYPE html><html><head><title></title>"
    "<body>Request number " + std::to_string(number) +
    " has been processed></body></html>\r\n";

    read(client_fd, &recieve, 500);
    write(client_fd, response.c_str(), response.length());

    close(client_fd);
    done++;
    printf("request %i has been done\n", done);
      
    // Открываем мьютекс
    err = pthread_cond_signal(&my_cond_2);
    err = pthread_mutex_unlock(&my_mutex);
    if(err != 0)
      err_exit(err, "Cannot unlock mutex");
    

  }
}

//argv[1] = threads_count; - число потоков
int main(int argc, char *argv[])
{



  //Функция atoi() конвертирует строку, на которую указывает параметр str, в int
  int threads_count = 10;
  pthread_t* threads = new pthread_t[threads_count];

  // переменная для счетчика
  int count = 0;
  
  int one = 1, client_fd;
  struct sockaddr_in svr_addr, cli_addr;
  socklen_t sin_len = sizeof(cli_addr);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    err(1, "can't open socket");

  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));

  int port = 8045;
  svr_addr.sin_family = AF_INET;
  svr_addr.sin_addr.s_addr = INADDR_ANY;
  svr_addr.sin_port = htons(port);

  if (bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) == -1) {
    close(sock);
    err(1, "Can't bind");
  }
  

  int error;
  // Инициализируем мьютекс
  error = pthread_mutex_init(&my_mutex, NULL);
  error = pthread_mutex_init(&my_mutex_2, NULL);
  
  if(error != 0) {
    err_exit(error, "Cannot initialize mutex");
  }
  // Инициализируем условную переменную
  error = pthread_cond_init(&my_cond, NULL);
  error = pthread_cond_init(&my_cond_2, NULL);
  if(error != 0)
    err_exit(error, "Cannot initialize condition variable");

  for (int i = 0; i < threads_count; ++i){
    // Создаём потоки
    error = pthread_create(&threads[i], NULL, thread_job, NULL);
    // Если при создании потока произошла ошибка, выводим
    // сообщение об ошибке и прекращаем работу программы
    if(error != 0) {
      std::cout << "Cannot create a thread: " << strerror(error) << std::endl;
      exit(-1);
    }
      //так как изначально поток делается как присоеденямый надо сделать его отсоеденяемым
          pthread_detach(threads[i]);
  }
  
  //второй параметр отвечает за то как много запросов связи может быть принято на сокет одновременно
  // если ложится бенчмарк это прекрасный повод этот параметр увеличить
  listen(sock, 1000);
  // структура данных
  data_client params;
  printf("server started at %i port...\n", port);
  while (true) {
    error = pthread_mutex_lock(&my_mutex_2);
    while (queries.size()>threads_count) {
        error = pthread_cond_wait(&my_cond_2, &my_mutex_2);
        if(error != 0)
        err_exit(error, "Cannot wait on condition variable");
    }
      
    count++;
    client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);
    printf("got connection. ");

    if (client_fd == -1) {
      perror("Can't accept\n");
      continue;
    }
    //добавляем запрос в очередь запросов
    printf("count %i.\n", count);
    //двигаем счетчик
    
    //готовим данные для передачи в функцию
    params.client_fd  = client_fd;
    params.count_of_connect = count;
    // if(error != 0)
    //   err_exit(error, "Cannot lock mutex");
    queries.push_back(params);

    // Посылаем сигнал, что появился запрос в очереди
    error = pthread_cond_signal(&my_cond);
    if(error != 0)
      err_exit(error, "Cannot send signal");

    error = pthread_mutex_unlock(&my_mutex_2);
    if(error != 0)
      err_exit(error, "Cannot unlock mutex");
  }

  // delete[] threads;
  return 0;
}

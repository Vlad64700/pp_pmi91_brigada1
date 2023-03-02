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
#include <iostream>
#include <cstring>
#include <pthread.h>


char recieve[500];

//структура данных которая описывает параметры для передачи в функцию
struct data_client {
    int client_fd;
    int count_of_connect;
};


void *thread_job(void *arg)
{
  // перобразуем входной параметр в нужный тип
  data_client *params = (data_client *) arg;

  int client_fd= params->client_fd;
  int number = params->count_of_connect;

  //выходная строка для ответа на запрос
  std::string response = "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html; charset=UTF-8\r\n\r\n"
  "<!DOCTYPE html><html><head><title></title>"
  "<body>Request number " + std::to_string(number) +
  " has been processed></body></html>\r\n";

  read(client_fd, &recieve, 500);
  write(client_fd, response.c_str(), response.length());

  close(client_fd);
  delete params;

  //pthread_cancel(pthread_self());
  return 0;
}



int main()
{
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
  
  //второй параметр отвечает за то как много запросов связи может быть принято на сокет одновременно
  // если ложится бенчмарт это прекрасный повод этот параметр увеличить
  listen(sock, 1000);
  printf("server started at %i port...\n", port);
  while (1) {
    client_fd = accept(sock, (struct sockaddr *) &cli_addr, &sin_len);
    printf("got connection. ");

    if (client_fd == -1) {
      perror("Can't accept\n");
      continue;
    }
    // для нового потока
    pthread_t thread;
    // структура данных
    data_client * params = new data_client;
    //дввигаем счетчик
    count++;
    //готовим данные для передачи в функцию
    params->client_fd  = client_fd;
    params->count_of_connect = count;
    //атрибуты потока
    pthread_attr_t attr_t;
    pthread_attr_init(&attr_t);
    //размер стека в байтах
      pthread_attr_setstacksize(&attr_t, 512*1024*4);
      
    //создаем поток
    printf("count %i.\n", count);
    int er = pthread_create(&thread, &attr_t, thread_job, (void*)params);
      
    //проверяем ошибки
    if(er!= 0) {
        printf("Cannot create a thread: %i", er);
        close(client_fd);
        exit(-1);
    }
    //так как изначально поток делается как присоеденямый надо сделать его отсоеденяемым
    pthread_detach(thread);
    //удаляем параметры потока
    pthread_attr_destroy(&attr_t);


  }
}

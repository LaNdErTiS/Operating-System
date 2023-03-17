#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <errno.h> 
 
#include <stdio.h> 
#include <string.h> 
#include <stdlib.h>
#include <math.h>
  
int sock_err(const char* function, int s) 
{ 
  int err; 
  err = errno; 
  fprintf(stderr, "%s: socket error: %d\n", function, err); 
  return -1; 
}  

sockaddr_in sock_init(char* server){
  char* sav = server;
  char* ip = strtok(server, ":");
  char* port = strtok(NULL, ":");
  
  sockaddr_in sock;
  memset(&sock, 0, sizeof(sock));
  sock.sin_family = AF_INET;
  sock.sin_port = htons(atoi(port));
  sock.sin_addr.s_addr = inet_addr(ip);
  return sock;
}

int send_str(int sock, char* data, size_t len){
  int res = send(sock, data, len, MSG_NOSIGNAL);
  if(res < 0) return sock_err("send", sock);
  printf("%d bytes sent\n", res);

  return 0;
}

int konvertirovat_ctroky(uint32_t number, char* data, char** buf){
  char *AA = strtok(data, " ");
  char* BBB = strtok(NULL, " ");
  char* time1 = strtok(NULL, " ");
  char* msg = strtok(NULL, "\n");

  if(time1 == NULL || AA == NULL || BBB == NULL || msg == NULL) return -1;

  size_t data_size = 4 + 2 + 4 + 3 + 4 + strlen(msg);
  *buf = (char*) calloc(data_size, sizeof(char));

  char* bytes = *buf;
    
  int* bytes_num = (int*) bytes;
  bytes_num[0] = htonl(number);
  short* kek = (short*) (bytes + 4);

  //short lol = atoi(AA);
  kek[0]= htons(atoi(AA));
 
  bytes_num = (int*)(bytes + 6);
  bytes_num[0] = htonl(atoi(BBB));

  char* h = strtok(time1, ":");
  char* m = strtok(NULL, ":");
  char* s = strtok(NULL, ":");

  bytes[10] = atoi(h);
  bytes[11] = atoi(m);
  bytes[12] = atoi(s);

  bytes_num = (int*)(bytes + 13);
  bytes_num[0]=htonl(strlen(msg));

  strncpy(bytes + 17, msg, strlen(msg));
  return data_size;  
}

int get_response(int socket)
{
  char buffer[128] = {0};
  int res;
  res = recv(socket, buffer, sizeof(buffer), 0);

  if (res < 0)
    return sock_err("recv", socket);

  if(strcmp(buffer, "ok") == 0){
     printf("OK\n");
    return 1;
  }  
  return 0;
}

void send_msgs(int socket, FILE* f){
  int msg_sent = 0;
  char* buf = NULL;
  size_t len = 0;

  while(getline(&buf, &len, f) != -1){ 
    char* dat = NULL;
    int s_len = konvertirovat_ctroky(msg_sent, buf, &dat);
    if(s_len > 0){
      msg_sent++;
      send_str(socket, dat, s_len);
      free(dat);
      get_response(socket);
    }
    free(buf);
    buf = NULL;
  }
}
  
int main(int argc, char** argv) 
{ 
  if(argc < 3) return printf("ERR\n");

  FILE* f = fopen(argv[2], "r");
  if(f == NULL){
  return printf("ERR: %s\n", argv[2]);
  }
    
  int s; 
  struct sockaddr_in addr = sock_init(argv[1]);
 
  // Создание TCP-сокета 
  s = socket(AF_INET, SOCK_STREAM, 0); 
  if (s < 0) 
    return sock_err("socket", s); 
 
  // Установка соединения с удаленным хостом 
  int tries = 0;
  while (connect(s, (struct sockaddr*) &addr, sizeof(addr)) != 0) 
  { 
    tries++;
    if(tries == 10){
      return printf("ERR\n");
    }
    usleep(100000); 
  } 

  send_str(s, "put", 3);

  send_msgs(s, f);
   
  // Закрытие соединения 
  close(s);
  fclose(f);
 
  return 0; 
}

/* 

Boliang Yang (yangb5@rpi.edu)

*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <dirent.h>

#define BUFFER_SIZE 64
fd_set readfds;

void TCP_server_function(int newsd);
void PUT(int newsd, char* buffer, char* filename, char* file_num, char* left);
void GET(int newsd, char* buffer, char* filename, char* foffset, char* file_num);
void LIST(int newsd);

int main(int argc, char* argv[])
{
	if (argc != 2) {
		fprintf(stderr, "ERROR INVALID INPUT FORMAT\n");
		exit( EXIT_FAILURE );
	}
	int tcpsd;
	/* Create the listener socket as TCP socket */
	tcpsd =  socket(PF_INET, SOCK_STREAM, 0);

	printf("Started server\n");
	fflush(stdout);
	if (tcpsd == -1) {
    fprintf(stderr, "ERROR SOCKET() FAILED\n" );
    exit( EXIT_FAILURE );
  }
	unsigned short tcp_port = atoi(argv[1]);
	if (tcp_port == 0) {
		fprintf(stderr, "ERROR: INVALID PORT\n");
		exit( EXIT_FAILURE );
	}
	struct sockaddr_in tcp_server;
	struct sockaddr_in tcp_client;
	tcp_server.sin_family = PF_INET;
	tcp_server.sin_addr.s_addr = INADDR_ANY;
	tcp_server.sin_port = htons(tcp_port);
	if (bind(tcpsd, (struct sockaddr *)&tcp_server, sizeof(tcp_server)) == -1){
		fprintf(stderr, "ERROR BIND() FAILED\n" );
		exit( EXIT_FAILURE );
	}
  if (listen(tcpsd, 5) == -1) {
    fprintf(stderr, "ERROR LISTEN() FAILED\n" );
    exit( EXIT_FAILURE );
  }
	int lenTCP_c = sizeof(tcp_client);
	printf("Listening for TCP connections on port: %s\n", argv[1]);
	fflush(stdout);
  while (1)
  {
		FD_ZERO( &readfds );
		FD_SET(tcpsd, &readfds);
    //TCP coneection
    if (FD_ISSET(tcpsd, &readfds)) {
      int newsd = accept(tcpsd, (struct sockaddr *)&tcp_client,
        (socklen_t *)&lenTCP_c);
      printf("Rcvd incoming TCP connection from: %s\n",
  					inet_ntoa((struct in_addr)tcp_client.sin_addr));
      fflush(stdout);
    	pid_t pid;
      pid = fork();
      if ( pid == -1 ) {
	      fprintf(stderr, "ERROR FORK() FAILED\n" );
	      exit( EXIT_FAILURE );
	    }
    	if ( pid > 0 ) {
	      /* parent simply closes the new client socket
	          because the child process will handle that connection */
	      close(newsd);
	    }
			else	{
				TCP_server_function(newsd);
				close(newsd);
				return 0;
			}
    }
  }
	return 0;
}

void TCP_server_function(int newsd)
{
  int n;
	char buffer[BUFFER_SIZE];
  char filename[50];
	while (1)
	{
    n = recv(newsd, buffer, BUFFER_SIZE - 1, 0);
		if (n == -1) {
			fprintf(stderr, "[CHILD %d] ERROR RECV() FAILED\n", getpid());
			return;
		}
    else if (n == 0) {
			printf("[CHILD %d] Client disconnected\n", getpid());
			fflush(stdout);
			close(newsd);
			return;
    }
    else {
      buffer[n] = '\0';
      char app[5];
      strncpy(app, buffer, 4);
      app[4] = '\0';
      char * pchn = strchr(buffer, '\n');

      if (strcmp(app, "PUT ") == 0) {
  			char* pchs1 = strchr(buffer, ' ');
  			char* pchs2 = strchr(pchs1 + 1, ' ');
  			if (pchs1 == NULL || pchs2 == NULL || pchn == NULL) {
  					char str[] = "ERROR INVALID REQUEST\n";
  					printf("[CHILD %d] Sent ERROR INVALID REQUEST\n", getpid());
            fflush(stdout);
  					if(send(newsd, str, strlen(str), 0) != strlen(str))	{
  						fprintf(stderr, "ERROR SEND() FAILED" );
  						return;
  					}
  					continue;
  			}
  			strncpy(filename, pchs1 + 1, pchs2 - pchs1 -1);
  			filename[pchs2 - pchs1 -1] = '\0';
        char file_num[10];
  			strncpy(file_num, pchs2 + 1, pchn - pchs2 -1);
        file_num[pchn - pchs2 -1] = '\0';
        char left[BUFFER_SIZE];
        strncpy(left, pchn + 1, buffer + n - pchn);
        left[buffer + n - pchn] = '\0';
        PUT(newsd, buffer, filename, file_num, left);
      }
      else if (strcmp(app, "GET ") == 0) {
  			char* pchs1 = strchr(buffer, ' ');
  			char* pchs2 = strchr(pchs1 + 1, ' ');
  			char* pchs3 = strchr(pchs2 + 1, ' ');
        if (pchs1 == NULL || pchs2 == NULL || pchs3 == NULL || pchn == NULL) {
  					char str[] = "ERROR INVALID REQUEST\n";
  					printf("[CHILD %d] Sent ERROR INVALID REQUEST\n", getpid());
            fflush(stdout);
  					if(send(newsd, str, strlen(str), 0) != strlen(str))	{
  						fprintf(stderr, "ERROR SEND() FAILED" );
  						return;
  					}
  					continue;
  			}
  			strncpy(filename, pchs1 + 1, pchs2 - pchs1 -1);
  			filename[pchs2 - pchs1 -1] = '\0';
        char foffset[10];
        strncpy(foffset, pchs2 + 1, pchs3 - pchs2 - 1);
        foffset[pchs3 - pchs2 - 1] = '\0';
        char file_num[10];
  			strncpy(file_num, pchs3 + 1, pchn - pchs3 -1);
  			file_num[pchn - pchs3 -1] = '\0';
        GET(newsd, buffer, filename, foffset, file_num);
      }
      else if (strcmp(app, "LIST") == 0) {
        LIST(newsd);
      }
      else	{
  			printf("[CHILD %d] ERROR INVALID COMMAND\n", getpid());
				fflush(stdout);
  		}
    }
  }
  return;
}


void PUT(int newsd, char* buffer, char* filename, char* file_num, char* left)
{
  printf("[CHILD %d] Received PUT %s %s\n", getpid(), filename, file_num);
  fflush(stdout);
  char path[100] =  "storage/";
	int flag = 0;
  strcat(path, filename);
  FILE *fRead = fopen(path, "r");
  if (fRead != NULL) {
    char str[] = "ERROR FILE EXISTS\n";
    printf("[CHILD %d] Sent ERROR FILE EXISTS\n", getpid());
    fflush(stdout);
    if(send(newsd, str, strlen(str), 0) != strlen(str)) {
      fprintf(stderr, "ERROR SEND() FAILED" );
      return;
    }
    fflush(stdout);
    fclose(fRead);
    flag = 1;
  }
  int num = atoi(file_num);
  if (num <= 0) {
    char str[] = "ERROR INVALID REQUEST\n";
    printf("[CHILD %d] Sent ERROR INVALID REQUEST\n", getpid());
    fflush(stdout);
    if(send(newsd, str, strlen(str), 0) != strlen(str))
    {
      fprintf(stderr, "ERROR SEND() FAILED" );
      return;
    }
    return;
  }
  FILE *fp = fopen(path, "w");
  if (flag == 0) {
    fputs(left, fp);
  }
	num -= strlen(left);
  int n;
  while (num > 0)
  {
    if (BUFFER_SIZE < num) {
      n = recv(newsd, buffer, BUFFER_SIZE - 1, 0);
      buffer[n] = '\0';
    }
    else {
      n = recv(newsd, buffer, num, 0);
      buffer[n] = '\0';
    }
		if (flag == 0) {
			fputs(buffer, fp);
		}
    num -= n;
  }
	if (flag == 0) {
	  fclose(fp);
	  printf("[CHILD %d] Stored file \"%s\" (%s bytes)\n", getpid(), filename, file_num);
	  fflush(stdout);
	  if(send(newsd, "ACK\n", 4, 0) != 4) {
	    fprintf(stderr, "[CHILD %d] ERROR SEND() FAILED\n", getpid());
	    return;
		}
	  printf("[CHILD %d] Sent ACK\n", getpid());
		fflush(stdout);
  }
  return;
}


void GET(int newsd, char* buffer, char* filename, char* foffset, char* file_num)
{
  printf("[CHILD %d] Received GET %s %s %s\n", getpid(), filename, foffset, file_num);
  fflush(stdout);
  char path[100] =  "storage/";
  strcat(path, filename);
	FILE *fp = fopen(path, "rb");
  if (fp == NULL) {
    char str[] = "ERROR NO SUCH FILE\n";
    printf("[CHILD %d] Sent ERROR NO SUCH FILE\n", getpid());
    fflush(stdout);
    if(send(newsd, str, strlen(str), 0) != strlen(str))
    {
      fprintf(stderr, "ERROR SEND() FAILED" );
      return;
    }
    fflush(stdout);
    return;
  }
  int offset = atoi(foffset);
  int num = atoi(file_num);
  if (num <= 0 && offset < 0) {
    char str[] = "ERROR INVALID REQUEST\n";
    printf("[CHILD %d] Sent ERROR INVALID REQUEST\n", getpid());
    fflush(stdout);
    if(send(newsd, str, strlen(str), 0) != strlen(str))
    {
      fprintf(stderr, "ERROR SEND() FAILED" );
      return;
    }
    return;
  }
  char* buffer_file;
  long lSize;
  fseek (fp , 0 , SEEK_END);
  lSize = ftell (fp);
  rewind (fp);
  buffer_file = (char*) calloc (lSize, sizeof(char));
  fread(buffer_file, 1, lSize, fp);
  fclose(fp);/*
  if (lSize < (offset + num - 1)) {
    char str[] = "ERROR INVALID BYTE RANGE\n";
    printf("[CHILD %d] Sent ERROR INVALID BYTE RANGE\n", getpid());
    if(send(newsd, str, strlen(str), 0) != strlen(str)) {
      fprintf(stderr, "ERROR SEND() FAILED" );
      return;
    }
    return;
  }*/
  char* excerpt_file;
  excerpt_file = (char*) calloc (num, sizeof(char));
  strncpy(excerpt_file, buffer_file + offset, num);
  char temp[4 + strlen(file_num)];
	memset(temp,0,sizeof(temp));
  strcat(temp, "ACK ");
  strcat(temp, file_num);
  strcat(temp, "\n");
  if(send(newsd, temp, strlen(temp), 0) != strlen(temp)) {
    fprintf(stderr, "ERROR SEND() FAILED" );
    return;
  }
  printf("[CHILD %d] Sent ACK %s\n", getpid(), file_num);
  fflush(stdout);
  if(send(newsd, excerpt_file, num, 0) != num) {
    fprintf(stderr, "ERROR SEND() FAILED" );
    return;
  }
	printf("[child %d] Sent %s bytes of \"%s\" from offset %s\n", getpid(), file_num, filename, foffset);
	fflush(stdout);
  free(buffer_file);
  free(excerpt_file);
  return;
}

void LIST(int newsd)
{
  printf("[CHILD %d] Received LIST\n", getpid());
	fflush(stdout);
  struct dirent **namelist;
  int num = scandir("storage", &namelist, NULL, alphasort);
  if (num < 0) {
    fprintf(stderr, "ERROR scandir() fail\n");
    return;
  }
  else if (num == 2)  {
    char str[] = "0\n";
    if(send(newsd, str, 2, 0) != 2) {
      fprintf(stderr, "ERROR SEND() FAILED" );
      return;
    }
    return;
  }
  else
  {
    char len_f[10];
    char str[100];
		memset(str,0,sizeof(str));
    int i, count = 0;
    count += sprintf(len_f, "%d", num - 2);
    strcat(str, len_f);
    for (i = 2; i < num; i++)
    {
      strcat(str, " ");
      strcat(str, namelist[i]->d_name);
			count += (1 + strlen(namelist[i]->d_name));
    }
    char tmp[1];
    tmp[0] = '\n';
		tmp[1] = '\0';
		count += 1;
    strcat(str, tmp);
    if(send(newsd, str, count, 0) != count){
      fprintf(stderr, "ERROR SEND() FAILED" );
      return;
    }
    printf("[CHILD %d] Sent %s", getpid(), str);
		fflush(stdout);
		for (i = 2; i < num; i++) {
			free(namelist[i]);
		}
  }
  return;
}

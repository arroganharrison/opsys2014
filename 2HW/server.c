#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#define BUFFER_SIZE 1024

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int mutex_checker = 0;
int num_readers = 0;

void * handleConnection(void * arg);
void handleClientInput(char * buffer, int sock);

int main(int argc, char ** argv)
{
  struct stat buf;	
  if (stat(".storage", &buf) != 0) {
  	mkdir(".storage", 0);
  }
  /* Create the listener socket as TCP socket */
  int sock = socket( PF_INET, SOCK_STREAM, 0 );

  if ( sock < 0 )
  {
    perror( "socket() failed" );
    exit( EXIT_FAILURE );
  }

  /* socket structures */
  struct sockaddr_in server;

  server.sin_family = PF_INET;
  server.sin_addr.s_addr = INADDR_ANY;

  unsigned short port = atoi(argv[1]);

  /* htons() is host-to-network-short for marshalling */
  /* Internet is "big endian"; Intel is "little endian" */
  server.sin_port = htons( port );
  int len = sizeof( server );

  if ( bind( sock, (struct sockaddr *)&server, len ) < 0 )
  {
    perror( "bind() failed" );
    exit( EXIT_FAILURE );
  }

  listen( sock, 5 );   /* 5 is the max number of waiting clients */
  printf("Started file-server\n");
  printf( "Listening on port %d\n", port );

  struct sockaddr_in client;
  int fromlen = sizeof( client );

  
  int threadCounter = 0;

  while ( 1 )
  {
    //printf( "PARENT: Blocked on accept()\n" );
    int newsock = accept( sock, (struct sockaddr *)&client,
                          (socklen_t*)&fromlen );
    //printf( "PARENT: Accepted client connection\n" );
    printf("Recieved incoming connection from %s\n", inet_ntoa( (struct in_addr)client.sin_addr ));
    /* handle socket in child process */
    int rc;
    int * t;
    t = (int *)malloc( sizeof( int ) );
    *t = newsock;
    pthread_t tid;

    rc = pthread_create( &tid, NULL, handleConnection, t );
    
    if ( rc != 0 ) {
      perror( "Could not create the thread" );
    }
    threadCounter++;
  }

  close( sock );

  return EXIT_SUCCESS;
}

void *  handleConnection(void * arg) {

	char buffer[ BUFFER_SIZE ];
	int newsock= *(int *) arg;
	while (1) {

		int n = read(newsock, buffer, BUFFER_SIZE);
		if (n > 0 ) {
			buffer[n] = '\n';
			//char * response = 
			handleClientInput(buffer, newsock);
			//write(newsock, response, strlen(response));
		}
		else if (n == 0) {
			printf("[thread %ld] Client closed its socket....terminating\n", (long)pthread_self());
			break;
		}
	
	}

	return arg;
}

void handleClientInput(char * buffer, int sock) {
	char * fileMeta[3];
	char * pch;

	

	pch = strtok(buffer, "\\\n");

	printf("[thread %ld] Rcvd: %s\n", (long)pthread_self(), pch);

	if (pch != NULL) {
		pch = strtok(NULL, "\n");	
	}
	

	//printf("buffer = %s\n", buffer);
	//printf("pch = %s\n", pch);

	char * pch2;
	pch2 = strtok(buffer, " ");
	int i = 0;
	while (pch2 != NULL && i < 3) {
		//printf("pch2 = %s\n", pch2);
		////printf("fileMeta[i] = %s\n", fileMeta[i]);
		//strcpy(fileMeta[i], pch2);
		fileMeta[i] = pch2;
		//printf("fileMeta[%d] = %s\n", i, fileMeta[i]);
		//printf("pch2 = %s\n", pch2 );
		pch2 = strtok(NULL, " ");
		i++;
	}

	// for (i = 0; i < 3; i++) {
	// 	//printf("%s\n", fileMeta[i]);
	// }

	//printf("checking for function\n");

	if (strncmp("ADD", fileMeta[0], 3) == 0) {
		while (num_readers != 0) {

		}

		char fileContents[atoi(fileMeta[2])];
		memcpy(fileContents, pch, atoi(fileMeta[2]));
		//printf("%s\n", fileContents);
		
		pthread_mutex_lock(&mutex);
		mutex_checker++;
		//printf("locked mutex\n");
		struct stat buf;
		char filename[1024];
		sprintf(filename, ".storage/%s", fileMeta[1]);

		//printf("%s\n", filename);
  		if (stat(filename, &buf) != 0) {
  			int fd = open(filename, O_CREAT | O_WRONLY);
  			write(fd, fileContents, atoi(fileMeta[2]));
  			close(fd);
  			pthread_mutex_unlock(&mutex);
  			mutex_checker--;
  			printf("Transferred file (%d)\n", atoi(fileMeta[2]));
  			printf("[thread %ld] Sent: ACK\n", (long)pthread_self());
  			write(sock, "ACK\n", 4);
  		}
  		else {
  			pthread_mutex_unlock(&mutex);
  			mutex_checker--;
  			printf("[thread %ld] Sent: ERROR FILE EXISTS\n", (long)pthread_self());
  			write(sock, "ERROR FILE EXISTS\n", strlen("ERROR FILE EXISTS\n"));
  		}
  		
  	}



	else if (strncmp("APPEND", fileMeta[0], 6) == 0) {
		while (num_readers != 0) {

		}

		char fileContents[atoi(fileMeta[2])];
		memcpy(fileContents, pch, atoi(fileMeta[2]));
		//printf("%s\n", fileContents);

		pthread_mutex_lock(&mutex);
		mutex_checker++;
		//printf("locked mutex\n");
		struct stat buf;
		char filename[1024];
		sprintf(filename, ".storage/%s", fileMeta[1]);

		//printf("%s\n", filename);
  		if (stat(filename, &buf) == 0) {
  			int fd = open(filename, O_APPEND | O_WRONLY);
  			write(fd, fileContents, atoi(fileMeta[2]));
  			close(fd);
  			pthread_mutex_unlock(&mutex);
  			mutex_checker--;
  			printf("Transferred file (%d)\n", atoi(fileMeta[2]));
  			printf("[thread %ld] Sent: ACK\n", (long)pthread_self());
  			write(sock, "ACK\n", 4);
  		}
  		else {
  			pthread_mutex_unlock(&mutex);
  			mutex_checker--;
  			printf("[thread %ld] Sent: ERROR NO SUCH FILE\n", (long)pthread_self());
  			write(sock, "ERROR NO SUCH FILE\n", strlen("ERROR NO SUCH FILE\n"));
  		}
  		
	}
	else if (strncmp("READ", fileMeta[0], 4) == 0) {
		while (mutex_checker != 0) {

		}
		num_readers++;
		struct stat buf;
		char filename[1024];
		//printf(">%s<\n", fileMeta[1]);
		sprintf(filename, ".storage/%s", fileMeta[1]);

		//printf("%d %d\n", stat(filename, &buf), errno);
  		if (stat(filename, &buf) == 0) {
  			int fd = open(filename, O_RDONLY);
  			off_t fileSize = buf.st_size;
  			char requestedFileContents[(int)fileSize+1];
  			read(fd, requestedFileContents, (int)fileSize);
  			requestedFileContents[(int)fileSize] = '\0';
  			char returnString[(int)fileSize + 1024];
  			sprintf(returnString, "ACK %d\n%s\n", (int)fileSize, requestedFileContents);
  			printf("[thread %ld] Sent: ACK %d\n", (long)pthread_self(), (int)fileSize);
  			printf("[thread %ld] Transferred file (%d)\n", (long)pthread_self(), (int)fileSize);
  			write(sock, returnString, strlen(returnString));
  		}
  		else {
  			printf("[thread %ld] Sent: ERROR NO SUCH FILE\n", (long)pthread_self());
  			write(sock, "ERROR NO SUCH FILE\n", strlen("ERROR NO SUCH FILE\n"));;
  		}
  		num_readers--;

		

	}
	else if (strncmp("DELETE", fileMeta[0], 6) == 0) {
		while (num_readers != 0) {

		}
		struct stat buf;
		char filename[1024];
		//printf(">%s<\n", fileMeta[1]);
		sprintf(filename, ".storage/%s", fileMeta[1]);

		pthread_mutex_lock(&mutex);
		mutex_checker++;

		//printf("%d %d\n", stat(filename, &buf), errno);
  		if (stat(filename, &buf) == 0) {
  			remove(filename);
  			pthread_mutex_unlock(&mutex);
  			mutex_checker--;
  			printf("[thread %ld] Delete \"%s\" file\n", (long)pthread_self(), fileMeta[1] );
  			printf("[thread %ld] Sent: ACK\n", (long)pthread_self());
  			write(sock, "ACK\n", 4);
  		}
  		else {
  			pthread_mutex_unlock(&mutex);
  			mutex_checker--;
  			printf("[thread %ld] Sent: ERROR NO SUCH FILE\n", (long)pthread_self());
  			write(sock, "ERROR NO SUCH FILE\n", strlen("ERROR NO SUCH FILE\n"));;
  		}

		

	}
	else if (strncmp("LIST", fileMeta[0], 4) == 0) {
		while (mutex_checker != 0) {

		}
		num_readers++;
		DIR* d;
		char listFiles[10000];
		struct dirent * dir;
		d = opendir(".storage");
		if (d) {
			int i = 0;
			while ( (dir = readdir(d)) != NULL) {
				i++;
				sprintf(listFiles+strlen(listFiles), "%s\n", dir->d_name);

			}
			char response[10000];
			sprintf(response, "%d\n%s\n", i, listFiles);
			write(sock, response, strlen(response));
		}
		num_readers--;

	}
	else {
		write(sock, "ERROR COMMAND NOT RECOGNIZED", strlen("ERROR COMMAND NOT RECOGNIZED"));
	}
	return;
		
	
}
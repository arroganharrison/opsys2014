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

#define BUFFER_SIZE 1024

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void * handleConnection(void * arg);
void handleClientInput(char * buffer);

int main()
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

  unsigned short port = 8127;

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
  printf( "PARENT: Listener bound to port %d\n", port );

  struct sockaddr_in client;
  int fromlen = sizeof( client );

  
  int threadCounter = 0;

  while ( 1 )
  {
    printf( "PARENT: Blocked on accept()\n" );
    int newsock = accept( sock, (struct sockaddr *)&client,
                          (socklen_t*)&fromlen );
    printf( "PARENT: Accepted client connection\n" );

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
			handleClientInput(buffer);	
		}
	
	}

	return arg;
}

void handleClientInput(char * buffer) {
	char * fileContents;
	char * fileMeta[3];
	char * pch;

	printf("pre-strtok buffer = %s\n", buffer);

	pch = strtok(buffer, "\\n");
	pch = strtok(NULL, "\n");

	printf("buffer = %s\n", buffer);
	printf("pch = %s\n", pch);

	char * pch2;
	pch2 = strtok(buffer, " ");
	int i = 0;
	while (pch2 != NULL && i < 3) {
		strcpy(fileMeta[i], pch2);
		printf("fileMeta[%d] = %s\n", i, fileMeta[i]);
		printf("pch2 = %s\n", pch2 );
		pch2 = strtok(NULL, " ");
		i++;
	}

	for (i = 0; i < 3; i++) {
		printf("%s\n", fileMeta[i]);
	}

	//memcpy(fileContents, buffer);
	char request[strlen(buffer)];
	strcpy(request, buffer);

	if (strncmp("ADD ", request, 3) == 0) {
		
	}
}
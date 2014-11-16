/* server-select.c */

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <signal.h>
#include <strings.h>
#include <netdb.h>

#include <sys/select.h>      /* <===== */

extern int errno;

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

static int success_request = 0;
static int blocked_request = 0;

char *msg = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n <!doctype html><html><head>200 OK</head><body> Hello </body> </html>\r\n";
char* msg_405 = "<!doctype html><html><head>405 Method Not Allowed</head>\n<body> You must use a 'GET', 'POST', or 'HEAD' method with this proxy. </body> </html>";
char* msg_403 =  "<!doctype html><html><head>403 Forbidden</head>\n<body>You may not access this website through this proxy. </body> </html>";

void delete_connec( int signalnumber )
{
  /* we know that we received a SIGCHLD, so call wait() */
  int * status = (int*)malloc(sizeof(int));
  wait( status );
  //printf("Status on exit: %d\nPARENT: Received signal %d\n", *status, signalnumber );
  if (*status == 0) {
    success_request++;
  }
  else {
    blocked_request++;
  }
  free(status);
  status = NULL;
}

void parse_request( char* buffer, char* returnvalue, char* host) {
  //fprintf(stderr, "Parsing\n");
  char request_type[8];
  char uri[1024];
  int i = 0;
  //printf("%s\n", buffer);
  while (buffer[i] != '\r' || buffer[i+2] != '\r') {
    i++;
  }
  i+=4;
  int contentlength = strlen(buffer)-i;  
  //fprintf(stderr, "%d\n", i);
  char content[1024];
  while (i < strlen(buffer)) {
    content[i-(strlen(buffer)-contentlength)] = buffer[i];
    i++;
  }
  i = 0;
  while (buffer[i] != ' ') {
    request_type[i] = buffer[i];
    i++;
  }
  request_type[i] = '\0';
  //fprintf(stderr, "Request_type: %s\n", request_type);
  i++;
  int j = 0;
  while (buffer[i] != ' ') {
    uri[j] = buffer[i];
    i++;
    j++;
  }
  uri[j] = '\0';
  int k = 7;
  while (uri[k] != '/') {
    host[k-7] = uri[k];
    k++;
  }
  host[k] = '\0';
  //fprintf(stderr, "URI: %s\n", uri);
  //fprintf(stderr, "Host: %s\n", host);
  if ( strcmp(request_type, "POST") == 0) {
    //fprintf(stderr, "I'm a POST request.\n");
    sprintf(returnvalue, "%s %s HTTP/1.1\r\nContent-Length: %d\r\nConnection: keep-alive\r\n\r\n%s\r\n\r\n\0", request_type, uri, contentlength, content);  
  }
  else {
    sprintf(returnvalue, "%s %s HTTP/1.1\r\nConnection: keep-alive\r\nUser-Agent:Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/32.0.1700.107 Safari/537.36\r\n\r\n\0", request_type, uri);
  }
  return;
}

int validate_request(char* returnvalue, char** filtering, int filter_count, char* host) {
  char method[8];
  char address[1024];
  int i = 0;
  while (returnvalue[i] != ' ') {
    method[i] = returnvalue[i];
    i++;
  }
  method[i] = '\0';
  if (strcmp(method, "POST") == 0  || strcmp(method, "HEAD") == 0 || strcmp(method, "GET") == 0 || strcmp(method, "CONNECT") == 0) {
    
  }
  else {
    sprintf(returnvalue, "HTTP/1.0 405 Method Not Allowed\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n", (int)strlen(msg_405), msg_405);
    //fprintf(stderr, "Invalid method\n");
    return 1;
  }
  //fprintf(stderr, "VALID method\n");
  int host_len = strlen(host);
  i = 0;
  for (; i < filter_count; i++) {
    int j = 0;
    //fprintf(stderr, "Filter = %s\n", filtering[i]);
    for (; j < host_len; j++) {
      if (j == strlen(filtering[i])) {
        sprintf(returnvalue, "HTTP/1.0 403 Forbidden\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n %s\r\n\r\n", (int)strlen(msg_403), msg_403);
        return 1;
      }
      if (host[j] != filtering[i][j]) {
        //fprintf(stderr, "Host: %c != Filter[%d]: %c\n", host[j], i, filtering[i][j]);
        break;
      }
      
    }
    j = 0;
    for (; j < host_len; j++) {
      if (j == strlen(filtering[i])) {
        sprintf(returnvalue, "HTTP/1.0 403 Forbidden\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n %s\r\n\r\n", (int)strlen(msg_403), msg_403);
        return 1;
        //fprintf(stderr, "Host: %c == Filter[%d]: %c\n", host[j], i, filtering[i][j]);
      }
      if (host[host_len-(j+1)] != filtering[i][strlen(filtering[i])-(j+1)]) {
        //fprintf(stderr, "Host: %c != Filter[%d]: %c\n", host[j], i, filtering[i][j]);

        break;
      }
      
      
    }
  }
  return 0;
}

int main(int argc, char *argv[])
{
  char buffer[ BUFFER_SIZE ];
  char** filtering = malloc((argc-1)*128);
  int i = 2;
  for (; i < argc; i++) {
    filtering[i-2] = argv[i];
  }
  int filter_count = argc-2;
  int sock, newsock, len, n;
  unsigned int fromlen;
  pid_t pid;

  fd_set readfds;
  int client_sockets[ MAX_CLIENTS ]; /* client socket fd list */
  int client_socket_index = 0;  /* next free spot */

  /* socket structures from /usr/include/sys/socket.h */
  struct sockaddr_in server;
  struct sockaddr_in client;

  unsigned short port = atoi(argv[1]);

  /* Create the listener socket as TCP socket */
  /*   (use SOCK_DGRAM for UDP)               */
  sock = socket( PF_INET, SOCK_STREAM, 0 );

  if ( sock < 0 ) {
    perror( "socket()" );
    exit( 1 );
  }

  server.sin_family = PF_INET;
  server.sin_addr.s_addr = INADDR_ANY;

  /* htons() is host-to-network-short for marshalling */
  /* Internet is "big endian"; Intel is "little endian" */
  server.sin_port = htons( port );
  len = sizeof( server );


  if ( bind( sock, (struct sockaddr *)&server, len ) < 0 ) {
    perror( "bind()" );
    exit( 1 );
  }

  fromlen = sizeof( client );
  listen( sock, 5 );  /* 5 is number of backlogged waiting clients */
  //printf( "Listener socket created and bound to port %d\n", port );

  while ( 1 )
  {
    int q;

    /** Add **/
    // struct timeval timeout;
    // timeout.tv_sec = 3;
    // timeout.tv_usec = 500;  /* AND 500 microseconds */
    /** Add **/

    FD_ZERO( &readfds );
    FD_SET( sock, &readfds );
    //printf( "Set FD_SET to include listener fd %d\n", sock );


    /** A **/
    // q = select( FD_SETSIZE, &readfds, NULL, NULL, &timeout );
    // if ( q == 0 ) {
    //   //printf( "No activity\n" );
    //   continue;
    // }
    /** A **/

    if ( FD_ISSET( sock, &readfds ) ) {
      /* we know that this accept() call will NOT block */
      newsock = accept( sock, (struct sockaddr *)&client, &fromlen );
      //printf( "Accepted client connection\n" );
      pid = fork();
    }
    
    if ( pid < 0 ) {
      perror( "fork() failed" );
      return EXIT_FAILURE;
    }
    if (pid == 0) {
      signal(SIGINT, SIG_IGN);
      //fprintf(stderr, "Hello, I'm a child.\n"); // Reports that it's a child
      while(1) {
        n = read( newsock, buffer, BUFFER_SIZE - 1); // Reads from client
        if (n == 0) {
         exit(0);
        }
        else {
          buffer[n] = '\0'; 
          char request[1040];
          char host [1000];
          parse_request(buffer, request, host);
          char old_request[1040];
          strcpy(old_request, request);

          /* VALIDATE AND PREPARE PRINTOUT */

          if (validate_request( request, filtering, filter_count, host) == 0) {
            char printable_request[1024];
            int x = 0;
            while (request[x] != 'H')
            {
              printable_request[x] = request[x];
              x++;
            }
            printable_request[x] = '\n';
            char client_name[1024];
            getnameinfo((struct sockaddr*)&client, sizeof(client), client_name, 1024, NULL, 0, 0);
            fprintf(stderr, "%s: %s", client_name, request );
            //fprintf(stderr, "%s\n", request);
            success_request++;
            //fprintf(stderr, "%d\n", success_request);
            
            /* GET END SOCKET READY */

            int end_sock;
            char end_buffer[4096];
            unsigned short server_port;
            struct sockaddr_in server2;
            struct hostent *hp;

            end_sock = socket( PF_INET, SOCK_STREAM, 0 );
            if ( end_sock < 0 ) {
              perror( "socket()" );
              exit( 1 );
            }

            server2.sin_family = PF_INET;
            hp = gethostbyname( host );  // localhost
            if ( hp == NULL ) {
              perror( "Unknown host" );
              exit( 1 );
            }

            /* could also use memcpy */
            bcopy( (char *)hp->h_addr, (char *)&server2.sin_addr, hp->h_length );
            server_port = 80;
            server2.sin_port = htons( server_port );

            /* CONNECT TO END HOST */

            if ( connect( end_sock, (struct sockaddr *)&server2, sizeof( server2) ) < 0 ) {
              perror( "connect()" );
              exit( 1 );
            }

            /* SEND REQUEST */
            //char user_agent[200] = "User-Agent:Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/32.0.1700.107 Safari/537.36\r\n";
            //sprintf(request, "%s%s", request, user_agent );
            fprintf(stderr, "%s", request);
            n = write( end_sock, request, strlen( request ) );
            if ( n < strlen( request ) ) {
              perror( "write()" );
              exit( 1 );
            }
            //fprintf(stderr, "Processed the write.\n");

            /* RECEIVE RESPONSE */

            while( n > 0) {
              n = read( end_sock, end_buffer, 4095 );  // BLOCK
              //fprintf(stderr, "Processed the read.\n");
              if ( n == -1 ) {
                perror( "read()" );
                exit( 1 );
              }
              else {
                end_buffer[n] = '\0';
                //fprintf(stderr, "Received message from server:\n%s\nHad length: %d\n", end_buffer, (int)strlen(end_buffer));
              }

              /* SEND RESPONSE TO CLIENT */
              int z = n;
              n = write( newsock, end_buffer, z);              //fprintf(stderr, "End buffer length sent to client: %d\n", (int)strlen(end_buffer));
              if ( n < z ) {
                perror( "send()" );

              }
              if (n == 0) {
                fprintf(stderr, "Closing down...");
                shutdown(end_sock, 2);
                shutdown(newsock, 2);
                close(end_sock);
                close(newsock);
                exit(0);   
              }

            }

            
          }
          
          /* FILTER REQUEST */

          else {
            n = write( newsock, request, strlen( request ));
              char printable_request[1024];
              int x = 0;
              while (old_request[x] != 'H')
              {
                printable_request[x] = old_request[x];
                x++;
              }
              printable_request[x] = '\0';
              char client_name[1024];
              getnameinfo((struct sockaddr*)&client, sizeof(client), client_name, 1024, NULL, 0, 0);
              printf("%s: %s[FILTERED]\n", client_name, printable_request );
              blocked_request++;
              
              shutdown(newsock, 2);
              close(newsock);
              exit(1);
          }
          
        }
      }
    }
    else {
      //printf("%d\n", pid);
      signal(SIGCHLD, &delete_connec);
      void print_status(int signo) {
        printf("Received SIGUSR1...reporting status:\n");
        printf("-- Processed %d requests successfully.\n", success_request);
        printf("-- Filtering:");
        int i = 0;
        for (; i < filter_count-1; i++) {
          printf(" %s;", filtering[i]);
        }
        printf(" %s\n", filtering[i]);
        printf("-- Filtered %d requests.\n", blocked_request);
        printf("-- Encountered 0 requests in error.\n");

      }
      signal(SIGUSR1, &print_status);
      void exit_gracefully( int signalnumber ) {
        free(filtering);
        exit(0);
      }
      signal(SIGUSR2, &exit_gracefully);
      signal(SIGINT, SIG_IGN);
    }
  }
  

  return 0; /* we never get here */
}

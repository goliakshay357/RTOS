#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// Error Function
void error(char *msg)
{
  perror(msg);
  exit(0);
}

// Main Function
int main(int argc, char *argv[])
{
  int sockfd, portno, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[256];

  // making sure we got 3 args
  if (argc < 3)
  {
    fprintf(stderr, "usage %s hostname port \n", argv[0]);
    exit(0);
  }
  portno = atoi(argv[2]);
  printf("[+] Arguments given correctly \n");
  // Creating socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  // If socket fails
  if (sockfd < 0)
  {
    error("ERROR opening socket");
  }
  printf("[+] Socket connected succesfully 🚀\n");

  // Taking the IP ADDR
  server = gethostbyname(argv[1]);
  if (server == NULL)
  {
    fprintf(stderr, "Error no such host /n");
    exit(0);
  }

  bzero((char *)&serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;

  bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(portno);

  if (connect(sockfd, &serv_addr, sizeof(serv_addr)) < 0)
  {
    error("Error connecting");
  }
  printf("Connection established with server \n");

  while (1)
  {
    printf("Enter Message:");
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);
    n = write(sockfd, buffer, 255);

    if (n < 0)
    {
      error("Error writing to socket");
    }
    printf("[+]Message sent Succesfully to Server🎉\n", buffer);

    bzero(buffer, 256);
    n = read(sockfd, buffer, 255);
    if (n < 0)
    {
      error("ERROR reading from socket");
    }
    printf("Server Message: %s \n", buffer);
  }

  return 0;
}
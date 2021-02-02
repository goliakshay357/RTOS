#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Error Function
void error(char *msg)
{
  perror(msg);
  exit(1);
}

// Main Funtion
int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno, clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n;

  // If correct arguments are not given
  if (argc < 2)
  {
    fprintf(stderr, "ERROR, Port number is not entered /n");
    exit(1);
  }
  printf("[+] Given correct Arguments \n");
  // Step1: Create a socket connection
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  // If socket fails
  if (sockfd < 0)
  {
    error("ERROR opening socket");
  }
  printf("[+] Socket connected succesfully \n");

  bzero((char *)&serv_addr, sizeof(serv_addr));
  // Getting the port number from CMD
  portno = atoi(argv[1]);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    error("ERROR on binding");
  }
  printf("[+] Binding step succesfully \n");

  listen(sockfd, 5);
  printf("[+]Now Listening \n");

  clilen = sizeof(cli_addr);
  newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  if (newsockfd < 0)
  {
    error("ERROR on accept");
  }
  printf("[+] Client Accepted \n");
  bzero(buffer, 256);

  n = read(newsockfd, buffer, 255);
  if (n < 0)
  {
    error("ERROR reading from socket");
  }

  printf("Here is the message: %s \n", buffer);

  //Sending message to client
  n = write(newsockfd, "Server sending confirmation", 27);
  if (n < 0)
  {
    error("ERROR writing to socket");
  }



  return 0;
}
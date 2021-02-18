#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define NAME_LEN 32

// Error Function
void error(char *msg)
{
  perror(msg);
}

// Client Details Structure
// TODO Client
struct client_t{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
};

struct message_type{
  int chatP_G;
  char message[BUFFER_SZ];
  char senderName[BUFFER_SZ];
  char ReceiverName[BUFFER_SZ];
};

struct client_t* clients[MAX_CLIENTS];

// Theading for sending message for all the clients
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// String Overrite
void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

// String Triming
void str_trim_lf (char* arr, int length) {
  int i;
  for (i = 0; i < length; i++) { // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

// Adding the client to the queue
void queue_add(struct client_t *cl){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(!clients[i]){
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

// Remove clients from the queue
void queue_remove(int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid == uid){
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

// Sending messages to all the clients except The self
void send_message(char *s, int uid){
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(clients[i]->uid != uid){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

// Sending Message to that specific person
void sendPrivateMessage(char* s, char* name){
  pthread_mutex_lock(&clients_mutex);

  printf("Message Sending \n");
  	for(int i=0; i<MAX_CLIENTS; ++i){
		if(clients[i]){
			if(!strcmp(clients[i]->name, name) ){
				if(write(clients[i]->sockfd, s, strlen(s)) < 0){
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}
    printf("Message Sent \n");

  pthread_mutex_unlock(&clients_mutex);
}





// For counting number of client
static _Atomic unsigned int cli_count = 0;
// Unique ID for each Client
static int uid = 10;



// Handling client Messages
void* handle_client(void* arg){
  printf("Someone entered! \n");
  char buffer [BUFFER_SZ];
  char name[NAME_LEN];

  // Whether to disconnect client or not
  // TODO L
  int leave_flag = 0;

  // New client joined so increment
  cli_count = cli_count + 1;
  struct client_t* cli = (struct client_t*) arg;

  // First name is received from the client
  int n = recv(cli->sockfd, name, 32, 0);
  if(n <= 0){
    error("[!] Client Name not entered properly");
    leave_flag = 1;
  }
// ................Client has joined the server
  
  // Name received successfully
  // Store the name
  strcpy(cli->name, name);

  // Send message to all the clients
  sprintf(buffer, "%s joined the server \n", cli->name);
  
  // Print here in the server
  printf("%s", buffer);
  
  // Send message to all the connected clients
  send_message(buffer, cli->uid);

  // Clear the buffer
  bzero(buffer, BUFFER_SZ);




  // For exchanging and receiving the messages from other clients
  while(1){
    if (leave_flag){
			break;
		}


    struct message_type recv_structure;
    int receive = recv(cli->sockfd, &recv_structure, sizeof(recv_structure), 0);
    if(receive > 0){
      // printf("RN: %s \n",recv_structure.ReceiverName);
      // printf("M: %s \n",recv_structure.message);
      printf("KEY: %d \n",recv_structure.chatP_G);

      // THis is for group Message
      if(!strcmp(recv_structure.ReceiverName, "all")){
        // Except this one person..send it to everyone
        send_message(recv_structure.message, cli->uid);
        // Printing the message in the server
        printf("[Group Message] 👉 %s \n", recv_structure.message);
      }

      // Private Message
      // else if(strcmp(recv_structure.ReceiverName, "all")){
      //   printf("Private Message: %s", recv_structure.senderName);
      // }
      else{
        printf("Private Message: %s -> %s \n",recv_structure.ReceiverName, recv_structure.senderName);
        sendPrivateMessage(recv_structure.message, recv_structure.ReceiverName);
      }

      // // Send the message received from this client to all other clients
      // send_message(buffer, cli->uid);
      // // TODO Ch
      // str_trim_lf(buffer, strlen(buffer));
      // // printf("%s -> %s\n", buffer, cli->name);
      //       printf("%s \n", buffer);
    }

    // If the person exits by any means
    else if(receive == 0 || strcmp(buffer, "exit") == 0){
      sprintf(buffer, "%s has left\n", cli->name);
			printf("%s", buffer);
      // Send message that this person has left
			send_message(buffer, cli->uid);
			leave_flag = 1;
    }

    else {
      printf("[!] Error while exchanging Messages in %s \n", cli->name);
			leave_flag = 1;
    }

    bzero(buffer, BUFFER_SZ);
  }


  // User is out of while loop which means person exited
  close(cli->sockfd);
  queue_remove(cli->uid);
  // Free the space
  free(cli);
  // Reduce the count
  cli_count = cli_count -1;
  // Detaching the thread
  pthread_detach(pthread_self());  
}



// Main Function
int main(int argc, char **argv){

  // If inputs are not entered properly
  // TODO
  if(argc != 2){
    printf("[+] Input Error \n");
		return EXIT_FAILURE;
  }

  // IP and Port number for socket
  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);

	int option = 1;
	int listenfd = 0;
  int connfd = 0;
  
  // Socket Addr Struct
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  
  // Threading
  pthread_t tid;  

  // Setting up Socket Settings
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(ip);
  serv_addr.sin_port = htons(port);

  //System Generated Interupt- SIGNAL for ignoring pipe signals
  // TODO C
  // signal(SIGINT, SIG_IGN); 

  // Socket Connection
  if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0){
		error("[!] Failed During Socket Connection");
    return EXIT_FAILURE;
	}
  printf("[+] Socket Connection Successfull \n");



  // Bind the socket file discriptor to IP and Port
  if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
    error("[!] Failed During Binding SocketFD and IP-Port");
    return EXIT_FAILURE;
  }
  printf("[+] Binding Successfull \n");




  // Listening to Socket with 10 backlogs
  if (listen(listenfd, 10) < 0) {
    error("[!] Listening Method failed");
    return EXIT_FAILURE;
	}
  printf("[+] Listening Successfull \n");




  printf("======WHATSAPP CHAT-ROOM ======== \n \n");
  printf("\n");

// ...........................Continous listening to client.........................................
while(1){
  // .....................Accept the Incoming client.....................
  	// TODO N
    socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

  // Checking for Maximum clients check
	if((cli_count + 1) == MAX_CLIENTS){
		printf("Max clients reached. Rejected: ");
    // TODO Print
		close(connfd);
		continue;
	}

  // Configuring Received client Settings
  struct client_t *cli = (struct client_t *)malloc(sizeof(struct client_t));
	cli->address = cli_addr;
	cli->sockfd = connfd;
	cli->uid = uid++;
  
	// Add client to the queue and fork thread 
	queue_add(cli);
  pthread_create(&tid, NULL, &handle_client, (void*)cli);

}
  return EXIT_SUCCESS;
}
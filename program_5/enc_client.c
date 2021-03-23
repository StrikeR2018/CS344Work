#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()

#define BUFMAX 100000   //make sure buffer is large enough to hold biggest plaintext file

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname); 
  if (hostInfo == NULL) { 
    fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

//function to read the plaintext file into an array, checks for bad characters
char* getPlainText(char* filename){
  char* buffer = calloc(BUFMAX, sizeof(char));   //buffer to hold plaintext
  //open file
  FILE *fp = fopen(filename, "r");
  //copy content into buffer
  fgets(buffer, BUFMAX, fp);
  fclose(fp);
  //check for bad characters, i.e. any chars not 'A' through 'Z' or newline
  int i;
  for(i = 0; i < strlen(buffer); ++i){
    if((buffer[i] < 'A' || buffer[i] > 'Z') && buffer[i] != ' '){
      if(buffer[i] != '\n'){
        fprintf(stderr, "Text file contains invalid chars\n");
        exit(1);
      }
    }
  }
  int len = strlen(buffer);
  buffer[len-1] = '\0';
  //new array of size strlen
  char* ptStr = calloc(len, sizeof(char));
  strcat(ptStr, buffer);

  return ptStr;
  
}

//function to read the key into a buffer
char* getKey(char* keyname){
  char* key = calloc(BUFMAX, sizeof(char));
  //open key file
  FILE *fp = fopen(keyname, "r");
  fgets(key, BUFMAX, fp);
  int keylen = strlen(key);
  key[keylen-1] = '\0';
  char* keyStr = calloc(keylen, sizeof(char));
  strcat(keyStr, key);

  return keyStr;
}

int main(int argc, char *argv[]) {
  int socketFD, portNumber, charsWritten, charsRead;
  //we want to use localhost for this
  char* hostname = "localhost";
  struct sockaddr_in serverAddress;
  //make sure buffer can hold largest plaintext file
  //char buffer[100000];
  //make sure buffer is cleared out
  //memset(buffer, '\0', sizeof(buffer));
  // Check usage & args
  if (argc < 3) { 
    fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); 
    exit(0); 
  } 

  //get key and plaintext
  
  char* buffer;
  buffer = getPlainText(argv[1]);
  char* key;
  key = getKey(argv[2]);


  //check to make sure key isn't shorten than plaintext
  if(strlen(buffer) > strlen(key)){
    fprintf(stderr,"Key is too short\n");
    exit(1);
  }
  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

   // Set up the server address struct, change to argv[3] becasue the port number is third arg in our program. The server hostname should be localhost
  setupAddressStruct(&serverAddress, atoi(argv[3]), hostname);

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }
  //printf("client has connected to the server\n");


  //ensure that we are connected to the write server
  char* msg = "enc_cli";
  charsWritten = 0;
  charsWritten = send(socketFD, msg, strlen(msg), 0);

  //buffer to hold response
  char* resp = calloc(7, sizeof(char));
  charsRead = 0;
  while(charsRead == 0){
    charsRead = recv(socketFD, resp, 7, 0);
  }

  if(strcmp(resp, "enc_ser") != 0){
    fprintf(stderr, "Failed to connect to enc_server on port %s", argv[3]);
    exit(2);
  }

  //send size of buffer to server
  int bufflen = strlen(buffer);
  //might need to adapt for txt file 4
  char* tmpBuf = calloc(4, sizeof(char));
  //put buffer size into temp buffer
  sprintf(tmpBuf, "%d", bufflen);
  send(socketFD, tmpBuf, sizeof(tmpBuf), 0);
  //printf("Client has sent bufflen to server\n");


  // Send message to server, use a loop to makesure that the data is sent completely
  charsWritten = 0;
  while(charsWritten < strlen(buffer)){    
    if(charsWritten < 0){
      error("CLIENT: ERROR writing to socket");
    }
    // Write to the server
    charsWritten += send(socketFD, buffer, strlen(buffer), 0);
    //printf("client sent %d bytes\n", charsWritten);
  }


  //send key to server
  charsWritten = 0;
  while(charsWritten < strlen(key)){    
    if(charsWritten < 0){
      error("CLIENT: ERROR writing to socket");
    }
    // Write to the server
    charsWritten += send(socketFD, key, strlen(key), 0); 
    //printf("Client send %d bytes\n", charsWritten);
  }


  // Get return message from server
  // Clear out the buffer again for reuse
  memset(buffer, '\0', sizeof(buffer));

  charsRead = 0;
  //string to hold cyphertext
  char* cyphertext = calloc(BUFMAX, sizeof(char));
  while(charsRead < bufflen){
    if(charsRead < 0){
      error("CLIENT: ERROR reading from socket");
    }
    // Read data from the socket, leaving \0 at end
    charsRead += recv(socketFD, buffer, bufflen, 0);
    strcat(cyphertext, buffer);
  }
  //print out the cyphertext
  printf("%s\n", cyphertext);

  // // Close the socket
  close(socketFD); 
  return 0;
}
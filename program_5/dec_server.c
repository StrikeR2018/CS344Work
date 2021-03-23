#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFMAX 100000

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
} 

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address)); 

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);
  // Allow a client at any address to connect to this server
  address->sin_addr.s_addr = INADDR_ANY;
}

//decryptes the message
char* decryptMsg(char* buffer, char* key){
    int len = strlen(buffer);
    char* encBuffer;
    encBuffer = calloc(len, sizeof(char));      //buffer to store the cyphertext
    int i;
    int textChar;   //value of current char from msg
    int keyChar;    //value of current char from key
    for(i = 0; i < strlen(buffer); ++i){
        char currentBuf = buffer[i];
        char currentKey = key[i];
        //to get the same numerical values as in the example, subtract 'A' from the char, unless it's a space, then its value is 26
        if(currentBuf == ' '){
            textChar = 26;
        }
        else{
            textChar = currentBuf - 'A';
        }
        if(currentKey == ' '){
            keyChar = 26;
        }
        else{
            keyChar = currentKey - 'A';
        }
        //now that we have values for the current char of buff and key, we can subtract them
        int cypherVal;
        cypherVal = (textChar - keyChar);

        if(cypherVal < 0){
            cypherVal += 27;
        }

        if(cypherVal == 26){
            encBuffer[i] = ' ';
        }
        else{
            encBuffer[i] = (cypherVal + 'A'); 
        }
    }
    encBuffer[i] = '\0';        //might need to put newline at end
    return encBuffer;
}

int main(int argc, char *argv[]){
  int connectionSocket, charsRead, charsWritten;
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);
  pid_t spawnpid = -5;
  int childstatus;

  // Check usage & args
  if (argc < 2) { 
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) {
    error("ERROR opening socket");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, 
          (struct sockaddr *)&serverAddress, 
          sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 
  
  // Accept a connection, blocking if one is not available until one connects
  while(1){
    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, 
                (struct sockaddr *)&clientAddress, 
                &sizeOfClientInfo); 
    if (connectionSocket < 0){
      error("ERROR on accept");
    }
    //use a forked child process to read msg from client and encyrpt it, then send it back
    spawnpid = fork();
    switch(spawnpid){
        //fork error
        case -1: {
            error("fork failed");
            break;
        }
        case 0: {
            //buffer to store correct server identifier msg
            char* msg = calloc(7, sizeof(char));
            charsRead = 0;
            charsRead = recv(connectionSocket, msg, 7, 0);
            //verify we are communicating with enc_client
            if(strcmp(msg, "dec_cli") != 0){
              fprintf(stderr, "Not connecting with client on port %s", argv[1]);
              exit(2);
            }
            //if we got enc_client, send proper response back
            memset(msg, '\0', sizeof(msg));
            strcpy(msg, "dec_ser");
            charsWritten = 0;
            charsWritten = send(connectionSocket, msg, strlen(msg), 0);


            char* sizeBuf = calloc(5, sizeof(char));   //to hold buffer size
            char* msgstr = calloc(BUFMAX, sizeof(char));    //to hold the recieved msg
            char* keystr = calloc(BUFMAX, sizeof(char));
            //recieve buffer len
            charsRead = 0;
            while(charsRead == 0){    //ITS TRYING TO READ TOO MANY BYTES HERE
              charsRead = recv(connectionSocket, sizeBuf, sizeof(sizeBuf), 0);
            }
            int bufflen = atoi(sizeBuf);   //make it an int
            //printf("server got bufflen: %d\n", bufflen);

            char* buffer;   //hold the msg from client
            buffer = calloc(bufflen, sizeof(char));
            char* key;
            key = calloc(bufflen, sizeof(char));

            //use a similar loop as in the client to make sure all of the msg is written
            charsRead = 0;
            while(charsRead < bufflen){
                charsRead += recv(connectionSocket, buffer, bufflen, 0);
                //printf("buffer now recieved %d bytes: %s\n", charsRead, buffer);
                //add what's in the buffer to the whole msg string
                strcat(msgstr, buffer);
            }
            //printf("server has recieved msg: %s\n", msgstr);


            //now get the key
            charsRead = 0;
            while(charsRead < bufflen){
                charsRead += recv(connectionSocket, key, bufflen, 0);
                strcat(keystr, key);
            }
            //printf("server has recieved key: %s\n", keystr);


            // //do the decryption
            char* encBuffer;
            encBuffer = decryptMsg(buffer, key);
            //printf("encrypted text is:%s\n", encBuffer);
            //send the cyphertext back to client
            charsRead = 0;
            while(charsRead < strlen(encBuffer)){
              charsRead += send(connectionSocket, encBuffer, strlen(encBuffer), 0);
              //printf("server sent %d bytes\n", charsRead);
            }
            //printf("server has written something back to client\n");
            exit(0);

            break;
        }
        default: {
            //printf("here in the default\n");
            pid_t waitingpid = waitpid(spawnpid, &childstatus, WNOHANG);
        }

    }
    /*

    // Get the message from the client and display it
    memset(buffer, '\0', 256);
    // Read the client's message from the socket
    charsRead = recv(connectionSocket, buffer, 255, 0); 
    if (charsRead < 0){
      error("ERROR reading from socket");
    }
    printf("SERVER: I received this from the client: \"%s\"\n", buffer);

    // Send a Success message back to the client
    charsRead = send(connectionSocket, 
                    "I am the server, and I got your message", 39, 0); 
    if (charsRead < 0){
      error("ERROR writing to socket");
    }
  */
    // Close the connection socket for this client
    close(connectionSocket); 
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}

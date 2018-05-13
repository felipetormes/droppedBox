#include "../headers/serverCommunication.hpp"
#include "../../utils/headers/dropboxUtils.hpp"
#include "../../utils/headers/udpUtils.hpp"
#include "../../utils/fileSystem/headers/folder.hpp"
#include <iostream>
#include <string>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>

using namespace std;

#define TRUE 1

ServerCommunication::ServerCommunication() {}

void ServerCommunication::serverComm(int port) {
  int socketDesc, itr = 1, status, lastChunck = 0;
  socklen_t clilen;
  struct sockaddr_in serverAddress;
  struct sockaddr_in clientAddress;
  Datagram receiveChunck;
  Datagram sendChunck;
  UserInfo userInfo;
  char buffer[CHUNCK_SIZE];
  fflush(stdin);
  char fname[20], ack[10], str[10];
  FILE *fp;
  unsigned int filesizeInt;
  //struct datagram receiveChunck;

  // Open socket
	socketDesc = openSocket();

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  bzero(&(serverAddress.sin_zero), BYTE_IN_BITS);

  if ( bind (
      socketDesc,
      (struct sockaddr *) &serverAddress,
      sizeof(struct sockaddr)
    ) < 0
  ) {
    throwError("[ServerCommunication::ServerCommunication]: Error on on binding");
  }

  clilen = sizeof(struct sockaddr_in);

  Folder* folder = new Folder("");
  folder->createFolder("db/");
  folder->createFolder("db/clients");
  //folder->createFolder(strcat("db/clients/" + (string)userInfo.userId));


  while (TRUE) {
    do {
  		status = recvfrom(
        socketDesc,
        &userInfo,
        sizeof(userInfo),
        MSG_OOB,
        (struct sockaddr *) &clientAddress,
        &clilen
      );
  		if (status < 0) {
        throwError("[ServerCommunication::ServerCommunication]: Error on recvfrom");
      }

      if (strcmp(userInfo.message, UPLOAD) != 0 && strcmp(userInfo.message, DOWNLOAD) != 0) {
        cout << userInfo.message << endl;
      }
    } while (strcmp(userInfo.message, UPLOAD) != 0 && strcmp(userInfo.message, DOWNLOAD) != 0);

  if (strcmp(userInfo.message, UPLOAD) == 0) {
  	status = recvfrom(
      socketDesc,
      buffer,
      CHUNCK_SIZE,
      MSG_OOB,
      (struct sockaddr *) &clientAddress,
      &clilen
    );
  	if (status < 0) {
      throwError("Error on recvfrom");
    }

    strcpy(fname, buffer);

    status = recvfrom(
      socketDesc,
      buffer,
      CHUNCK_SIZE,
      0,
      (struct sockaddr *) &clientAddress,
      &clilen
    );
  	if (status < 0) {
      throwError("Error on recvfrom");
    }

    filesizeInt = atoi(buffer);

    fp = fopen(fname, "wb");
    itr = 1;
    memset(buffer, 0, CHUNCK_SIZE);

    while(itr * CHUNCK_SIZE < filesizeInt) {
      status = recvfrom(
        socketDesc,
        &receiveChunck,
        sizeof(receiveChunck),
        MSG_OOB,
        (struct sockaddr *) &clientAddress,
        &clilen
      );

      if (status < 0) {
        throwError("[ServerCommunication::ServerCommunication]: Error on receiving datagram");
      }
      fflush(stdin);
      sprintf(ack, "%d", receiveChunck.chunckId);

      status = sendto(
        socketDesc,
        ack,
        sizeof(int),
        0,
        (struct sockaddr *) &clientAddress,
        sizeof(struct sockaddr)
      );

      if (status < 0) {
        throwError("[ServerCommunication::ServerCommunication]: Error on sending ack");
      }
      if (lastChunck != receiveChunck.chunckId) {
        fwrite(receiveChunck.chunck, CHUNCK_SIZE, 1, fp);
        memset(receiveChunck.chunck, 0, CHUNCK_SIZE);
        itr++;
      }
      lastChunck = receiveChunck.chunckId;
    }

    memset(receiveChunck.chunck, 0, (filesizeInt % CHUNCK_SIZE));
    status = recvfrom(
      socketDesc,
      &receiveChunck,
      sizeof(receiveChunck),
      0,
      (struct sockaddr *) &clientAddress,
      &clilen
    );
    if (status < 0) {
      throwError("[ServerCommunication::ServerCommunication]: Error on sending ack");
    }

    fwrite(receiveChunck.chunck,(filesizeInt % CHUNCK_SIZE), 1, fp);
    memset(buffer, 0, CHUNCK_SIZE);
    fclose(fp);
    fflush(stdin);

    cout << fname << " has been received" << endl;

    sprintf(userInfo.message, "%s", fname);
  } else if (strcmp(userInfo.message, DOWNLOAD) == 0) {
    status = recvfrom(
      socketDesc,
      buffer,
      CHUNCK_SIZE,
      MSG_OOB,
      (struct sockaddr *) &clientAddress,
      &clilen
    );
    if (status < 0) {
      throwError("Error on recvfrom");
    }

    filesizeInt = fileSize(buffer);

    sprintf(str, "%d", filesizeInt);

    status = sendto(
      socketDesc,
      str,
      CHUNCK_SIZE,
      0,
      (const struct sockaddr *) &clientAddress,
      sizeof(struct sockaddr_in)
    );
    if (status < 0) {
      throwError("[Process::upload]: Error on sending message");
    }

    fp = fopen(buffer, "rb");
    memset(sendChunck.chunck, 0, CHUNCK_SIZE);
    fread(sendChunck.chunck, CHUNCK_SIZE, 1, fp);
    sendChunck.chunckId = 1;

    while(itr * CHUNCK_SIZE < filesizeInt){
      status = sendto(
        socketDesc,
        &sendChunck,
        sizeof(sendChunck),
        0,
        (const struct sockaddr *) &clientAddress,
        sizeof(struct sockaddr_in)
      );
      if (status < 0) {
        throwError("[Process::upload]: Error on sending message");
      }
      fflush(stdin);
      status = recvfrom(
        socketDesc,
        ack,
        sizeof(int),
        MSG_OOB,
        (struct sockaddr *) &clientAddress,
        &clilen
      );
      if (status < 0) {
        throwError("[Process::upload]: Error on receive ack");
      }

      if(atoi(ack) == sendChunck.chunckId) {
        memset(sendChunck.chunck, 0, CHUNCK_SIZE);
        fread(sendChunck.chunck, CHUNCK_SIZE, 1, fp);
        itr++;
        sendChunck.chunckId = sendChunck.chunckId%1000 + 1;
      }
    }

    fread(sendChunck.chunck, (filesizeInt % CHUNCK_SIZE), 1, fp);

    status = sendto(
      socketDesc,
      &sendChunck,
      sizeof(sendChunck),
      0,
      (const struct sockaddr *) &clientAddress,
      sizeof(struct sockaddr_in)
    );
    if (status < 0) {
      throwError("[Process::upload]: Error on sending message");
    }

    memset(sendChunck.chunck, 0, CHUNCK_SIZE);
    fclose(fp);
    sprintf(userInfo.message, "%s", fname);
    }
}

  close(socketDesc);

}

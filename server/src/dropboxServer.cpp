#include "../headers/dropboxServer.hpp"
#include "../headers/serverCommunication.hpp"
#include "../../settings/config.hpp"
#include "../../utils/headers/dropboxUtils.hpp"
#include "../../utils/headers/udpUtils.hpp"
#include "../../utils/headers/process.hpp"
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
#include <sys/stat.h>

using namespace std;

/*
  start socket
  bind
  while true:
    sendto
    recvfrom
  close socket
*/

int main (int argc, char* argv[]) {
  cout << "******* Server is running *******" << endl << endl;
  int port;

  if (argv[PORT_SERVER] != NULL) {
      port = atoi(argv[PORT_SERVER]);
  } else {
    cout << "Usage:" << endl
      << "./dropboxServer <port>" << endl;
    throwError("[Server]: Invalid use of the application");
  }

  int status, socketDesc = 0;
  socklen_t clilen;
  struct sockaddr_in serverAddress;
  struct sockaddr_in clientAddress;
  UserInfo userInfo = {};

  socketDesc = openSocket();

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  bzero(&(serverAddress.sin_zero), BYTE_IN_BITS);

  if (bind (
      socketDesc,
      (struct sockaddr *) &serverAddress,
      sizeof(struct sockaddr)
    ) < 0
  ) {
    throwError("[Server]: Error on on binding");
  }

  status = recvfrom(
    socketDesc,
    &userInfo,
    sizeof(userInfo),
    MSG_OOB,
    (struct sockaddr *) &clientAddress,
    &clilen
  );
  if (status < 0) {
    throwError("[Server]: Error on recvfrom");
  }

  //while(true) {
    //waits for the first message
    //check client info
    //creates a thread
    // makes a new serverCommunication
  //}

  ServerCommunication* s = new ServerCommunication();
  s->serverComm(port, socketDesc, (socklen_t)clilen);
  return 0;
}

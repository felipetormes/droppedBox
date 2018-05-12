#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>

#include "../headers/process.hpp"
#include "../headers/ui.hpp"
#include "../headers/udpUtils.hpp"
#include <unistd.h>
using namespace std;

Process::~Process() {
  cout << "Destroying process..." << endl;
}

bool Process::managerCommands(
  string command,
  string parameter,
  ClientUser* user,
  int port,
  string host,
  int socketDesc
) {
  int resp;
  if (command.compare(UPLOAD) == 0) {
    resp = upload(parameter, user, port, host, socketDesc);
  } else if (command.compare(DOWNLOAD) == 0) {
      resp = download(parameter, user, host, port, socketDesc);
  } else if (command.compare(LIST_SERVER) == 0) {
      resp = listServer(user, port, host, socketDesc);
  } else if (command.compare(LIST_CLIENT) == 0) {
      resp = listClient(user, port, host, socketDesc);
  } else if (command.compare(GET_SYNC_DIR) == 0) {
      resp = getSyncDir(user);
  } else if (command.compare(EXIT_APP) == 0) {
      resp = exitApp(user);
      if (resp == EXIT_OPT_YES) {
        //string clientRequest = "[Client Request]: Log off user";
        //writeToSocket(clientRequest, socketDesc, host, port);
        closeSocket(socketDesc);
        return EXIT;
      } else if (resp == EXIT_OPT_WRONG) {
          resp = exitApp(user);
      } else {
        return !EXIT;
      }
  } else if (command.compare(HELP_C) == 0 || command.compare(HELP_L) == 0) {
    showHelp();
  }else {
    throwError("[managerCommands]: Invalid command");
  }
}

int Process::getProcessId() {
  return this->processId;
}

int Process::upload(string fileName, ClientUser* user, int port, string host, int socketDesc) {
  int status;
  unsigned int lenSckAddr;
  const char *fileNameChar = fileName.c_str();
  const char *hostChar = host.c_str();
  /* According to the C standard, the address of a structure and its first
     member are the same, so you can cast the pointer to sockaddr_in(6) in a
     pointer to sockaddr. (source: https://stackoverflow.com/questions/18609397)*/
  struct sockaddr_in serverAddress;
  struct sockaddr_in from;
  struct hostent *server;
  Datagram sendChunck;
  string homePath = getpwuid(getuid())->pw_dir;
  string filePath = homePath + "/sync_dir_" + user->getUserId() + "/" + fileName;
  unsigned int size;
  char str[10], ack[10];
  FILE *file;
  int itr = 1;
  //struct datagram sendChunck;

  fflush(stdin);
  // Get host
  server = gethostbyname(hostChar);
  if (server == NULL) {
    throwError("[Process::upload]: The host does not exist");
  }

  serverAddress.sin_family = AF_INET; // IPv4
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr = *((struct in_addr *)server->h_addr);
  bzero(&(serverAddress.sin_zero), BYTE_IN_BITS);

  status = sendto(
    socketDesc,
    UPLOAD,
    CHUNCK_SIZE,
    0,
    (const struct sockaddr *) &serverAddress,
    sizeof(struct sockaddr_in)
  );
  if (status < 0) {
    throwError("Error on sending message");
  }

  status = sendto(
    socketDesc,
    fileNameChar,
    CHUNCK_SIZE,
    0,
    (const struct sockaddr *) &serverAddress,
    sizeof(struct sockaddr_in)
  );
  if (status < 0) {
    throwError("[Process::upload]: Error on sending message");
  }

  size = fileSize(filePath);
  //printf("%d\n", size);
  sprintf(str, "%d", size);

  status = sendto(
    socketDesc,
    str,
    CHUNCK_SIZE,
    0,
    (const struct sockaddr *) &serverAddress,
    sizeof(struct sockaddr_in)
  );
  if (status < 0) {
    throwError("[Process::upload]: Error on sending message");
  }
  const char *filePathChar = filePath.c_str();
  file = fopen(filePathChar, "rb");
  memset(sendChunck.chunck, 0, CHUNCK_SIZE);
  fread(sendChunck.chunck, CHUNCK_SIZE, 1, file);
  sendChunck.chunckId = 1;

  while(itr * CHUNCK_SIZE < size){
    status = sendto(
      socketDesc,
      &sendChunck,
      sizeof(sendChunck),
      0,
      (const struct sockaddr *) &serverAddress,
      sizeof(struct sockaddr_in)
    );
    if (status < 0) {
      throwError("[Process::upload]: Error on sending message");
    }

    //printf("%s\n", sendChunck.chunck);
    printf("%d\n", sendChunck.chunckId);

    status = recvfrom(
      socketDesc,
      ack,
      sizeof(int),
      MSG_OOB,
      (struct sockaddr *) &from,
      &lenSckAddr
    );
    if (status < 0) {
      throwError("[Process::upload]: Error on receive ack");
    }

    //printf("%s\n", ack);
    //printf("%d\n", atoi(ack));

    if(atoi(ack) == sendChunck.chunckId) {
      memset(sendChunck.chunck, 0, CHUNCK_SIZE);
      fread(sendChunck.chunck, CHUNCK_SIZE, 1, file);
      itr++;
      sendChunck.chunckId = sendChunck.chunckId%1000 + 1;
    }
  }

  fread(sendChunck.chunck, (size % CHUNCK_SIZE), 1, file);

  status = sendto(
    socketDesc,
    &sendChunck,
    sizeof(sendChunck),
    0,
    (const struct sockaddr *) &serverAddress,
    sizeof(struct sockaddr_in)
  );
  if (status < 0) {
    throwError("[Process::upload]: Error on sending message");
  }

  memset(sendChunck.chunck, 0, CHUNCK_SIZE);
  fclose(file);
  //close(socketDesc);
}

int Process::download(string fileName, ClientUser* user, string host, int port, int socketDesc) {
  int status, lastChunck = 0, sizeInt;
  unsigned int lenSckAddr;
  const char *fileNameChar = fileName.c_str();
  const char *hostChar = host.c_str();
  /* According to the C standard, the address of a structure and its first
     member are the same, so you can cast the pointer to sockaddr_in(6) in a
     pointer to sockaddr. (source: https://stackoverflow.com/questions/18609397)*/
  struct sockaddr_in serverAddress;
  struct sockaddr_in from;
  struct hostent *server;
  Datagram receiveChunck;
  string homePath = getpwuid(getuid())->pw_dir;
  string filePath = homePath + "/sync_dir_" + user->getUserId() + "/" + fileName;
  char str[10], ack[10], size[CHUNCK_SIZE];
  FILE *file;
  int itr = 1;
  //struct datagram sendChunck;

  fflush(stdin);
  // Get host
  server = gethostbyname(hostChar);
  if (server == NULL) {
    throwError("[Process::download]: The host does not exist");
  }

  serverAddress.sin_family = AF_INET; // IPv4
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr = *((struct in_addr *)server->h_addr);
  bzero(&(serverAddress.sin_zero), BYTE_IN_BITS);

  status = sendto(
    socketDesc,
    DOWNLOAD,
    CHUNCK_SIZE,
    0,
    (const struct sockaddr *) &serverAddress,
    sizeof(struct sockaddr_in)
  );
  if (status < 0) {
    throwError("Error on sending message");
  }

  status = sendto(
    socketDesc,
    fileNameChar,
    CHUNCK_SIZE,
    0,
    (const struct sockaddr *) &serverAddress,
    sizeof(struct sockaddr_in)
  );
  if (status < 0) {
    throwError("Error on sending message");
  }

  status = recvfrom(
    socketDesc,
    size,
    CHUNCK_SIZE,
    MSG_OOB,
    (struct sockaddr *) &from,
    &lenSckAddr
  );
  if (status < 0) {
    throwError("[Process::download]: Error on receive ack");
  }

  sizeInt = atoi(size);
  const char *filePathChar = filePath.c_str();
  file = fopen(filePathChar, "wb");

  while(itr * CHUNCK_SIZE < sizeInt) {
    status = recvfrom(
      socketDesc,
      &receiveChunck,
      sizeof(receiveChunck),
      MSG_OOB,
      (struct sockaddr *) &from,
      &lenSckAddr
    );

    if (status < 0) {
      throwError("[Process::download]: Error on receiving datagram");
    }

    sprintf(ack, "%d", receiveChunck.chunckId);

    status = sendto(
      socketDesc,
      ack,
      sizeof(int),
      0,
      (struct sockaddr *) &serverAddress,
      sizeof(struct sockaddr)
    );

    if (status < 0) {
      throwError("[Process::download]: Error on sending ack");
    }
    if (lastChunck != receiveChunck.chunckId) {
      fwrite(receiveChunck.chunck, CHUNCK_SIZE, 1, file);
      memset(receiveChunck.chunck, 0, CHUNCK_SIZE);
      itr++;
    }
    lastChunck = receiveChunck.chunckId;
  }

  memset(receiveChunck.chunck, 0, (sizeInt % CHUNCK_SIZE));
  status = recvfrom(
    socketDesc,
    &receiveChunck,
    sizeof(receiveChunck),
    0,
    (struct sockaddr *) &from,
    &lenSckAddr
  );
  if (status < 0) {
    throwError("[Process::download]: Error on sending ack");
  }

  fwrite(receiveChunck.chunck,(sizeInt % CHUNCK_SIZE), 1, file);
  memset(receiveChunck.chunck, 0, CHUNCK_SIZE);
  fclose(file);
  fflush(stdin);

  cout << fileName << " has been received" << endl;
}

int Process::listServer(ClientUser* user, int port, string host, int socketDesc) {
  string clientRequest = "[Client Request]: List files on the server side";
  writeToSocket(clientRequest, socketDesc, host, port);
  Folder* procFolder = new Folder();
  procFolder->listFiles(SERVER_LIST, user->getUserId());
}

int Process::listClient(ClientUser* user, int port, string host, int socketDesc) {
  string clientRequest = "[Client Request]: List files on the client side";
  writeToSocket(clientRequest, socketDesc, host, port);
  Folder* procFolder = new Folder();
  procFolder->listFiles(CLIENT_LIST, user->getUserId());
}

int Process::getSyncDir(ClientUser* user) {
  cout << "It has to be implemented" << endl;
}

int Process::exitApp(ClientUser* user) {
  string userAnswer;
  cout << "$ Do you really want to log off, " << user->getUserId()
    << "? [yes or no]" << endl;
    cout << "$ ";
  cin >> userAnswer;

  if (userAnswer.compare("yes") == 0 || userAnswer.compare("YES") == 0) {
    cout << "$ Have a good one and take care, " << user->getUserId() << " !" << endl;
    return EXIT_OPT_YES;
  } else if (userAnswer.compare("no") == 0 || userAnswer.compare("NO") == 0) {
      cout << "$ You can stay logged in then!" << endl;
      return EXIT_OPT_NO;
  } else {
      cout << INVALID_OPTION << endl;
      return EXIT_OPT_WRONG;
  }
}

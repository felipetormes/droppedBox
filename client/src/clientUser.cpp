#include <iostream>
#include <string>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <vector>
#include <string>
#include <stdio.h>
#include <semaphore.h>

#include "../headers/clientUser.hpp"
#include "../../utils/headers/ui.hpp"
#include "../../utils/headers/dropboxUtils.hpp"
#include "../../utils/headers/process.hpp"

using namespace std;

ClientUser::ClientUser(string userId, Folder *userFolder, char* ip, int port) {
  this->userId = userId;
  this->userFolder = userFolder;
  this->ip = ip;
  this->port = port;
  this->socketDescriptor = this->loginServer();
}

void ClientUser::startThreads(){
  thread inotifyThread = thread(&ClientUser::inotifyEvent, this);
  thread syncDirThread = thread(&ClientUser::syncDirLoop, this);
  thread userLoop = thread(&ClientUser::userLoop, this);
  thread commandLoopThread = thread(&ClientUser::commandLoop, this);
  commandLoopThread.join();
}

void ClientUser::syncDirLoop() {
  vector<string> commandToRun;
  string command = GET_SYNC_DIR;
  string parameter = "";
  commandToRun.push_back(command);
  commandToRun.push_back(parameter);
  puts(commandToRun.front());
  while(true) {
    usleep(10000000); //10 seconds
    addCommandToQueue(commandToRun);
  }
}

void ClientUser::commandLoop() {
  int resp = !EXIT;
  vector<string> command;
  Process* proc = new Process();
  printf("COMMAND LOOP  1\n");
  while(TRUE){
    command = getCommandFromQueue();
      printf("COMMAND LOOP  2\n");
      resp = proc->managerCommands(command.front(),
                                   command.back(),
                                   this,
                                   this->port,
                                   ip,
                                   this->socketDescriptor
      );
    usleep(200000); //0.2 sec
  }
}

void ClientUser::userLoop() {
  string command, parameter, ip = this->ip;
  vector<string> commandToRun;
  showMenu();
  while(TRUE) {
    commandToRun = getUserCommand();
    printf("USER LOOP\n");
    addCommandToQueue(commandToRun);
  };
}

void ClientUser::inotifyEvent() {
  int fd, wd, wd1, i = 0, len = 0;
  char pathname[100],buf[1024];
  struct inotify_event *event;
  string folderStr = this->userFolder->getHome();
  folderStr += "/sync_dir_" + this->userId;
  //cout << folderStr << endl;
  char watchedPath[100];
  folderStr.copy(watchedPath, folderStr.length(), 0);
  fd = inotify_init1(IN_NONBLOCK);
  bool notTempFile;
  bool threIsThisFile;
  wd = inotify_add_watch(fd, watchedPath, IN_ALL_EVENTS);
  string command;
  string parameter;
  vector<string> commandToRun;

  while(true) {
    i = 0;
    len = read(fd,buf,1024);

    while(i < len) {
      event = (struct inotify_event *) &buf[i];
      sprintf(pathname, "%s/%s", watchedPath, event->name);
      notTempFile = (event->name[0] != '.') && (event->name[strlen(event->name) - 1] != '~');
      threIsThisFile = fileExists(pathname);
      if(!fileExists(pathname) && notTempFile) {
        command = DELETE_FILE;
        parameter = event->name;
        commandToRun.push_back(command);
        commandToRun.push_back(parameter);
        addCommandToQueue(commandToRun);
        command.clear();
        parameter.clear();
        commandToRun.clear();
        //printf("%s : deleted\n",event->name);
      }
      if(event->mask & IN_CLOSE_WRITE) {
        notTempFile = (event->name[0] != '.') && (event->name[strlen(event->name) - 1] != '~');
        threIsThisFile = fileExists(pathname);
        if (notTempFile && threIsThisFile) {
          command = UPLOAD;
          parameter = event->name;
          vector<string> commandToRun;
          commandToRun.push_back(command);
          commandToRun.push_back(parameter);
          addCommandToQueue(commandToRun);
          command.clear();
          parameter.clear();
          commandToRun.clear();
        }
      }
      // update index to start of next event
      i += sizeof(struct inotify_event) + event->len;
    }
  }
}

vector<string> ClientUser::getCommandFromQueue() {
  this->commandAllocation.wait();
  vector<string> c;
  printf("Fila tem %d elementos.\n", commandQueue.size());
  this->commandMutex.lock();
  c = this->commandQueue.front();
  cout << c[0] << endl;
  this->commandQueue.pop();
  printf("Fila agora tem %d elementos.\n", commandQueue.size());
  this->commandMutex.unlock();
  return c;
}

void ClientUser::addCommandToQueue(vector<string> command) {
  this->commandAllocation.post();
  this->commandMutex.lock();
  printf("Fila agora tem %d elementos.\n", this->commandQueue.size());
  this->commandQueue.push(command);
  printf("Comando adicionado a fila: %s\n", command[0]);
  printf("Fila agora tem %d elementos.\n", this->commandQueue.size());
  this->commandMutex.unlock();
}

string ClientUser::getUserId() {
  return this->userId;
}

Folder* ClientUser::getUserFolder() {
  return this->userFolder;
}
void ClientUser::setUserFolder(Folder* userFolder) {
  this->userFolder = userFolder;
}

int ClientUser::loginServer() {
  char* ip = this->ip;
  int port = this->port;
  ClientUser* user = this;
  int socketDesc;
  int status;
  unsigned int lenSckAddr;
  struct sockaddr_in serverAddress;
  struct sockaddr_in from;
  struct hostent *host;
  string clientFolderPath;
  string serverFolderPath;
  UserInfo userInfo = {};

  char buffer[BUFFER_SIZE];
  fflush(stdin);
  // Get host
  host = gethostbyname(ip);
  if (host == NULL) {
    throwError("The host does not exist");
  }

  socketDesc = openSocket();

  // Address' configurations
  serverAddress.sin_family = AF_INET; // IPv4
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr = *((struct in_addr *)host->h_addr);
  bzero(&(serverAddress.sin_zero), BYTE_IN_BITS);

  // Folder management after the user gets logged in
  clientFolderPath = getpwuid(getuid())->pw_dir; // Get user's home folder
  clientFolderPath = clientFolderPath + "/sync_dir_" + user->getUserId();
  serverFolderPath = "db/clients/sync_dir_" + user->getUserId();
  Folder* folder = new Folder("");
  folder->createFolder(clientFolderPath);
  folder->createFolder(serverFolderPath);

  string message = "[Client Login]: User " + user->getUserId()
      + " has logged in via the socket " + to_string(socketDesc);
  message.copy(userInfo.message, message.length(), 0);
  string userId = user->getUserId();
  userId.copy(userInfo.userId, userId.length(), 0);
  writeToSocket(userInfo, socketDesc, ip, port);

  return socketDesc;
}

vector<string> ClientUser::getUserCommand() {
  vector<string> commandWithParameter;
  string wholeCommand;
  char string[256];
  cout << endl << PREFIX << " ";
  fflush(stdin);
  //scanf("%s\n", string);
  cin.clear();
  cin.sync();
  getline(cin, wholeCommand);
  //wholeCommand = string;
  commandWithParameter = parseUserCommand(wholeCommand, " ");
  return commandWithParameter;
}

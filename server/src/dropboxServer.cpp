#include "../headers/serverUser.hpp"
#include "../../utils/headers/dropboxUtils.h"

void closeThreadsOpen(map<string,ServerUser*> threads) {
  auto it = threads.cbegin();
  while (it != threads.cend()) {
    if (!it->second->usingActive) {
      delete it->second;
      it = threads.erase(it);
    } else {
      ++it;
    }
  }
}

int main(int argc, char *argv[]) {
  int port;
  if (argc > 1) {
    port = atoi(argv[1]);
  } else {
    char error[ERROR_MSG_SIZE] = "[DropboxServer]: Invalid parameter";
    throwError(error);
  }
  map<string,ServerUser*> threads;
  UDPUtils listener(port);
  listener.bindServer();
  cout << "******* Server is running *******" << endl << endl;

  while (true) {
    Data message = Data::parse(listener.receive());
    closeThreadsOpen(threads);
    if (message.type == Data::T_SYN) {
      if (!threads.count(message.session)) {
        Process* processComm = new Process(message.content, message.session, listener.get_answerer());
        ServerUser* new_thread = new ServerUser(processComm);
        new_thread->start();
        threads[message.session] = new_thread;
      }
      else {
        char error[ERROR_MSG_SIZE] = "Session already exists";
        throwError(error);
      }
    }
    else {
      cout << "Message received: " << message.type << "\n";
    }
  }
}

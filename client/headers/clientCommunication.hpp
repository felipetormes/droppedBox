#pragma once

#include <string>
#include <thread>
#include "../../utils/headers/communication.hpp"
#include "clientUser.hpp"

using namespace std;
// ClientCommunication extends Communication
class ClientCommunication : public Communication {
  private:
  public:
    int port;
    int socketDescriptor;
    char* ip;
    
    ClientCommunication();
    ClientCommunication(int port);
    ClientCommunication(char* ip, int port);
    int loginServer(char* ip, int port, ClientUser* user);
    bool closeSession ();
    int getPort();
    int getSocketDesc();
    char* getIp();
};

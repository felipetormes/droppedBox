#ifndef PROCESS_H
#define PROCESS_H

#include "udpUtils.hpp"
#include "data.hpp"
#include "dropboxUtils.h"
#include "../fileSystem/headers/file.hpp"

class Process {
  private:
    Data receive();
    void init_sequences();

    map<int, string> messages_sent;
    int theLastPartS;
    int theLastPartRCV;

  public:
    // Attr
    string idUser;
    string folderOfTheUser;
    string session;
    UDPUtils *sock;
    Process(string idUser, string session = "", UDPUtils *new_socket = NULL);
    ~Process();

    void login();
    void initProcessComm();

    void send(string type, string content = "");
    void sendb(string stype, char *content = NULL);
    void send_ack(bool workedProperly = true);
    int sendArq(string filepath);
    void send_string(string data);
    void resend();

    Data receive(string expected_type);
    Data receive(list<string> expected_types);
    Data receive_request();
    bool receive_ack();
    int getArq(string filepath);
    string receive_string();
    string list_server_dir(string dirpath);

    static void *server_thread(void *void_this);
};

#endif

#pragma once

#include <string>

using namespace std;

class ClientAction {
  private:
    string command;
    string parameter;

  public:
    ~ClientAction();
    ClientAction(string command);
    ClientAction(string command, string parameter);
    string getCommand();
    string getParameter();
};

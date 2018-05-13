#include "../headers/action.hpp"

using namespace std;

ClientAction::ClientAction(string command){
  this.command = command;
  this.parameter = NULL;
}

ClientAction::ClientAction(string command, string parameter){
  this.command = command;
  this.parameter = parameter;
}

string ClientAction::getCommand(){
  return this->command;
}
string ClientAction::getParameter(){
  return this->command;
}

#include "../headers/serverUser.hpp"
#include <string>
#include <iostream>
#include <thread>

using namespace std;

ServerUser::ServerUser(string userid) {
  this->userid = userid;
}

string ServerUser::getUserId() {
  return this->userid;
}

Folder* ServerUser::getUserFolder() {
  return this->folderPath;
}

// Return true if the vector of devices does not contain any element
bool ServerUser::thereAreNoDevices() {
  return this->devices.empty();
}

void ServerUser::delDevice(Device* device) {
  // TODO Delete a device from the devices vector
}

// Add a new element of Device at the end of the vector of devices
void ServerUser::addDevice(Device* device) {
  this->devices.push_back(device);
}

void ServerUser::serverThread () {
  std::cout << "ENTREI" << '\n';
  while (true);
}

void ServerUser::startThread() {
  std::thread someThread = std::thread(&ServerUser::serverThread, this);
  someThread.detach();
}

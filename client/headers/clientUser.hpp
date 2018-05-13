#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <queue>
#include "../../utils/headers/device.hpp"
#include "../../utils/fileSystem/headers/folder.hpp"
#include "../../utils/headers/dropboxUtils.hpp"
#include "../../utils/headers/actions.hpp"

using namespace std;

class ClientUser {
  private:
    string userId;
    bool isSync;
    mutex actionsMutex; // Inotify and user enqueue actions
    int numberOfFiles;
  public:
    Folder *userFolder;
    Device* device;

    ClientUser(string userId, Folder* userFolder);
    ClientUser(string userId, Device* device, Folder *userFolder);
    ClientUser(string userId, Device *device, Folder *userFolder, int numberOfFiles);

    queue<ClientAction> actionsQueue;
    void addClientAction();
    ClientAction getClientAction();

    string getUserId();
    Folder* getUserFolder();
    string getUserConnectedDevicesToString();
    int getNumberOfFiles();

    void setUserFolder(Folder* userFolder);
    void sync();
    bool isSynchronized();
    ClientAction getUserCommand();
};

#include <string>
#include <sys/socket.h>

using namespace std;

class ServerCommunication{
  private:
    string loggedUserId;
  public:
    ServerCommunication(int port);
    ServerCommunication();
    void serverComm(int port, int socketDesc, socklen_t clilen);
    void setLoggedUser(string loggedUserId);
    string getLoggedUser();
};

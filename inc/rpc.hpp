#ifndef __NDB_RPC
#define __NDB_RPC
#include <iostream>
#include <thread>
#include <functional>

#include "stratum.hpp"
#include "json.hpp"
#include "tcp.hpp"

using namespace std;
using json = nlohmann::json;

/**
 * @brief      RPC queries and receicption
 */
class RPCConnection {
 private:
  enum ConnectionState  {disconencted, connected};
  int state;
  bool receiveThreadRunning;
  int idCounter;
  std::thread* callbackThread;
  TCPClient* connection;
 public:
  RPCConnection(std::string address, int port) {
    connection = new TCPClient();
    idCounter=111;
    if (connection->setup(address, port)) {
      state = ConnectionState::connected;
    } else {
      state = ConnectionState::disconencted;
      cout << "Setup failed" << endl;
    }
  }
  ~RPCConnection() {
    //Detach receive callback
    if (receiveThreadRunning) {
      cout << "killing tcp thread" << endl;
      receiveThreadRunning = false;
      callbackThread->join();
    }
    delete connection;//closes socket
  }
  bool isConnected() {
    return state == ConnectionState::connected;
  }
  /**
   * @brief      Gets a new id and increments the counter. can be used to send
   *             messages to the server which require new distinct ids.
   *
   * @return     The new identifier.
   */
  int getNewId(){
    return idCounter++;
  }
  bool sendQuery(json& query) {
    //Check if connected
    if (state != ConnectionState::connected)
      cout << "ERR:not connected" << endl;

    if (connection->send(query.dump() + '\n')) {
      return true;
    } else {
      cout << "ERR:failed RPC send" << endl;
      return false;
    }
  }


  // If we are connected, this launches a thread that keeps checking for replies
  // and calls the given callback with a new line if there is one.
  //
  // @param[in]  callback  The callback
  //
  // @return     true: callback setup. false: callback setup failed
  //
  bool registerReceiveCallback(std::function<void(json)>& stratumHandler) {
    if (state == ConnectionState::connected) {
      //launch thread
      receiveThreadRunning = true;
      callbackThread = new std::thread([&]() { 
        while (receiveThreadRunning) {
          std::string res = connection->read();
          json resJson = json::parse(res);
          stratumHandler(resJson);
        }
      });
      return true;
    }
    return false;
  }

};

#endif

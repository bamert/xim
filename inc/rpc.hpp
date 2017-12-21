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

  std::thread* callbackThread;
  TCPClient* connection;
 public:
  RPCConnection(std::string address, int port) {
    connection = new TCPClient();
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
      receiveThreadRunning = false;
      callbackThread->join();
    }
    delete connection;//closes socket
  }
  bool isConnected() {
    return state == ConnectionState::connected;
  }
  bool sendQuery(json& query) {
    //Check if connected
    if (state != ConnectionState::connected)
      cout << "ERR:not connected" << endl;

    //Check if malformed query
    //(to be implemented)
    //Serialize and send

    if (connection->send(query.dump() + '\n')) {
      //If query sent successfully, add id to list to confirm receive later on (and avoid we get reply for sth we didn't ask for.)
      cout << "SENT:" << query.dump() << endl;
      return true;
    } else {
      cout << "ERR:failed RPC send" << endl;
      return false;
    }
  }
  //For now this is synchronous.
  json recMessage() {
    std::string res = connection->read();
    cout << res << endl;
    auto reso = json::parse(res);
    //Confirm we have sent out a query with this ID
    //Then hand package back to stratum handler(the stratum class shouldn't have to care about the connection status nor security)
    return reso;
  }

  // If we are connected, this launches a thread that keeps checking for replies
  // and calls the given callback with a new line if there is one.
  //
  // @param[in]  callback  The callback
  //
  // @return     true: callback setup. false: callback setup failed
  //
  bool registerReceiveCallback(std::function<void(int)> stratumHandler) {
    if (state == ConnectionState::connected) {
      //launch thread
      receiveThreadRunning = true;
      callbackThread = new std::thread([&]() { // issue could be that this is within a thread
       // while (receiveThreadRunning) {
          std::string res = connection->read();
          cout << res << endl;
          json resJson = json::parse(res);
          cout << "calling stratumHandler" << endl;
          // stratumHandler(resJson);
          stratumHandler(2);//t
        //}
      });
      //callbackThread->join();
      //cout << "shouldn't be getting here" << endl;
      return true;
    }
    return false;
  }

};

#endif
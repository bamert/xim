/*
  Manages communication with a SIA stratum server
 */
#ifndef __NDB_STRATUMRPC
#define __NDB_STRATUMRPC
#include <iostream>
#include <thread>


#include "json.hpp"
#include "tcp.hpp"

using namespace std;
using json = nlohmann::json;

class Stratum {
 private:
  enum StratumState  {setup, setupFailed, subscribing, subscribed, connectFailed};
  int state;
  std::thread* stratumThread;
  TCPClient* connection;
 public:
  Stratum(std::string address, int port) {
    connection = new TCPClient();
    if (connection->setup(address, port)) {
      state = StratumState::setup;
    } else {
      state = StratumState::setupFailed;
      cout << "Setup failed" << endl;
    }
  }
  ~Stratum() {
    connection->close();
  }
  void subscribe(std::string miningAddress) {
    //need to pass miningAddress somethow.
    if (state == StratumState::setup) {
      if (connection->send("{\"id\": 1, \"method\": \"mining.subscribe\", \"params\": []}\n")) {
        state = StratumState::subscribing;
        std::string res = connection->read();
        cout << res << endl;
        auto reso = json::parse(res);

        int id;
        std::string miningNotify;
        std::string miningDifficulty;
        //RPC gave us a valid response to the mining subscription query
        if ( reso["error"].is_null() && reso["result"].size() == 3) {
          id = reso["id"];
          //iterate over response array
          for (int i = 0; i < 3; i++) {
            //mining details are in here:
            if (reso["result"][i].is_array()) {
              for (auto& el : reso["result"][i]) { //mining details
                if (el[0] == "mining.notify")
                  miningNotify = el[1];
                if (el[0] == "mining.set_difficulty")
                  miningDifficulty = el[1];
              }
            }//mining details
          }//response array
        }
        cout << "notify:" << miningNotify << ", difficulty:" << miningDifficulty << endl;
        cout << "numResult:" << reso["result"].size() << endl;
        cout << reso["result"][1] << endl;

        //Spawn thread to handle connection events
        /*  stratumThread = new std::thread([&]() {
            std::string res = connection->read();
            cout << res << endl;
            json reso(res);

            cout << "numResult:" << reso["result"].size() << endl;
            cout << reso["result"][1] << endl;


          });*/

      } else {
        state = StratumState::connectFailed;
      }
    }

  }
  bool isSubscribed() {
    if (state == StratumState::subscribed)
      return true;
    else return false;
  }

};


#endif
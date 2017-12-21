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

/**
 * @brief      RPC queries and receicption
 */
class RPCConnection {
 private:
  enum ConnectionState  {disconencted, connected};
  int state;
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

};

// To keep the queries that we sent to server
enum StratumMethod {miningAuthorize, miningSubscribe, miningSubmit};

struct StratumQuery {
  int method;
  int id;
  StratumQuery(int method, int id) : method(method), id(id) {}
};



class Stratum {
 private:
  enum StratumState  {setup, setupFailed, subscribing, subscribed, connectFailed};
  int state;
  RPCConnection* rpc;
  std::thread* stratumThread;
  /* keeps the sent queries such that we can reidentify the responses*/
  std::vector<StratumQuery> sentQueries;
  //Checks if a query with given id has been sent
  bool sentQueryWithId(const int id, int& method) {
    for (auto &el : sentQueries) {
      if (el.id == id) {
        method = el.method;
        return true;
      } else
        return false;
    }
  }

 public:
  Stratum(std::string address, int port) {
    rpc = new RPCConnection(address, port);
  }
  ~Stratum() {
    delete rpc;
  }
  void subscribe(std::string miningAddress) {
    cout << "subscribing" << endl;
    if (rpc->isConnected()) {
      cout << "connected" << endl;
      state = StratumState::setup;
    }

    //need to pass miningAddress somehow.
    if (state == StratumState::setup) {
      cout << "sending subscription query" << endl;
      json q;
      q["id"] = 1;
      q["method"] = "mining.subscribe";
      q["params"] = json::array();

      if (rpc->sendQuery(q)) { //query sent successfully
        sentQueries.push_back(StratumQuery(StratumMethod::miningSubscribe, q["id"]));
      }



      //Spawn thread to handle connection events
      /*  stratumThread = new std::thread([&]() {
          std::string res = connection->read();
          cout << res << endl;
          json r(res);

          cout << "numResult:" << r["result"].size() << endl;
          cout << r["result"][1] << endl;


        });*/


    }
  }
  void processReply() {
    json r = rpc->recMessage();

    int id, method;


    id = r["id"];

    //check if this id is present in the list of our sent queries
    if (sentQueryWithId(id, method)) {
      //We got an answer to our subscribe query.
      if (method == StratumMethod::miningSubscribe) {
        std::string miningNotify;
        std::string miningDifficulty;
        std::string extraNonce1;
        int extraNonce2size ;
        //RPC gave us a valid response to the mining subscription query
        if ( r["error"].is_null() && r["result"].size() == 3) {

          //mining details are in here:
          if (r["result"][0].is_array()) {
            for (auto& el : r["result"][0]) { //mining details
              if (el[0] == "mining.notify")
                miningNotify = el[1];
              if (el[0] == "mining.set_difficulty")
                miningDifficulty = el[1];
            }
          }//mining details
          extraNonce1 = r["result"][1];
          extraNonce2size = r["result"][2];
        }

        cout << "notify:" << miningNotify << ", difficulty:" << miningDifficulty << ",extraNonce1:" << extraNonce1 << ",extraNonce2size:" << extraNonce2size << endl;

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
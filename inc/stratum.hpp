/*
  Manages communication with a SIA stratum server
 */
#ifndef __NDB_SIASTRATUM
#define __NDB_SIASTRATUM
#include <iostream>
#include <thread>
#include <functional>


#include "json.hpp"
#include "tcp.hpp"
#include "rpc.hpp"

using namespace std;
using json = nlohmann::json;


// To keep the queries that we sent to server
enum StratumMethod {miningAuthorize, miningSubscribe, miningSubmit};

struct StratumQuery {
  int method;
  int id;
  StratumQuery(int method, int id) : method(method), id(id) {}
};



class Stratum {
 private:
  enum StratumState  {setup, setupFailed, subscribing, subscribed, connectFailed, disconnected};
  int state;
  RPCConnection* rpc;
  std::thread* stratumThread;
  /* keeps the sent queries such that we can reidentify the responses*/
  std::vector<StratumQuery> sentQueries;


  /*Keeps the callback functor*/
  std::function<void(json)> callbackFunctor;

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
    //if connectioned succeeded, setup receive callback
    if (rpc->isConnected()) {
      //std::function<void()> callback = std::bind(Stratum::processReply, (*this));
      using namespace std::placeholders; //for _1
      callbackFunctor = std::bind(&Stratum::processReply, this, _1);
      if(rpc->registerReceiveCallback(callbackFunctor))
        cout << "callback handler added" << endl;
    }
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
    }
  }
  /**
   * @brief      Processes a reply received from the stratum server. This method
   *             is used as a callback from the RPC class.
   *
   * @param[in]  r     the json object we received over RPC
   */

  void processReply(json r) {
    int id, method;
    id = r["id"];
    cout <<  "processingReply" << endl;
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
    state = StratumState::disconnected;
  }
  bool isSubscribed() {
    return state == StratumState::subscribed;
    
  }
  bool isSetup() {
    return state == StratumState::setup;
    
  }

};


#endif
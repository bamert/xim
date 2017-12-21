/*
  Manages communication with a SIA stratum server
 */
#ifndef __NDB_SIASTRATUM
#define __NDB_SIASTRATUM
#include <iostream>
#include <thread>
#include <functional>
#include <queue>

#include "json.hpp"
#include "tcp.hpp"
#include "rpc.hpp"

using namespace std;
using json = nlohmann::json;


// To keep the queries that we sent to server
enum StratumMethod {miningAuthorize, miningSubscribe, miningSubmit, miningSetDifficulty, clientGetVersion};

struct StratumQuery {
  int method;
  int id;
  StratumQuery(int method, int id) : method(method), id(id) {}
};

//Build a job from the information we've received
struct SiaJob {
  int extraNonce2size;
  std::string jobID;
  std::string prevHash;
  std::string coinb1;
  std::string coinb2;
  std::vector<std::string> merkleBranches;
  std::string blockVersion;
  std::string nBits;
  std::string nTime;
  bool cleanJobs;
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

  /* Keeps the jobs*/
  std::queue<SiaJob> jobs;

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

  //check if a message is a query to the client or an answer from the server
  bool isQuery(const json& q) {
    return q.find("method") != q.end();
  }
  bool isReply(const json& q) {
    return q.find("error") != q.end();
  }
 public:
  Stratum(std::string address, int port) {
    rpc = new RPCConnection(address, port);
    //if connectioned succeeded, setup receive callback
    if (rpc->isConnected()) {
      using namespace std::placeholders; //for _1
      callbackFunctor = std::bind(&Stratum::processReply, this, _1);
      if (rpc->registerReceiveCallback(callbackFunctor))
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
    //If the id is null, it may be the client sending a command without id.
    if (!r["id"].is_null())
      id = r["id"];
    else
      id = -1;

    cout <<  "RECV:" << r.dump() << endl;
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

        //Authorize
        json q;
        q["id"] = 111;
        q["method"] = "mining.authorize";
        q["params"] = {"99eed232c4749a8bbc505ba7fe9c21fd7261d92438d2a2d4c3069ddc72f4b1cafa21cf0421af", ""};


        if (rpc->sendQuery(q)) {
          cout << "sent auth" << endl;
          sentQueries.push_back(StratumQuery(StratumMethod::miningAuthorize, q["id"]));
        }
      }
      if (method == StratumMethod::miningAuthorize) {
        //HMMMM We can't get this yet, this is weird. We receive a zero-error reply though.
        cout << "auth reply1" << endl;
        //RPC gave us a valid response to the mining subscription query
        if ( r["error"].is_null() ) {
          cout << "auth reply:" <<  r.dump() << endl;

          /* //mining details are in here:
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
          }*/
          //We'll shut down  after we're authorized
          state = StratumState::disconnected;
        }
      }

    }

    //Else it must be server command without an id
    if (isQuery(r) && r["method"] == "client.get_version") {
      json q;
      q["id"] = r["id"]; //answer with same id
      q["error"] = nullptr;
      q["result"] = {"xiaminer 0.1"};


      if (rpc->sendQuery(q));//we're actually sending a reply here, so no reply is expected.
    }
    if (isQuery(r) && r["method"] == "mining.set_difficulty") {
      json q;
      q["id"] = r["id"]; //answer with same id
      q["error"] = nullptr;
      q["result"] = {}; //empty array probably good enough

      float difficulty = r["params"][0];
      cout << "Received difficulty update:" << difficulty << endl;
      if (rpc->sendQuery(q));//we're actually sending a reply here, so no reply is expected.
    }
    if (isQuery(r) && r["method"] == "mining.notify") {

      json q;
      q["id"] = r["id"]; //answer with same id
      q["error"] = nullptr;
      q["result"] = {}; //empty array probably good enough

      if (rpc->sendQuery(q));//we're actually sending a reply here, so no reply is expected.

      SiaJob sj;


      //Got piece of work
      sj.jobID =  r["params"][0]; //string
      sj.prevHash = r["params"][1]; //hexStringToBytes
      sj.coinb1 =   r["params"][2]; //hexStringToBytes
      sj.coinb2 =   r["params"][3]; //hexStringToBytes
      for (auto& el : r["result"][4]) { //merkle branches, hexStringToBytes
        sj.merkleBranches.push_back(el);
      }
      sj.blockVersion = r["params"][5]; //string
      sj.nBits = r["params"][6]; //string
      sj.nTime = r["params"][7]; //hexStringToBytes
      sj.cleanJobs = r["params"][8]; //bool
      cout << "Got notify: " << sj.jobID << ", " << sj.nTime << endl;

      jobs.push(sj);
    }
  }
  bool isSubscribed() {
    return state == StratumState::subscribed;

  }
  bool isSetup() {
    return state == StratumState::setup;

  }

};


#endif
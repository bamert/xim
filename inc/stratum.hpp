/*
  Manages communication with a SIA stratum server
 */
#ifndef __NDB_SIASTRATUM
#define __NDB_SIASTRATUM
#include <iostream>
#include <thread>
#include <functional>
#include <queue>

#include "blake2b.hpp"
#include "json.hpp"
#include "tcp.hpp"
#include "rpc.hpp"
#include "bigmath.hpp"
#include "miner.hpp"

using namespace std;
using json = nlohmann::json;


// To keep the queries that we sent to server
enum StratumMethod {miningAuthorize, miningSubscribe, miningSubmit, miningSetDifficulty, clientGetVersion, none};

struct StratumQuery {
  int method;
  int id;
  StratumQuery(int method, int id) : method(method), id(id) {}
};




class Stratum {
 private:
  /*mining Address*/
  std::string miningAddress;

  enum StratumState  {setup, setupFailed, subscribing, subscribed, connectFailed, disconnected};
  int state;
  RPCConnection* rpc;
  std::thread* stratumThread;
  /* keeps the sent queries such that we can reidentify the responses*/
  std::vector<StratumQuery> sentQueries;

  /*Keeps the callback functor for RPC calls*/
  std::function<void(json)> callbackFunctorRpc;
  /*Keeps the callback functor for mining results*/
  std::function<void(SiaJob)> callbackFunctorMining;

  std::string miningDifficulty;
  std::vector<uint8_t> extraNonce1;

  ExtraNonce2 en2;
  Target miningTarget;

  /*Keeps the miner*/
  Miner& miner;
  //Checks if a query with given id has been sent
  int sentQueryWithId(const int id) {
    int method = StratumMethod::none;

    for (int i = 0; i < sentQueries.size(); i++) {
      if (sentQueries[i].id == id) {
        method = sentQueries[i].method;
        sentQueries.erase(sentQueries.begin() + i);
      }
    }
    return method;

  }

  //check if a message is a query to the client or an answer from the server
  bool isQuery(const json& q) {
    return q.find("method") != q.end();
  }
  bool isReply(const json& q) {
    return q.find("error") != q.end();
  }
 public:
  Stratum(std::string address, int port, std::string miningAddress, Miner& miner) : miner(miner) {
    this->miningAddress = miningAddress;
    rpc = new RPCConnection(address, port);
    //if connectioned succeeded, setup receive callback
    if (rpc->isConnected()) {
      using namespace std::placeholders; //for _1
      callbackFunctorRpc = std::bind(&Stratum::processReply, this, _1);
      if (rpc->registerReceiveCallback(callbackFunctorRpc))
        cout << "rpc callback handler added" << endl;
    }

    using namespace std::placeholders; //for _1

    //Get  miner  up and running:
    callbackFunctorMining = std::bind(&Stratum::submitHeader, this, _1);
    if (miner.registerMiningResultCallback(callbackFunctorMining))
      cout << "mining callback handler added" << endl;
  }
  ~Stratum() {
    delete rpc; //terminates tcp connection thread
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
      q["id"] = rpc->getNewId();
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

    //check if this id is present in the list of our sent queries
    method = sentQueryWithId(id);
    if (method != StratumMethod::none) {
      //We got an answer to our subscribe query.
      if (method == StratumMethod::miningSubscribe) {
        std::string miningNotify;

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
          Bigmath bigmath;
          extraNonce1 = bigmath.hexStringToBytes(r["result"][1]);
          en2.size = r["result"][2];
          en2.val = 0;
        }

        //Authorize
        json q;
        q["id"] = rpc->getNewId();
        q["method"] = "mining.authorize";
        q["params"] = {miningAddress, ""};


        if (rpc->sendQuery(q)) {
          cout << "sent auth" << endl;
          sentQueries.push_back(StratumQuery(StratumMethod::miningAuthorize, q["id"]));
        }
      }
      if (method == StratumMethod::miningAuthorize) {
        //RPC gave us a valid response to the mining subscription query
        if ( r["error"].is_null() ) {
          cout << "auth reply: ok" << endl;
        }
      }
      if (method == StratumMethod::miningSubmit) {
        cout << "Submission reply:" << endl << r.dump() << endl;
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

      double difficulty = r["params"][0];
      cout << "Received difficulty update:" << difficulty << endl;

      Bigmath bigmath;
      miner.setTarget(difficulty);


      //cout << "mining difficulty:" << bigmath.toHexString(miner->getTarget()) << endl;

      if (rpc->sendQuery(q));//we're actually sending a reply here, so no reply is expected.
    }
    if (isQuery(r) && r["method"] == "mining.notify") {

      json q;
      q["id"] = r["id"]; //answer with same id
      q["error"] = nullptr;
      q["result"] = {}; //empty array probably good enough

      if (rpc->sendQuery(q));//we're actually sending a reply here, so no reply is expected.

      processMiningNotify(r);


    }
  }
  //This has a seperate method for testing purposes. Here we can inject a
  //json package that we have the solution for (i.e. collected with wireshark)
  void processMiningNotify(json& r) {
    SiaJob sj;
    Bigmath bigmath;

    //Got piece of work
    sj.jobID =  r["params"][0]; //string
    sj.prevHash = bigmath.hexStringToBytes(r["params"][1]); //hexStringToBytes
    sj.coinb1 =  bigmath.hexStringToBytes(r["params"][2]); //hexStringToBytes
    sj.coinb2 =  bigmath.hexStringToBytes(r["params"][3]); //hexStringToBytes
    for (auto& el : r["params"][4]) { //merkle branches, hexStringToBytes
      sj.merkleBranches.push_back(bigmath.hexStringToBytes(el));
    }
    sj.blockVersion = r["params"][5]; //string
    sj.nBits = r["params"][6]; //string
    sj.nTime = bigmath.hexStringToBytes(r["params"][7]); //hexStringToBytes
    sj.cleanJobs = r["params"][8]; //bool
    cout << "job " << sj.jobID << " received" << endl;
    sj.extranonce2 = en2.bytes();
    cout << "adding job with en1:" << bigmath.toHexString(extraNonce1) << ", en2:" << bigmath.toHexString(sj.extranonce2) << endl;

    en2.increment();
    miner.computeHeader(sj, extraNonce1);

    //cout << "nbits:" << sj.nBits << endl;
    //Set network target from nbits
    sj.target.fromNbits(bigmath.hexStringToBytes(sj.nBits));
    // cout << "network difficulty:" << bigmath.toHexString(sj.target.value) << endl;

    miner.addJob(sj);
  }
  //

  void submitHeader(SiaJob sj) {
    Bigmath bigmath;
    //extract nonce that we found from header
    std::vector<uint8_t> hdrNonce(sj.header.begin() + 32, sj.header.begin() + 40);
    std::vector<uint8_t> extraNonce2(sj.header.begin() + 32, sj.header.begin() + 40);

    //Get human-readable hex representation of everything
    //hdrNonce[3]++;
    std::string nonce = bigmath.toHexString(hdrNonce);
    std::string en2hex = bigmath.toHexString(sj.extranonce2);
    std::string nTime = bigmath.toHexString(sj.nTime);
    json q;
    q["id"] = rpc->getNewId();
    q["method"] = "mining.submit";
    q["params"] = {miningAddress, sj.jobID, en2hex, nTime, nonce};

    cout << "submitting job with en2:" << bigmath.toHexString(sj.extranonce2) << endl;

    if (rpc->sendQuery(q)) {
      cout << "job " << sj.jobID << " sent solution (id=" << q["id"]  << ")" << endl;
      sentQueries.push_back(StratumQuery(StratumMethod::miningSubmit, q["id"]));
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
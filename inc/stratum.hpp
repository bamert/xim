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
  std::vector<uint8_t> prevHash;
  std::vector<uint8_t> coinb1;
  std::vector<uint8_t> coinb2;
  std::vector<std::vector<uint8_t>> merkleBranches;
  std::string blockVersion;
  std::string nBits;
  std::vector<uint8_t> nTime;
  std::vector<uint8_t> header;
  Target target; //256bit Bigint
  bool cleanJobs;
};

struct ExtraNonce2 {
  uint32_t val;
  uint32_t size;
  ExtraNonce2(): val(0), size(4) {}
  ExtraNonce2(uint32_t val, uint32_t size) : val(val), size(size) {}
  void increment() {
    val++;
  }
  std::vector<uint8_t> bytes() { //get size-byte representation of value
    std::vector<uint8_t> out(size);
    for (int i = 0; i < 4; i++)
      out[size - 1 - i] = (val >> (i * 8));
    return out;
  }
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


  /*Keeps the callback functor*/
  std::function<void(json)> callbackFunctor;

  /* Keeps the jobs*/
  std::queue<SiaJob> jobs;

  std::string miningDifficulty;
  std::vector<uint8_t> extraNonce1;

  ExtraNonce2 en2;

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
  Stratum(std::string address, int port, std::string miningAddress) {
    this->miningAddress = miningAddress;
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
    uint8_t buffer[2048];
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

        cout << "notify:" << miningNotify << ", difficulty:" << miningDifficulty << ", extraNonce2size:" <<
             en2.size << endl;

        //Authorize
        json q;
        q["id"] = 111;
        q["method"] = "mining.authorize";
        q["params"] = {miningAddress, ""};


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
      Target target;
      Bigmath bigmath;
      target.fromDifficulty(difficulty);

      cout << "mining difficulty:" << bigmath.toHexString(target.value) << endl;

      if (rpc->sendQuery(q));//we're actually sending a reply here, so no reply is expected.
    }
    if (isQuery(r) && r["method"] == "mining.notify") {

      json q;
      q["id"] = r["id"]; //answer with same id
      q["error"] = nullptr;
      q["result"] = {}; //empty array probably good enough

      if (rpc->sendQuery(q));//we're actually sending a reply here, so no reply is expected.

      SiaJob sj;
      Bigmath bigmath;

      //Got piece of work
      sj.jobID =  r["params"][0]; //string
      sj.prevHash = bigmath.hexStringToBytes(r["params"][1]); //hexStringToBytes
      sj.coinb1 =  bigmath.hexStringToBytes(r["params"][2]); //hexStringToBytes
      sj.coinb2 =  bigmath.hexStringToBytes(r["params"][3]); //hexStringToBytes
      for (auto& el : r["result"][4]) { //merkle branches, hexStringToBytes
        sj.merkleBranches.push_back(bigmath.hexStringToBytes(el));
      }
      sj.blockVersion = r["params"][5]; //string
      sj.nBits = r["params"][6]; //string
      sj.nTime = bigmath.hexStringToBytes(r["params"][7]); //hexStringToBytes
      sj.cleanJobs = r["params"][8]; //bool
      cout << "Got notify: " << sj.jobID << endl;

      /* Compute difficulty target */
      auto append = [](uint8_t* buf, std::vector<uint8_t> src) {
        for (int i = 0; i < src.size(); i++)
          buf[i] = src[i];
        return src.size();
      };
      int offset = 1;
      buffer[0] = 0; //has to be a zero
      offset += append(&buffer[offset], sj.coinb1);
      offset += append(&buffer[offset], extraNonce1);
      offset += append(&buffer[offset], en2.bytes());
      en2.increment();//update extranonce2
      offset += append(&buffer[offset], sj.coinb2);

      //hash [0,offset] on buffer
      Blake2b b2b;
      unsigned char hash[32];
      b2b.sia_gen_hash(buffer, offset, hash);

      std::vector<uint8_t> one;
      one.push_back(0x01);

      uint8_t merkleRoot[32]; //256bit
      memcpy(merkleRoot, hash, 32);

      for (auto el : sj.merkleBranches) {
        //merkleRoot = blake2b('\x01' + binascii.unhexlify(h) + merkle_root, digest_size = 32).digest();
        offset = 0;
        offset += append(&buffer[offset], one);//one
        offset += append(&buffer[offset], el); //markle branch
        memcpy(&buffer[offset], merkleRoot, 32); //256bit previous merkleRoot
        b2b.sia_gen_hash(buffer, offset + 32, merkleRoot); //output val to merkleRoot
      }
      //cout << "Finished MerkleRoot:" << endl << merkleRoot << endl;
      // cout << "offset (should be 32)" << offset << endl;
      uint8_t header[80];
      offset = 0;
      offset += append(&buffer[offset], sj.prevHash);
      offset += append(&buffer[offset], {0, 0, 0, 0, 0, 0 , 0, 0});
      offset += append(&buffer[offset], sj.nTime);
      memcpy(&buffer[offset], merkleRoot, 32);

      cout << "header:" << offset << endl;
      for (int i = 0; i < 80; i++)
        cout << buffer[i];
      cout << endl;

      //Add header to current job
      sj.header = bigmath.bufferToVector(buffer, 80);

      cout << "nbits:" << sj.nBits << endl;
      //Set target from nbits
      sj.target.fromNbits(bigmath.hexStringToBytes(sj.nBits));
      cout << "network difficulty:" << bigmath.toHexString(sj.target.value) << endl;

      jobs.push(sj);

      //return this header to server
      // submitHeader(sj);
    }
  }
  void submitHeader(SiaJob& sj) {
    Bigmath bigmath;
    //extract nonce that we found from header
    std::vector<uint8_t> hdrNonce(sj.header.begin() + 32, sj.header.begin() + 40);
    //Get human-readable hex representation of everything
    hdrNonce[3]++;
    std::string nonce = bigmath.toHexString(hdrNonce);
    std::string en2hex = bigmath.toHexString(en2.bytes());
    std::string nTime = bigmath.toHexString(sj.nTime);

    json q;
    q["id"] = 1561;
    q["method"] = "mining.submit";
    q["params"] = {miningAddress, sj.jobID, en2hex, nTime, nonce};


    if (rpc->sendQuery(q)) {
      cout << "sent solution" << endl;
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
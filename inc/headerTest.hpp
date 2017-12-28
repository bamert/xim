#ifndef __NDB_HEADERTEST
#define __NDB_HEADERTEST

#include <iostream>
#include <functional>
#include "bigmath.hpp"
#include "miner.hpp"
#include "json.hpp"


/**
 * @brief      Tests construction of a valid header based on a valid transaction and share captured on wireshark.
 */
class HeaderTest {
 private:
  ExtraNonce2 en2;
  std::vector<uint8_t> extraNonce1;
  Miner* miner;
  std::function<void(SiaJob)> callbackFunctorMining;

 public:
  HeaderTest() {
    miner = new Miner;
    using namespace std::placeholders; //for _1

    callbackFunctorMining = std::bind(&HeaderTest::reportSolution, this, _1);
    if (miner->registerMiningResultCallback(callbackFunctorMining))
      cout << "mining callback handler added" << endl;


    json msg[4];

    // (Contains the extranonce1: 080109a0 and extranonce2 size: 4)
    msg[0] = json::parse("{\"id\": 1, \"error\": null, \"result\": [[[\"mining.notify\", \"080109a073195693\"], [\"mining.set_difficulty\", \"080109a0731956932\"]], \"080109a0\", 4]}");

    //(Last difficulty update before the notification containing the work that lead to a valid share)
    msg[1] = json::parse("{\"id\": null, \"method\": \"mining.set_difficulty\", \"params\": [0.99998474121094105]}");

    //(Mining details used to build header of successful share)
    msg[2] = json::parse("{\"id\": null, \"method\": \"mining.notify\", \"params\": [\"4e76\", \"000000000000001f92628fdadd8d1b35758f88662148354d528546aef958da13\", \"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000010000000000000020000000000000004e6f6e536961000000000000000000009ef9435a00000000\", \"0000000000000000\", [\"96a635ca160fa19d5e0c92b3b6a439dd793343930665a6baa0d9f3621a3533f2\", \"4fdbb48f48977335941cd315362f80ea4aed31946ab3a6e36f7396136edf5eb8\", \"01177cbd19363d35621247e837699a57eeb22898b13ef26e8f1225637d0a90f0\", \"a5d5faed34201c1797ba5d80a9af8fcafa2304d332e6bd30adaf411b0cfb1a59\"], \"\", \"1921292f\", \"94f9435a00000000\", false]}");

    //(Successful share submission. This contains the successful nonce.)
    msg[3] = json::parse("{\"id\": 4, \"method\": \"mining.submit\", \"params\": [\"99eed232c4749a8bbc505ba7fe9c21fd7261d92438d2a2d4c3069ddc72f4b1cafa21cf0421af\", \"4e76\", \"06000000\", \"94f9435a00000000\", \"eea81a3600000000\"]}");

    //Set the target difficulty (Message 1)
    miner->setTarget(0.99998474121094105);

    //Set extranonce 1 and en2 size (Message 0)
    Bigmath bigmath;
    extraNonce1 = bigmath.hexStringToBytes(msg[0]["result"][1]);
    en2.size = msg[0]["result"][2];
    en2.val = 6; //from message0

    SiaJob test;

    test.jobID = "HeaderTest";
    test.offset = 0xeea81a36; //msg3 contains the valid nonce we found: eea81a3600000000, we start searching right at that element.
    test.intensity = 1; //Don't mine. We know the valid nonce, no need to scan the nonce space.

    //Mining details (msg 2)
    //test.jobID =  r["params"][0]; //string
    test.prevHash = bigmath.hexStringToBytes(msg[2]["params"][1]); //hexStringToBytes
    test.coinb1 =  bigmath.hexStringToBytes(msg[2]["params"][2]); //hexStringToBytes
    test.coinb2 =  bigmath.hexStringToBytes(msg[2]["params"][3]); //hexStringToBytes
    for (auto& el : msg[2]["params"][4]) { //merkle branches, hexStringToBytes
      test.merkleBranches.push_back(bigmath.hexStringToBytes(el));
    }
    test.blockVersion = msg[2]["params"][5]; //string
    test.nBits = msg[2]["params"][6]; //string
    test.nTime = bigmath.hexStringToBytes(msg[2]["params"][7]); //hexStringToBytes
    test.cleanJobs = msg[2]["params"][8]; //bool

    cout << "coinb1:" << bigmath.toHexString(test.coinb1) << endl;
    cout << "en1:" << bigmath.toHexString(extraNonce1) << endl;
    cout << "en2:" << bigmath.toHexString(en2.bytes()) << endl;
    cout << "coinb2:" << bigmath.toHexString(test.coinb2) << endl;

    cout << "en2 bytes:" << bigmath.toHexString(en2.bytes());
    cout << "nTime:" << bigmath.toHexString(test.nTime) << endl;
    cout << "job " << test.jobID << " received" << endl;

    test.extranonce2 = en2.bytes();
    miner->computeHeader(test, extraNonce1);

    //cout << "nbits:" << sj.nBits << endl;
    //Set network target from nbits
    test.target.fromNbits(bigmath.hexStringToBytes(test.nBits));

    miner->addJob(test);

  }
  void reportSolution(SiaJob sj) {
    //Don't do anything here. We already output the hash and know if it fits the given target.
    cout << "got solution for job:" << sj.jobID << endl;
  };
};

#endif
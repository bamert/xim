#ifndef __NDB_MINER
#define __NDB_MINER
#include <iostream>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include "blake2b.hpp"
#include "bigmath.hpp"
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

class Miner {
 private:
  std::thread* thread;
  bool threadRunning;
  /*Keeps the mining target difficulty*/
  Target miningTarget;
  /* Keeps the jobs*/
  std::queue<SiaJob> jobs;

  /* mutex to add jobs */
  std::mutex mtx;

 public:
  Miner() {

  }
  ~Miner() {
    cout << "Killing thread" << endl;
    //send kill signal
    threadRunning = false;
    //join
    thread->join();
    delete thread;
  }
  void addJob(SiaJob& sj) {
    cout << "added job to queue" << endl;
    mtx.lock();
    jobs.push(sj);
    mtx.unlock();
  }
  std::vector<uint8_t> getTarget() {
    return miningTarget.value;
  }
  void setTarget(double difficulty) {
    miningTarget.fromDifficulty(difficulty);
  }
  bool registerMiningResultCallback(std::function<void(SiaJob)>& submitHeaderr) {
    //attach mining thread
    threadRunning = true;
    thread = new std::thread([&]() {
      Blake2b b2b;
      Bigmath bigmath;
      //keep checking if there is work,
      //mine it,  report results
      //
      //
      //
      //Conundrum:
      //Here we need a callback to stratum to push work results
      //yet stratum also needs a way to call mine to deliver work.
      //How to implement this two-way street?
      cout << "starting to mine:" << endl;
      while (threadRunning) {
     //  cout << "K" << endl; //currently need this to actually mine, I don't know why. -_-
        mtx.lock();
        bool jobsEmpty = jobs.empty();
        mtx.unlock();
        if (!jobsEmpty) {
          cout << "running job" << endl;

          SiaJob sj = jobs.front();
          jobs.pop();

          //Mine work, send result if there is oen
          //Mine this first one right here.
          uint8_t header[80];
          //copy header
          for (int i = 0; i < 80; i++)
            header[i] = sj.header[i];

          uint8_t headerOut[80];
          uint8_t hash[32];
          uint32_t n = 0;
          uint32_t maxNonce = 0x1 << 28;//10000000;

          bool found = false;
          for (n = 0; n < maxNonce; n++) {
            if (n % 1000000 == 0)
              cout << 100.*n / float(maxNonce) << "percent" << '\r';

            //insert nonce
            header[32] = (n >> 24) & 0xFF;
            header[33] = (n >> 16) & 0xFF;
            header[34] = (n >> 8) & 0xFF;
            header[35] = (n) & 0xFF;
            //set second half of nonce to zero (would be 64bit!) (imho I already did this above, but double-check)
            header[36] = 0;
            header[37] = 0;
            header[38] = 0;
            header[39] = 0;
            //set nbits 0 (cpuminer does that)
            //header[44] = 0;
            //header[45] = 0;
            //header[46] = 0;
            //header[47] = 0;
            //hash
            b2b.sia_gen_hash(header, 80, hash);

            //check output

            //if (hash[0] < miningTarget.value[31]) { //only check all 256 bits if the first byte is already smaller.
            //   cout << "candidate" << endl;

            for (int i = 0; i < 31; i++) {
              //  cout << "a";
              if (hash[31 - i] < miningTarget.value[i]) { //we handle the reverse byte order in here.
                found = true;
                //  cout << "b" << endl;
                break;
              }
              if (hash[31 - i] > miningTarget.value[i]) {
                found = false;
                break;
              }
            }
            if (found == true)break;


          } //end nonce loop
          if (found == true) {
            cout << "found match:" << bigmath.toHexString(hash, 32) << endl;
            cout << "target    :" << bigmath.toHexString(miningTarget.value) << endl;
            cout << "n:" << n << endl;
            //copy result into header
            sj.header[32] = header[32];
            sj.header[33] = header[33];
            sj.header[34] = header[34];
            sj.header[35] = header[35];
            sj.header[36] = header[36];
            sj.header[37] = header[37];
            sj.header[38] = header[38];
            sj.header[39] = header[39];
            //return this header to server
            submitHeaderr(sj);
          } else {
            cout << "didn't find match" << endl;
          }
        }//job queue empty check
      } //thread loop
    });//mining thread
    return true;
  }
};
#endif
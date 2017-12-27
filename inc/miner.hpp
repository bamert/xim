#ifndef __NDB_MINER
#define __NDB_MINER
#include <iostream>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <chrono>
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
  uint32_t offset;
  std::vector<uint8_t> nTime;
  std::vector<uint8_t> header;
  Target target; //256bit Bigint
  bool cleanJobs;
  SiaJob() {offset = 0;}
};
static inline uint32_t le32dec(const void *pp) {
  const uint8_t *p = (uint8_t const *)pp;
  return ((uint32_t)(p[0]) + ((uint32_t)(p[1]) << 8) +
          ((uint32_t)(p[2]) << 16) + ((uint32_t)(p[3]) << 24));
}

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
  bool isMining() {
    return threadRunning;
  }
  void addJob(SiaJob sj) {
    mtx.lock();
    jobs.push(sj);
    mtx.unlock();
  }
  std::vector<uint8_t> getTarget() {
    return miningTarget.value;
  }
  void setTarget(double difficulty) {
    mtx.lock();
    miningTarget.fromDifficulty(difficulty);
    //convert to little endian
    /*for (int i = 0; i < 32; i += 4) {
      uint8_t a =  miningTarget.value[i + 3];
      uint8_t b =  miningTarget.value[i + 2];
      uint8_t c =  miningTarget.value[i + 1];
      uint8_t d =  miningTarget.value[i];

      miningTarget.value[i] = a;
      miningTarget.value[i + 1] = b;
      miningTarget.value[i + 2] = c;
      miningTarget.value[i + 3] = d;
    }*/

    mtx.unlock();
  }
  bool registerMiningResultCallback(std::function<void(SiaJob)>& submitHeaderr) {
    //attach mining thread
    threadRunning = true;
    thread = new std::thread([&]() {
      Blake2b b2b;
      Bigmath bigmath;
      while (threadRunning) {
        //  cout << "K" << endl; //currently need this to actually mine, I don't know why. -_-
        mtx.lock();
        bool jobsEmpty = jobs.empty();
        mtx.unlock();
        if (!jobsEmpty) {
          mtx.lock();
          SiaJob sj = jobs.front();
          jobs.pop();
          mtx.unlock();
          cout << "job "  << sj.jobID << " running, ";
          //Mine work, send result if there is oen
          //Mine this first one right here.
          uint8_t header[80];
          //copy header
          for (int i = 0; i < 80; i++)
            header[i] = sj.header[i];

          uint8_t headerOut[80];
          uint8_t hash[32];
          uint32_t nonce = 0;
          uint32_t minNonce = sj.offset; //start mining at given offset.
          uint32_t intensity = 0x1 << 28;//;

          bool found = false;
          mtx.lock();
          Target target = miningTarget;
          mtx.unlock();
          //cout << "inhdr :" << bigmath.toHexString(header, 80) << endl;
          cout << "target:" << bigmath.toHexString(target.value);
          cout << " , intensity:" << intensity << endl;
          //cout << "offset:" << minNonce << endl;
          auto now = std::chrono::high_resolution_clock::now();
          auto prev  = std::chrono::high_resolution_clock::now();
          for (uint32_t i = 0 ; i < intensity; i++) {
            //update range scanning status.
            if (i % 10000 == 0) {
              now = std::chrono::high_resolution_clock::now();
              std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(now - prev);
              double secs = time_span.count();
              double hashRate = 10000 / secs;
              cout << 100.*i / float(intensity) << "percent," <<  hashRate/1000000 << "MH/s   \r";
              prev = now;
            }


            nonce = i + minNonce;
            //we just pick the correct nonce direclty:
            //nonce1 88, 47, 107, 95,
            //nonce2 7, 235, 26, 63,
            //nonce = 0x582F6B5F; //expected result1 offset=5 * (28 << 1), intensity=28 <<1. maxReach = offset+intensity = 6 * (28 << 1) = 1610612736 nonce: 1479502687. nonce<maxReach: should be possible to find this!
            //nonce = 0x07EB1A3F; //expeted result2. offset=805306368, intensity=28<<1. maxReach = offset+intensity = 805306368 + 28<<1 = 1073741824. nonce=132848191. nonce<maxReach, but nonce<offset. not in scan range!

            //insert nonce big endian. (endianness doesn't really matter since we later just submit the header as-is)
            header[32] = (nonce >> 24) & 0xFF;
            header[33] = (nonce >> 16) & 0xFF;
            header[34] = (nonce >> 8) & 0xFF;
            header[35] = (nonce) & 0xFF;

            //change endianness of entire buffer in-place
            le32array(header,80);
            //hash
            b2b.sia_gen_hash(header, 80, hash);

            //check output




            for (int i = 0; i < 32; i++) {
              if (hash[i] < target.value[i]) { //we handle the reverse byte order in here.
                found = true;
                break;
              }
              if (hash[i] > target.value[i]) {
                found = false;
                break;
              }
            }

            if (found == true)break;

          } //end nonce loop
          if (found == true) {
            cout << "job " << sj.jobID << " found match:" << bigmath.toHexString(hash, 32) << endl;
            cout << "target    :" << bigmath.toHexString(target.value) << endl;
            cout << "nonce:" << nonce << endl;
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
            cout << "job " << sj.jobID << " didn't find match" << endl;
          }
        }//job queue empty check
      } //thread loop
    });//mining thread
    return true;
  }
};
#endif
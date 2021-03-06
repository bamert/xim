#ifndef __NDB_MINER
#define __NDB_MINER
#include <iostream>
#include <thread>
#include <queue>
#include <stack>
#include <functional>
#include <mutex>
#include <chrono>
#include "blake2b.hpp"
#include "blake2CPU.hpp"
#include "bigmath.hpp"


static inline uint32_t le32dec(const void *pp) {
  const uint8_t *p = (uint8_t const *)pp;
  return ((uint32_t)(p[0]) + ((uint32_t)(p[1]) << 8) +
          ((uint32_t)(p[2]) << 16) + ((uint32_t)(p[3]) << 24));
}
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
      out[i] = (val >> (i * 8));
    return out;
  }
};

//Build a job from the information we've received
struct SiaJob {
  int extraNonce2size;
  std::vector<uint8_t>
  extranonce2; //keeps the current extranonce2 (we need it again for the submission)
  std::string jobID;
  std::vector<uint8_t> prevHash;
  std::vector<uint8_t> coinb1;
  std::vector<uint8_t> coinb2;
  std::vector<std::vector<uint8_t>> merkleBranches;
  std::string blockVersion;
  std::string nBits;
  uint32_t offset; //Where to start searching the nonce space.
  uint32_t intensity;//The intensity at which we mine (i.e. the range of nonce to search before we (optionally) change the header)
  std::vector<uint8_t> nTime;
  std::vector<uint8_t> header;
  Target target; //256bit Bigint (network target)
  bool cleanJobs;
  SiaJob() {offset = 0; intensity = 0x1 << 28;}
};

class Miner {
 private:
  std::thread* thread;
  bool threadRunning;
  /*Keeps the mining target difficulty*/
  Target miningTarget;

  /* Keeps the jobs*/
  std::stack<SiaJob> jobs;

  /* mutex to add jobs */
  std::mutex mtx;


  int nThreads = 1;
  int intensity = 0x1 << 28;


 public:
  Miner(int nThreads, int intensity) : nThreads(nThreads), intensity(intensity) {

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
    sj.intensity = 0x1 << intensity;
    jobs.push(sj);
    mtx.unlock();
  }
  std::vector<uint8_t> getTarget() {
    return miningTarget.value;
  }
  void setTarget(double difficulty) {
    mtx.lock();
    miningTarget.fromDifficulty(difficulty);
    mtx.unlock();
  }
  void computeHeader(SiaJob& sj, std::vector<uint8_t> extraNonce1) {
    uint8_t buffer[2048];
    std::vector<uint8_t> tmp;
    Bigmath bigmath;

    /* Compute difficulty target */
    auto append = [](uint8_t* buf, std::vector<uint8_t> src)  {
      for (int i = 0; i < src.size(); i++)
        buf[i] = src[i];
      return src.size();
    };

    int offset = 1;
    buffer[0] = 0; //has to be a zero
    offset += append(&buffer[offset], sj.coinb1);
    offset += append(&buffer[offset], extraNonce1);
    offset += append(&buffer[offset], sj.extranonce2);
    offset += append(&buffer[offset], sj.coinb2);

    tmp.assign(buffer, buffer + offset);

    Blake2b b2b;
    uint8_t mhash[32];
    b2b.sia_gen_hash(buffer, offset, mhash);

    uint8_t merkleRoot[32]; //256bit
    memcpy(merkleRoot, mhash, 32);

    for (auto el : sj.merkleBranches) {
      offset = 1;
      buffer[0] = 1;
      offset += append(&buffer[offset], el); //markle branch
      memcpy(&buffer[offset], merkleRoot, 32); //256bit previous merkleRoot
      b2b.sia_gen_hash(buffer, offset + 32, merkleRoot); //output val to merkleRoot
    }
    uint8_t header[80];
    offset = 0;
    offset += append(&header[offset], sj.prevHash);
    offset += append(&header[offset], {0, 0, 0, 0, 0, 0 , 0, 0}); //where we aer gonna put our trial nonce
    offset +=  append(&header[offset], sj.nTime);
    memcpy(&header[offset], merkleRoot, 32);

    //Add header to current job
    sj.header = bigmath.bufferToVector(header, 80);
  }
  bool registerMiningResultCallback(std::function<void(SiaJob)>& submitHeaderr) {
    //attach mining thread
    threadRunning = true;
    thread = new std::thread([&]() {
      Blake2b b2b;
      Bigmath bigmath;
      while (threadRunning) {
        mtx.lock();
        bool jobsEmpty = jobs.empty();
        mtx.unlock();
        if (!jobsEmpty) {
          mtx.lock();
          SiaJob sj = jobs.top();
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
          uint32_t intensity = sj.intensity;//;

          bool found = false;
          mtx.lock();
          Target target = miningTarget;
          mtx.unlock();
          cout << " intensity:" << intensity << endl;


          ndb::Blake2bCPU b2bcpu(nThreads);
          uint32_t nonceOut;

          found = b2bcpu.hashRange(header, sj.offset, sj.offset + sj.intensity, target.value, &nonceOut);

          if (found == true) {
            header[32] = (nonceOut >> 24) & 0xFF;
            header[33] = (nonceOut >> 16) & 0xFF;
            header[34] = (nonceOut >> 8) & 0xFF;
            header[35] = (nonceOut) & 0xFF;
            cout << "job " << sj.jobID << " found match:" << bigmath.toHexString(hash, 32) << endl;
            cout << "target    :" << bigmath.toHexString(target.value) << endl;
            cout << "nonce:" << nonceOut << endl;
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

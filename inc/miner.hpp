#ifndef __NDB_MINER
#define __NDB_MINER
#include <iostream>
#include <thread>

class Miner {
 private:
 std::thread* thread;
 //mining thread
 void mine(void){
  //keep checking if there is work,
  //mine it,  report results
  //
  //
  //
  //Conundrum: 
  //Here we need a callback to stratum to push work results
  //yet stratum also needs a way to call mine to deliver work.
  //How to implement this two-way street?
 }

 public:
  Miner() {
    //attach mining thread
    thread = new std::thread(mine);
  }
  ~Miner(){
    //send kill signal
    //join
    thread.join();
    delete thread;
  }

};
#endif
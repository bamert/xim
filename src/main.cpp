#include <iostream>
#include "json.hpp"
#include "tcp.hpp"
#include "stratum.hpp"
#include "miningTest.hpp"


#include "blake2b.hpp"

#include "blakeTest.hpp"

using namespace std;
using json = nlohmann::json;


int main(int argc, char *argv[]) {
  cout << "ximiner 0.1" << endl;
  Bigmath bm;
  
  //Run Blake2b test:
 // BlakeTest bt;


  //Run mining tests
  MiningTest mt;


/*

  
  Stratum client("eu.siamining.com", 3333, "99eed232c4749a8bbc505ba7fe9c21fd7261d92438d2a2d4c3069ddc72f4b1cafa21cf0421af");

  client.subscribe("myMiningAdress");*/
  int i = 0; //this is just so the while loop doesn't get optimized away
  while (mt.isRunning() ) {
    //define some timeout or commands to terminate..
    //cout << "ugh" << endl;
    usleep(1000 * 100 * 1);
  }
  //server.processReply();
  //client.isSetup();


}
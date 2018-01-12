#include <iostream>
#include "json.hpp"
#include "tcp.hpp"
#include "stratum.hpp"
#include "miningTest.hpp"
#include "miner.hpp"

#include "blake2b.hpp"

#include "blakeTest.hpp"
using namespace std;
using json = nlohmann::json;

//this is an edit by vim :-)
int main(int argc, char *argv[]) {
  cout << "ximiner 0.1" << endl;

  for (auto && par : std::vector<std::string> { argv, argv + argc }) {
    std::cout << "param: " << par << std::endl;
    if (par.substr(0, 10) == "--address=")
      cout << par.substr(10, string::npos) << endl;
    else if (par.substr(0, 7) == "--user=")
      cout << par.substr(10, string::npos) << endl;
    else if (par.substr(0, 12) == "--intensity=")
      cout << par.substr(10, string::npos) << endl;
    else if (par.substr(0, 10) == "--threads=")
      cout << par.substr(10, string::npos) << endl;
  }
//Run Blake2b test:
// BlakeTest bt;


//Run mining tests
// MiningTest mt;




  Miner CPUminer(1);
  Stratum client("eu.siamining.com", 3333,
                 "99eed232c4749a8bbc505ba7fe9c21fd7261d92438d2a2d4c3069ddc72f4b1cafa21cf0421af.xia", CPUminer);

  client.subscribe();
  int i = 0; //this is just so the while loop doesn't get optimized away
  while (client.isSetup()) {//mt.isRunning() ) {
    //define some timeout or commands to terminate..
    //cout << "ugh" << endl;
    usleep(1000 * 100 * 1);
  }
  //server.processReply();
  //client.isSetup();


}

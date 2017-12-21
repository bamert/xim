#include <iostream>
#include "json.hpp"
#include "tcp.hpp"
#include "stratum.hpp"
#include "rpc.hpp"

using namespace std;
using json = nlohmann::json;


int main(int argc, char *argv[]) {
  cout << "ximiner 0.1" << endl;
  Stratum client("eu.siamining.com", 3333);

  client.subscribe("myMiningAdress");
  int i=0;//this is just so the while loop doesn't get optimized away
  while(client.isSetup() ){
    //define some timeout or commands to terminate..
    //cout << "ugh" << endl;
    usleep(1000*100*1); //wait 100ms. No idea why it doesn't work without
  }
  //server.processReply();
  //client.isSetup();
 

}
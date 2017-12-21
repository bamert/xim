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
  while(client.isSubscribed()){
    //define some timeout or commands to terminate..
  }
  //server.processReply();
  client.isSubscribed();
 

}
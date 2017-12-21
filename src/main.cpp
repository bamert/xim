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
  while(client.isSetup() && 1){
    //define some timeout or commands to terminate..
    //cout << "ugh" << endl;
    
  }
cout << "MAYBE WE NEED TO SPEND SOME TIME HERE?????" << i<<endl;
  //server.processReply();
  //client.isSetup();
 

}
#include <iostream>
#include "json.hpp"
#include "tcp.hpp"
#include "stratum.hpp"

#include "blake2b.hpp"

using namespace std;
using json = nlohmann::json;


int main(int argc, char *argv[]) {
  cout << "ximiner 0.1" << endl;
  Bigmath bm;
 /* std::vector<uint8_t>  hs = bm.hexStringToBytes("4b77f25a000000A");
  for(auto e:hs)
    cout << int(e) << ":" << endl;
  cout << "conv back:" << bm.toHexString(hs) << endl;*/
  
  Stratum client("eu.siamining.com", 3333, "99eed232c4749a8bbc505ba7fe9c21fd7261d92438d2a2d4c3069ddc72f4b1cafa21cf0421af");

  client.subscribe("myMiningAdress");
  int i = 0; //this is just so the while loop doesn't get optimized away
  while (client.isSetup() ) {
    //define some timeout or commands to terminate..
    //cout << "ugh" << endl;
    usleep(1000 * 100 * 1); //wait 100ms. No idea why it doesn't work without
  }
  //server.processReply();
  //client.isSetup();


}
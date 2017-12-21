#include <iostream>
#include "json.hpp"
#include "tcp.hpp"
#include "stratum.hpp"

using namespace std;
using json = nlohmann::json;


int main(int argc, char *argv[]) {
  cout << "ximiner 0.1" << endl;
  Stratum server("eu.siamining.com", 3333);

  server.subscribe("myMiningAdress");

  server.isSubscribed();
 

}
#include <iostream>
#include "json.hpp"
#include "tcp.hpp"
#include "stratum.hpp"
#include "miningTest.hpp"


#include "blake2b.hpp"

#include "blakeTest.hpp"
#include "headerTest.hpp"
using namespace std;
using json = nlohmann::json;


int main(int argc, char *argv[]) {
  cout << "ximiner 0.1 - HeaderTest" << endl;
  Bigmath bm;

  //Run Blake2b test:
// BlakeTest bt;


  //Run mining tests
// MiningTest mt;

  //Run Header Test
  HeaderTest ht;



  while (1) {
    //We just let this run
    usleep(1000 * 100 * 1);
  }

}
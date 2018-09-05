#include <iostream>
#include "json.hpp"
#include "tcp.hpp"
#include "stratum.hpp"
#include "miner.hpp"
#include "blake2b.hpp"
using namespace std;
using json = nlohmann::json;

//this is an edit by vim :-)
int main(int argc, char *argv[]) {
  cout << "ximiner 0.1" << endl;
  //Defaults
  std::string url = "eu.siamining.com";
  int port = 3333;
  std::string address = "99eed232c4749a8bbc505ba7fe9c21fd7261d92438d2a2d4c3069ddc72f4b1cafa21cf0421af";
  
  std::string worker="xim";
  int nthreads = 1;
  int intensity = 28;

  //Parse parameters
  for (auto && par : std::vector<std::string> { argv, argv + argc }) {
    if (par.substr(0, 6) == "--url=")
      url = par.substr(6, string::npos);
    else if (par.substr(0, 7) == "--addr=")
      address =  par.substr(7, string::npos);
    else if (par.substr(0,9) == "--worker=")
      worker = par.substr(9, string::npos);
    else if (par.substr(0, 12) == "--intensity=")
      intensity = stoi(par.substr(12, string::npos));
    //else if (par.substr(0, 10) == "--threads=")
      //nthreads = stoi(par.substr(10, string::npos));
  }
  cout << "Connecting to: " << url << ":" << port << endl;
  cout << "Mining to address: " << address << endl;
  cout << "Intensity:" << intensity << endl;
  cout << "Mode: CPU, number of threads:" << nthreads << endl;


  Miner CPUminer(nthreads, intensity);
  
  Stratum client(url, port,address +"." + worker, CPUminer);
  client.subscribe();
  while (client.isSetup()) {
    usleep(1000 * 100 * 1);
  }
}

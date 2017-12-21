#include <iostream>
#include "tcp.hpp"

using namespace std;

//simply contains some sort of accessible tree
class JSONobject {
 public:
  JSONobject(){

  }
  //methods to add items to tree (i.e. add string, add numeral, add...)
  //and same to also get them or check for their existence.
 private:
};

//should offer parse and pack commands.
class JSONparse {
 public:
  JSONparse() {

  }
  JSONobject parse(std::string& json) {
    JSONobject obj;
    for (int i = 0; i < json.length(); i++) {
      uint8_t c = json[i];

      cout << c;
      //detect tokens:
      switch (c) {
      case '{': cout << "obj start" << endl; break;
      case '}': cout << "obj end" << endl; break;
      case '[': cout << "arr start" << endl; break;
      case ']': cout << "arr end" << endl; break;
      case '\"': cout << "string delim" << endl; break;
      case '\n': cout << "new line" << endl; break;

      // case '\\': cout <<
      default: break;
      }
    }
    return obj;
  }
 private:
  enum State {string};
  //need a state stack such that we can push and pop
};


int main(int argc, char *argv[]) {
  cout << "ximiner alive" << endl;
  /*TCPClient tcp;
  tcp.setup("eu.siamining.com", 3333);
  tcp.send("{\"id\": 1, \"method\": \"mining.subscribe\", \"params\": []}\n");
  cout << tcp.read() << endl;*/

  //sample response string to the mining.subscribe query.
  std::string res =
    "{\"id\":1,\"error\":null,\"result\":[[[\"mining.notify\",\"080119004c9ad150\"],[\"mining.set_difficulty\",\"080119004c9ad1502\"]],\"08011900\",4]}";

  JSONparse parser;
  JSONobject obj;
  obj = parser.parse(res);
  cout << endl;

}
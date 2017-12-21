#include <iostream>
#include <functional>
#include <thread>

using namespace std;
class A {
 private:
  std::thread* thread;
 public:
  A() {};

  void registerCallback(std::function<void(int)> callback) {
    thread = new std::thread([&]() {
      /* Does some stuff*/
      callback(1);
    });
    thread->join(); //The issue is that we have to terminate the thread first, before the binary terminates :-)
  }
};
class B {
 private:
  A* a;

 public:
  B() {
    a = new A;
    using namespace std::placeholders; //for _1
    std::function<void(int)> callback = std::bind(&B::callback, this, _1);
    a->registerCallback(callback);
  }
  void callback(int i) {
    cout << "Received " << i << endl;
  }
};

int main() {

  B b;

}
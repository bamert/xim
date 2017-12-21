#include <iostream>
#include <functional>
#include <thread>

using namespace std;
class A {
 private:
  std::thread* t;
  //std::function<void(int)>* callback;
 public:
  A() {};
  ~A() {
    cout << "joining thread" << endl;
    t->join();//callback call causes segfault if we are joining here.
    delete t;
  }

  void registerCallback(std::function<void(int)>& callback) {
    //this->callback = callback;
    t = new std::thread([ & ]() {
      /* Does some stuff*/
      callback(1); 
    });
  }
};
class B {
 private:
  A* a;
  std::function < void(int)> cb;
 public:
  B() {
    a = new A;
    using namespace std::placeholders; //for _1
    cb = std::bind(&B::callback, this, _1);
    a->registerCallback(cb);
  }
  ~B() {
    delete a;
  }
  void callback(int i) {
    cout << "Received " << i << endl;
  }
};

int main() {

  B b;


}
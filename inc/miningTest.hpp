#ifndef __NDB_MININGTEST
#define __NDB_MININGTEST

#include <functional>
#include <iostream>
#include "miner.hpp"
#include "bigmath.hpp"

class MiningTest {
 private:
  /*Keeps the callback functor for mining results*/
  std::function<void(SiaJob)> callbackFunctorMining;
  Miner* miner;
  /*
  var provenSolutions = []struct {
    height          int
    hash            string
    workHeader      []byte
    offset          int
    submittedHeader []byte
    intensity       int
  }{
    {
      height:          56206,
      hash:            "00000000000006418b86014ff54b457f52665b428d5af57e80b0b7ec84c706e5",
      workHeader:      []byte{0, 0, 0, 0, 0, 0, 26, 158, 25, 209, 169, 53, 113, 22, 90, 11, 72, 7, 222, 103, 247, 244, 163, 156, 158, 5, 53, 126, 186, 215, 88, 48, 45, 32, 0, 0, 0, 0, 0, 0, 20, 25, 103, 87, 0, 0, 0, 0, 218, 189, 84, 137, 247, 169, 197, 113, 213, 120, 125, 148, 92, 197, 47, 212, 250, 153, 114, 53, 199, 209, 183, 97, 28, 242, 206, 120, 191, 202, 34, 9},
      offset:          5 * int(math.Exp2(float64(28))),
      submittedHeader: []byte{0, 0, 0, 0, 0, 0, 26, 158, 25, 209, 169, 53, 113, 22, 90, 11, 72, 7, 222, 103, 247, 244, 163, 156, 158, 5, 53, 126, 186, 215, 88, 48, 88, 47, 107, 95, 0, 0, 0, 0, 20, 25, 103, 87, 0, 0, 0, 0, 218, 189, 84, 137, 247, 169, 197, 113, 213, 120, 125, 148, 92, 197, 47, 212, 250, 153, 114, 53, 199, 209, 183, 97, 28, 242, 206, 120, 191, 202, 34, 9},
      intensity:       28,
    },
    {
      height:          57653,
      hash:            "00000000000001ccac64b49a9ebc69c6046a93f4d32d8f8f6967c8f487ed8cec",
      workHeader:      []byte{0, 0, 0, 0, 0, 0, 6, 72, 174, 217, 105, 206, 174, 59, 150, 117, 251, 55, 209, 192, 241, 37, 35, 184, 2, 194, 253, 173, 207, 249, 114, 1, 62, 26, 0, 0, 0, 0, 0, 0, 41, 7, 115, 87, 0, 0, 0, 0, 56, 56, 181, 217, 76, 24, 251, 231, 137, 4, 166, 20, 40, 53, 77, 36, 148, 23, 138, 146, 2, 199, 168, 122, 71, 162, 44, 150, 144, 2, 198, 67},
      offset:          805306368,
      submittedHeader: []byte{0, 0, 0, 0, 0, 0, 6, 72, 174, 217, 105, 206, 174, 59, 150, 117, 251, 55, 209, 192, 241, 37, 35, 184, 2, 194, 253, 173, 207, 249, 114, 1, 7, 235, 26, 63, 0, 0, 0, 0, 41, 7, 115, 87, 0, 0, 0, 0, 56, 56, 181, 217, 76, 24, 251, 231, 137, 4, 166, 20, 40, 53, 77, 36, 148, 23, 138, 146, 2, 199, 168, 122, 71, 162, 44, 150, 144, 2, 198, 67},
      intensity:       28,
    },
  }

  */
 public:
  MiningTest() {
    miner = new Miner;
    using namespace std::placeholders; //for _1

    callbackFunctorMining = std::bind(&MiningTest::reportSolution, this, _1);
    if (miner->registerMiningResultCallback(callbackFunctorMining))
      cout << "mining callback handler added" << endl;


    //We launch two jobs with known solutions (courtesy of Rob Van Miegham's go-miner code
    //at )
    //


    SiaJob tests[2];

    tests[0].jobID = "testcase 1";
    tests[0].offset = 5 * (1 << 28);
    //expected output
    //tests[0].header = {0, 0, 0, 0, 0, 0, 26, 158, 25, 209, 169, 53, 113, 22, 90, 11, 72, 7, 222, 103, 247, 244, 163, 156, 158, 5, 53, 126, 186, 215, 88, 48, 88, 47, 107, 95,  0,  0,   0,  0, 20, 25, 103, 87, 0, 0, 0, 0, 218, 189, 84, 137, 247, 169, 197, 113, 213, 120, 125, 148, 92, 197, 47, 212, 250, 153, 114, 53, 199, 209, 183, 97, 28, 242, 206, 120, 191, 202, 34, 9};
    //input                                                                                                                                                  //->target!
    tests[0].header = {0, 0, 0, 0, 0, 0, 26, 158, 25, 209, 169, 53, 113, 22, 90, 11, 72, 7, 222, 103, 247, 244, 163, 156, 158, 5, 53, 126, 186, 215, 88, 48, 45, 32, 0, 0, 0, 0, 0, 0, 20, 25, 103, 87, 0, 0, 0, 0, 218, 189, 84, 137, 247, 169, 197, 113, 213, 120, 125, 148, 92, 197, 47, 212, 250, 153, 114, 53, 199, 209, 183, 97, 28, 242, 206, 120, 191, 202, 34, 9};




    tests[1].jobID = "testcase 2"; //We don't expect to find this. offset is purposely set too high.
    tests[1].offset = 805306368;
    //expected output
    //tests[1].header = {0, 0, 0, 0, 0, 0, 6, 72, 174, 217, 105, 206, 174, 59, 150, 117, 251, 55, 209, 192, 241, 37, 35, 184, 2, 194, 253, 173, 207, 249, 114, 1, 7, 235, 26, 63, 0, 0, 0, 0, 41, 7, 115, 87, 0, 0, 0, 0, 56, 56, 181, 217, 76, 24, 251, 231, 137, 4, 166, 20, 40, 53, 77, 36, 148, 23, 138, 146, 2, 199, 168, 122, 71, 162, 44, 150, 144, 2, 198, 67};
    //input                                                                                                                                                         //->target
    tests[1].header = {0, 0, 0, 0, 0, 0, 6, 72, 174, 217, 105, 206, 174, 59, 150, 117, 251, 55, 209, 192, 241, 37, 35, 184, 2, 194, 253, 173, 207, 249, 114, 1, 62, 26, 0, 0, 0, 0, 0, 0, 41, 7, 115, 87, 0, 0, 0, 0, 56, 56, 181, 217, 76, 24, 251, 231, 137, 4, 166, 20, 40, 53, 77, 36, 148, 23, 138, 146, 2, 199, 168, 122, 71, 162, 44, 150, 144, 2, 198, 67};
    miner->setTarget(1.0);
    miner->addJob(tests[0]);
    miner->addJob(tests[1]);
  
  }
  ~MiningTest() {
    delete miner;
  }
  bool isRunning() {
    return miner->isMining();
  }
  void reportSolution(SiaJob sj) {
    Bigmath bigmath;
    std::vector<uint8_t> expected1  = {0, 0, 0, 0, 0, 0, 26, 158, 25, 209, 169, 53, 113, 22, 90, 11, 72, 7, 222, 103, 247, 244, 163, 156, 158, 5, 53, 126, 186, 215, 88, 48, 88, 47, 107, 95, 0, 0, 0, 0, 20, 25, 103, 87, 0, 0, 0, 0, 218, 189, 84, 137, 247, 169, 197, 113, 213, 120, 125, 148, 92, 197, 47, 212, 250, 153, 114, 53, 199, 209, 183, 97, 28, 242, 206, 120, 191, 202, 34, 9};
    //nonce1 88, 47, 107, 95,
    //nonce2 7, 235, 26, 63,
    std::vector<uint8_t> expected2  = {0, 0, 0, 0, 0, 0, 6, 72, 174, 217, 105, 206, 174, 59, 150, 117, 251, 55, 209, 192, 241, 37, 35, 184, 2, 194, 253, 173, 207, 249, 114, 1, 7, 235, 26, 63, 0, 0, 0, 0, 41, 7, 115, 87, 0, 0, 0, 0, 56, 56, 181, 217, 76, 24, 251, 231, 137, 4, 166, 20, 40, 53, 77, 36, 148, 23, 138, 146, 2, 199, 168, 122, 71, 162, 44, 150, 144, 2, 198, 67};

    cout << "solution header :" << bigmath.toHexString(sj.header) << endl;
    if (sj.jobID == "testcase 1") {
      cout << "expected header: " << bigmath.toHexString(expected1);
      for (int i = 0; i < expected1.size(); i++)
        if (expected1[i] != sj.header[i]) {
          cout << "mismatch at" << i << endl;
          break;
        }
    }

    if (sj.jobID == "testcase 2") {
      cout << "expected header: " << bigmath.toHexString(expected2) << endl;
      for (int i = 0; i < expected2.size(); i++)
        if (expected1[i] != sj.header[i]) {
          cout << "mismatch at" << i << endl;
          break;
        }
    }
  }

};



#endif
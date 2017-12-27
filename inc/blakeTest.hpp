#ifndef __NDB_BLAKETEST
#define __NDB_BLAKETEST

#include <iostream>
#include "blake2b.hpp"
#include "bigmath.hpp"


/**
 * @brief      Class to test the Blake2b-256 functionalities.
 */
class BlakeTest {
 private:

 public:
  BlakeTest() {
    //Testcase:
    //b2b("This is just a test!") = d3e68989dde811e6ff8809e759180ae202db46d196588d1ac5201e961bdbce62
    std::string str = "This is just a test!";
    std::string expected = "d3e68989dde811e6ff8809e759180ae202db46d196588d1ac5201e961bdbce62";
    Bigmath bigmath;
    std::vector<uint8_t> expectedDigest = bigmath.hexStringToBytes(expected);
    std::vector<uint8_t> out;
    //Hash val
    Blake2b b2b;

    uint8_t input[20], input2[20];
    for (int i = 0; i < 20; i++) {
      input[i] = str[i];
    }


    uint8_t hash[32];

    //8 bit generation without regarding endianness
    b2b.sia_gen_hash(input, 20, hash);

    //copy output to uint vector:
    out.clear();
    for (int i = 0; i < 32; i++) {
      out.push_back(hash[i]);
    }
    cout << "exp:" << bigmath.toHexString(expectedDigest) << endl;
    cout << "out:" << bigmath.toHexString(out) << endl;

    cout << "block hashes:" << endl;

    //Block hashes:
    uint8_t expected1out[]  = {0, 0, 0, 0, 0, 0, 26, 158, 25, 209, 169, 53, 113, 22, 90, 11, 72, 7, 222, 103, 247, 244, 163, 156, 158, 5, 53, 126, 186, 215, 88, 48, 88, 47, 107, 95, 0, 0, 0, 0, 20, 25, 103, 87, 0, 0, 0, 0, 218, 189, 84, 137, 247, 169, 197, 113, 213, 120, 125, 148, 92, 197, 47, 212, 250, 153, 114, 53, 199, 209, 183, 97, 28, 242, 206, 120, 191, 202, 34, 9};
    uint8_t expected2out[]  = {0, 0, 0, 0, 0, 0, 6, 72, 174, 217, 105, 206, 174, 59, 150, 117, 251, 55, 209, 192, 241, 37, 35, 184, 2, 194, 253, 173, 207, 249, 114, 1, 7, 235, 26, 63, 0, 0, 0, 0, 41, 7, 115, 87, 0, 0, 0, 0, 56, 56, 181, 217, 76, 24, 251, 231, 137, 4, 166, 20, 40, 53, 77, 36, 148, 23, 138, 146, 2, 199, 168, 122, 71, 162, 44, 150, 144, 2, 198, 67};
    uint8_t expected1in[]  = {0, 0, 0, 0, 0, 0, 26, 158, 25, 209, 169, 53, 113, 22, 90, 11, 72, 7, 222, 103, 247, 244, 163, 156, 158, 5, 53, 126, 186, 215, 88, 48, 2, 0, 0, 0, 20, 25, 103, 87, 0, 0, 0, 0, 218, 189, 84, 137, 247, 169, 197, 113, 213, 120, 125, 148, 92, 197, 47, 212, 250, 153, 114, 53, 199, 209, 183, 97, 28, 242, 206, 120, 191, 202, 34, 9  };
    uint8_t expected2in[]  = {0, 0, 0, 0, 0, 0, 6, 72, 174, 217, 105, 206, 174, 59, 150, 117, 251, 55, 209, 192, 241, 37, 35, 184, 2, 194, 253, 173, 207, 249, 114, 1, 62, 26, 0, 0, 0, 0, 0, 0, 41, 7, 115, 87, 0, 0, 0, 0, 56, 56, 181, 217, 76, 24, 251, 231, 137, 4, 166, 20, 40, 53, 77, 36, 148, 23, 138, 146, 2, 199, 168, 122, 71, 162, 44, 150, 144, 2, 198, 67};
    std::string hash1 = "00000000000006418b86014ff54b457f52665b428d5af57e80b0b7ec84c706e5";
    std::string hash2 = "00000000000001ccac64b49a9ebc69c6046a93f4d32d8f8f6967c8f487ed8cec";




    b2b.sia_gen_hash(expected1out, 80, hash);
    cout << "exp:" << hash1 << endl << endl;
    cout << "out:" << bigmath.toHexString(hash, 32) << endl;

    b2b.sia_gen_hash(expected2out, 80, hash);
    cout << "exp:" << hash2 << endl;
    cout << "out:" << bigmath.toHexString(hash, 32) << endl;

  }


};



#endif
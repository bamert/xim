#ifndef __NDB_BIGMATH
#define __NDB_BIGMATH


struct Target {
  std::vector<uint8_t> value;

  Target() {
    value.resize(32, 0);
  }
  void fromNbits(std::vector<uint8_t> v) {
    value.resize(32, 0);
    //get 8msb
    uint8_t diffLen =  v[0];
    cout << "diffLen:" << int(diffLen) << endl;
    //last digit is 31, thus with diffLen=3, we'd have to insert
    //the value at 29,30,31, i.e. start inserting at 32-diffLen
    //get prefix
    value[32 - diffLen] = v[1];
    value[32 - diffLen + 1] = v[2];
    value[32 - diffLen + 2] = v[3];
  }
  //can only be used for network difficulty, since this is supplied as nbits
  void fromNbits(uint32_t nbits) {
    value.resize(32, 0);
    //get 8msb
    uint8_t diffLen =  (nbits >> 24) & 0xFF;
    //last digit is 31, thus with diffLen=3, we'd have to insert
    //the value at 29,30,31, i.e. start inserting at 32-diffLen
    //get prefix
    value[32 - diffLen] = (nbits >> 16) & 0xFF;
    value[32 - diffLen + 1] = (nbits >> 8) & 0xFF;
    value[32 - diffLen + 2] = (nbits ) & 0xFF;

  }
  void fromDifficulty(double difficulty) {
    value.resize(32, 0);
    //https://siamining.com/stratum
    //Stratum diff 1:
    //val1 =0x00000000ffff0000000000000000000000000000000000000000000000000000
    //target with difficulty of ~0.9...
    //tar =0x00000000ffffffffffffffffffffffffffffffffffffffffffffffffffffffff
    //thus we have difficulty = val1/tar, or
    //tar = val1/difficulty.
    //This can also be rewritten as
    // (val1>>25 / difficulty) << 25, which is much simpler to compute with
    // the available types in cpp. (the 25 is the number of bytes that follow
    // the 1-difficulty val1).
    // val1>>25 simply equals 0xffff00, thus the calculation is
    double f = double(0xffff00) / difficulty;
    //we get this as an int to shift back 26 bytes. This incurs some accuracy loss which may yield invalid shares.
    uint32_t tar = uint32_t(f);

    //shift 25 positions to the left again:
    value[28 - 25] = (tar >> 24) & 0xFF;
    value[29 - 25] = (tar >> 16) & 0xFF;
    value[30 - 25] = (tar >> 8) & 0xFF;
    value[31 - 25] = (tar ) & 0xFF;

    //Note that we could  also use a shift of 26 bytes  (and use hex 0xffff),
    //however this would lose one digit of accuracy of the difficulty and thus would
    //often yield difficulty 1 instead of something like 0.9
    //For more accuracte results, a proper bigint implementation should be used.
  }
};
class Bigmath {
 public:
  Bigmath() {

  }
  static uint8_t hexDigitToByte(uint8_t c) {
    if (c <= '9' && c >= '0')
      return c - '0';
    else if (c >= 'A' && c <= 'F')
      return c - 'A';
    else if (c >= 'a' && c <= 'f')
      return c - 'a';
  }

  static std::vector<uint8_t> hexStringToBytes(std::string str) {
    std::vector<uint8_t> out;
    //check if the string starts with '0x'
    int i = 0;
    if (str[0] == '0' && str[1] == 'x') {
      i += 2;
    }
    //odd length hex string: first byte consist of a nibble only (4bit, 1 character)
    if ((str.length() - i) % 2 !=
        0) {
      out.push_back(hexDigitToByte(str[i]));
      i++;
    }
    for (i; i < str.length() ;  i += 2) {
      out.push_back(hexDigitToByte(str[i]) * 16 + hexDigitToByte(str[i + 1]));
    }
    return out;

  }
  std::string toHexString(uint8_t* buffer, int length) {
    static const char* const lut = "0123456789ABCDEF";

    std::string output;
    output.reserve(2 * length);
    for (size_t i = 0; i < length; ++i) {
      const unsigned char c = buffer[i];
      output.push_back(lut[c >> 4]);
      output.push_back(lut[c & 15]);
    }
    return output;
  }
  std::string toHexString(std::vector<uint8_t> buffer) {
    static const char* const lut = "0123456789ABCDEF";

    std::string output;
    int length = buffer.size();
    output.reserve(2 * length);
    for (size_t i = 0; i < length; ++i) {
      const unsigned char c = buffer[i];
      output.push_back(lut[c >> 4]);
      output.push_back(lut[c & 15]);
    }
    return output;
  }
  std::vector<uint8_t> bufferToVector(uint8_t* buffer, int length) {
    std::vector<uint8_t> out;
    for (int i = 0; i < length; i++) {
      out.push_back(buffer[i]);
    }
    return out;
  }
  uint8_t* vectorToBuffer(std::vector<uint8_t> vec) {
    uint8_t* out = new uint8_t[vec.size()];
    for (int i = 0; i < vec.size(); i++) {
      out[i] = vec[i];
    }
    return out;
  }


 private:

};
#endif __NDB_BIGMATH
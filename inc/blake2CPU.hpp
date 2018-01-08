#ifndef __NDB_BLAKE2BCPU
#define __NDB_BLAKE2BCPU

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <chrono>
#include "bigmath.hpp"

namespace ndb {

// Cyclic right rotation. (non overflowing)
#ifndef ROTR64
#define ROTR64(x, y)  (((x) >> (y)) ^ ((x) << (64 - (y))))
#endif

// Little-endian byte access.
#define B2B_GET64(p)                            \
    (((uint64_t) ((uint8_t *) (p))[0]) ^        \
    (((uint64_t) ((uint8_t *) (p))[1]) << 8) ^  \
    (((uint64_t) ((uint8_t *) (p))[2]) << 16) ^ \
    (((uint64_t) ((uint8_t *) (p))[3]) << 24) ^ \
    (((uint64_t) ((uint8_t *) (p))[4]) << 32) ^ \
    (((uint64_t) ((uint8_t *) (p))[5]) << 40) ^ \
    (((uint64_t) ((uint8_t *) (p))[6]) << 48) ^ \
    (((uint64_t) ((uint8_t *) (p))[7]) << 56))
//Big endian byte access
#define B2B_BIGGET64(p)                            \
    (((uint64_t) ((uint8_t *) (p))[7]) ^        \
    (((uint64_t) ((uint8_t *) (p))[6]) << 8) ^  \
    (((uint64_t) ((uint8_t *) (p))[5]) << 16) ^ \
    (((uint64_t) ((uint8_t *) (p))[4]) << 24) ^ \
    (((uint64_t) ((uint8_t *) (p))[3]) << 32) ^ \
    (((uint64_t) ((uint8_t *) (p))[2]) << 40) ^ \
    (((uint64_t) ((uint8_t *) (p))[1]) << 48) ^ \
    (((uint64_t) ((uint8_t *) (p))[0]) << 56))

// G Mixing function. a,b,c,d are const. x and y are from input data (offsets can be pre computed)
#define B2B_G(a, b, c, d, x, y) {   \
    v[a] = v[a] + v[b] + x;         \
    v[d] = ROTR64(v[d] ^ v[a], 32); \
    v[c] = v[c] + v[d];             \
    v[b] = ROTR64(v[b] ^ v[c], 24); \
    v[a] = v[a] + v[b] + y;         \
    v[d] = ROTR64(v[d] ^ v[a], 16); \
    v[c] = v[c] + v[d];             \
    v[b] = ROTR64(v[b] ^ v[c], 63); }

//One of twelve rounds
#define ROUND(i){ \
        B2B_G( 0, 4,  8, 12, m[sigma[i][ 0]], m[sigma[i][ 1]]);\
        B2B_G( 1, 5,  9, 13, m[sigma[i][ 2]], m[sigma[i][ 3]]);\
        B2B_G( 2, 6, 10, 14, m[sigma[i][ 4]], m[sigma[i][ 5]]);\
        B2B_G( 3, 7, 11, 15, m[sigma[i][ 6]], m[sigma[i][ 7]]);\
        B2B_G( 0, 5, 10, 15, m[sigma[i][ 8]], m[sigma[i][ 9]]);\
        B2B_G( 1, 6, 11, 12, m[sigma[i][10]], m[sigma[i][11]]);\
        B2B_G( 2, 7,  8, 13, m[sigma[i][12]], m[sigma[i][13]]);\
        B2B_G( 3, 4,  9, 14, m[sigma[i][14]], m[sigma[i][15]]);}


#define ROUNDS(m,nonce){ \
        B2B_G( 0, 4,  8, 12, m[0], m[1]);\
        B2B_G( 1, 5,  9, 13, m[2], m[3]);\
        B2B_G( 2, 6, 10, 14, nonce, m[5]);\
        B2B_G( 3, 7, 11, 15, m[6], m[7]);\
        B2B_G( 0, 5, 10, 15, m[8], m[9]);\
        B2B_G( 1, 6, 11, 12, m[10], m[11]);\
        B2B_G( 2, 7,  8, 13, m[12], m[13]);\
        B2B_G( 3, 4,  9, 14, m[14], m[15]);\
         B2B_G( 0, 4,  8, 12, m[14], m[10]);\
        B2B_G( 1, 5,  9, 13, nonce, m[8]);\
        B2B_G( 2, 6, 10, 14, m[9], m[15]);\
        B2B_G( 3, 7, 11, 15, m[13], m[6]);\
        B2B_G( 0, 5, 10, 15, m[1], m[12]);\
        B2B_G( 1, 6, 11, 12, m[0], m[2]);\
        B2B_G( 2, 7,  8, 13, m[11], m[7]);\
        B2B_G( 3, 4,  9, 14, m[5], m[3]);\
         B2B_G( 0, 4,  8, 12, m[11], m[8]);\
        B2B_G( 1, 5,  9, 13, m[12], m[0]);\
        B2B_G( 2, 6, 10, 14, m[5], m[2]);\
        B2B_G( 3, 7, 11, 15, m[15], m[13]);\
        B2B_G( 0, 5, 10, 15, m[10], m[14]);\
        B2B_G( 1, 6, 11, 12, m[3], m[6]);\
        B2B_G( 2, 7,  8, 13, m[7], m[1]);\
        B2B_G( 3, 4,  9, 14, m[9], nonce);\
         B2B_G( 0, 4,  8, 12, m[7], m[9]);\
        B2B_G( 1, 5,  9, 13, m[3], m[1]);\
        B2B_G( 2, 6, 10, 14, m[13], m[12]);\
        B2B_G( 3, 7, 11, 15, m[11], m[14]);\
        B2B_G( 0, 5, 10, 15, m[2], m[6]);\
        B2B_G( 1, 6, 11, 12, m[5], m[10]);\
        B2B_G( 2, 7,  8, 13, nonce, m[0]);\
        B2B_G( 3, 4,  9, 14, m[15], m[8]);\
         B2B_G( 0, 4,  8, 12, m[9], m[0]);\
        B2B_G( 1, 5,  9, 13, m[5], m[7]);\
        B2B_G( 2, 6, 10, 14, m[2], nonce);\
        B2B_G( 3, 7, 11, 15, m[10], m[15]);\
        B2B_G( 0, 5, 10, 15, m[14], m[1]);\
        B2B_G( 1, 6, 11, 12, m[11], m[12]);\
        B2B_G( 2, 7,  8, 13, m[6], m[8]);\
        B2B_G( 3, 4,  9, 14, m[3], m[13]);\
         B2B_G( 0, 4,  8, 12, m[2], m[12]);\
        B2B_G( 1, 5,  9, 13, m[6], m[10]);\
        B2B_G( 2, 6, 10, 14, m[0], m[11]);\
        B2B_G( 3, 7, 11, 15, m[8], m[3]);\
        B2B_G( 0, 5, 10, 15, nonce, m[13]);\
        B2B_G( 1, 6, 11, 12, m[7], m[5]);\
        B2B_G( 2, 7,  8, 13, m[15], m[14]);\
        B2B_G( 3, 4,  9, 14, m[1], m[9]);\
         B2B_G( 0, 4,  8, 12, m[12], m[5]);\
        B2B_G( 1, 5,  9, 13, m[1], m[15]);\
        B2B_G( 2, 6, 10, 14, m[14], m[13]);\
        B2B_G( 3, 7, 11, 15, nonce, m[10]);\
        B2B_G( 0, 5, 10, 15, m[0], m[7]);\
        B2B_G( 1, 6, 11, 12, m[6], m[3]);\
        B2B_G( 2, 7,  8, 13, m[9], m[2]);\
        B2B_G( 3, 4,  9, 14, m[8], m[11]);\
         B2B_G( 0, 4,  8, 12, m[13], m[11]);\
        B2B_G( 1, 5,  9, 13, m[7], m[14]);\
        B2B_G( 2, 6, 10, 14, m[12], m[1]);\
        B2B_G( 3, 7, 11, 15, m[3], m[9]);\
        B2B_G( 0, 5, 10, 15, m[5], m[0]);\
        B2B_G( 1, 6, 11, 12, m[15], nonce);\
        B2B_G( 2, 7,  8, 13, m[8], m[6]);\
        B2B_G( 3, 4,  9, 14, m[2], m[10]);\
         B2B_G( 0, 4,  8, 12, m[6], m[15]);\
        B2B_G( 1, 5,  9, 13, m[14], m[9]);\
        B2B_G( 2, 6, 10, 14, m[11], m[3]);\
        B2B_G( 3, 7, 11, 15, m[0], m[8]);\
        B2B_G( 0, 5, 10, 15, m[12], m[2]);\
        B2B_G( 1, 6, 11, 12, m[13], m[7]);\
        B2B_G( 2, 7,  8, 13, m[1], nonce);\
        B2B_G( 3, 4,  9, 14, m[10], m[5]);\
         B2B_G( 0, 4,  8, 12, m[10], m[2]);\
        B2B_G( 1, 5,  9, 13, m[8], nonce);\
        B2B_G( 2, 6, 10, 14, m[7], m[6]);\
        B2B_G( 3, 7, 11, 15, m[1], m[5]);\
        B2B_G( 0, 5, 10, 15, m[15], m[11]);\
        B2B_G( 1, 6, 11, 12, m[9], m[14]);\
        B2B_G( 2, 7,  8, 13, m[3], m[12]);\
        B2B_G( 3, 4,  9, 14, m[13], m[0]);\
        B2B_G( 0, 4,  8, 12, m[0], m[1]);\
        B2B_G( 1, 5,  9, 13, m[2], m[3]);\
        B2B_G( 2, 6, 10, 14, nonce, m[5]);\
        B2B_G( 3, 7, 11, 15, m[6], m[7]);\
        B2B_G( 0, 5, 10, 15, m[8], m[9]);\
        B2B_G( 1, 6, 11, 12, m[10], m[11]);\
        B2B_G( 2, 7,  8, 13, m[12], m[13]);\
        B2B_G( 3, 4,  9, 14, m[14], m[15]);\
         B2B_G( 0, 4,  8, 12, m[14], m[10]);\
        B2B_G( 1, 5,  9, 13, nonce, m[8]);\
        B2B_G( 2, 6, 10, 14, m[9], m[15]);\
        B2B_G( 3, 7, 11, 15, m[13], m[6]);\
        B2B_G( 0, 5, 10, 15, m[1], m[12]);\
        B2B_G( 1, 6, 11, 12, m[0], m[2]);\
        B2B_G( 2, 7,  8, 13, m[11], m[7]);\
        B2B_G( 3, 4,  9, 14, m[5], m[3]);}








// state context
typedef struct {
  uint8_t b[128];                     // input buffer
  uint64_t h[8];                      // chained state
  uint64_t t[2];                      // total number of bytes
  size_t c;                           // pointer for b[]
  size_t outlen;                      // digest size
} blake2b_ctx;


uint64_t swapLong(void *X) {
  uint64_t x = (uint64_t) X;
  x = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;
  x = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;
  x = (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;
  return x;
}




class Blake2bCPU {
 public:

  Blake2bCPU(int nThreads) : nThreads(nThreads) {
  }
  const uint8_t sigma[12][16] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
    { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
    { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
    { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
    { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
    { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
    { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
    { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
    { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
    { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
    { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 }
  };

  const uint64_t blake2b_iv[8] = {
    0x6A09E667F3BCC908, 0xBB67AE8584CAA73B,
    0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1,
    0x510E527FADE682D1, 0x9B05688C2B3E6C1F,
    0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179
  };

  //contains some fake const data for the header
  const uint64_t mhdr[16] = {
    0x6A09E667F3BCC908, 0xBB67AE8584CAA73B,   0x6A09E667F3BCC908, 0xBB67AE8584CAA73B,
    0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1,   0x6A09E667F3BCC908, 0xBB67AE8584CAA73B,
    0x510E527FADE682D1, 0x9B05688C2B3E6C1F,
    0, 0, 0, 0, 0, 0
  };

  /* Scans a nonce range.
   *
   * @param[in]  header    The header (80 byte)
   * @param[in]  start     The start offset (little endian!)
   * @param[in]  end       The end offset (little endian!)
   * @param      nonceOut  The nonce out. This is set if we found a valid nonce
   *
   * @return     true if nonce found, false if not.
   */
  bool sia_hash_range( const unsigned char* header, uint32_t startNonce, uint32_t endNonce,
                       std::vector<uint8_t>& target, uint32_t* nonceOut) {

    Bigmath bigmath;

    blake2b_ctx ctx;
    uint8_t hash[32];
    bool found = false;

    unsigned char headerLocal[80];
    memcpy(headerLocal, header, 80); //copy header




    //int i;
    uint64_t v[16], m[16];

    // get little-endian words. (80 bytes header = 10 x 64 byte words)
    for (int i = 0; i < 10; i++)
      m[i] = B2B_GET64(&headerLocal[8 * i]);
    // pad remaining 48 bytes (6 x 64 bit words)
    for (int i = 10; i < 16; i++)
      m[i] = 0;

    auto now = std::chrono::high_resolution_clock::now();
    auto prev  = std::chrono::high_resolution_clock::now();
    uint32_t intensity = endNonce - startNonce;

    for (uint32_t k = startNonce; k < endNonce; k++) {
      //cout << "nonce2:" << k << endl;
      //output some stats for now:
      if (k % 10000 == 0) {
        now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>
            (now - prev);
        double secs = time_span.count();
        double hashRate = 10000 / secs;
        cout << 100.*k / float(intensity) << "percent," <<  hashRate / 1000000 << "MH/s   \r";
        prev = now;
      }
      //Update. Here we can update only the 32bits in question instead of the full work header.
      headerLocal[32] = (k >> 24) & 0xFF;
      headerLocal[33] = (k >> 16) & 0xFF;
      headerLocal[34] = (k >> 8) & 0xFF;
      headerLocal[35] = (k) & 0xFF;
      //fill new nonce:
      //update little endian representation of 64bit nonce
      m[4] = B2B_GET64(&headerLocal[32]);

      //This is the same on every iteration, because we just filled h with the same content as on every round.
      //
      //
      //
      //Instead of computing this over and over, we can just precompute
      //the values that need to be loaded into h and v before
      //we apply the 12 rounds

      v[0] = blake2b_iv[0] ^ 0x01010020;
      v[1] = blake2b_iv[1];
      v[2] = blake2b_iv[2];
      v[3] = blake2b_iv[3];
      v[4] = blake2b_iv[4];
      v[5] = blake2b_iv[5];
      v[6] = blake2b_iv[6];
      v[7] = blake2b_iv[7];
      v[8] = blake2b_iv[0]; //not the last round thingy from above
      v[9] = blake2b_iv[1];
      v[10] = blake2b_iv[2];
      v[11] = blake2b_iv[3];
      v[12] = blake2b_iv[4] ^ 80; //^ ctx.t[0];, but t[0] = 80, always (header length)
      v[13] = blake2b_iv[5]; //^ ctx.t[1];, but t[1]  = 0, always
      v[14] = ~blake2b_iv[6]; //last block, thus invert  (the header we hash is only 80 bytes long, which is less than the size of one block(128b))
      v[15] = blake2b_iv[7];

      //previous loop content

      //---start update
      //------------start compress:
      //Here we cannot reduce anything. Although we only need 64bits
      //of the resulting hash, we still have to compute all 12 rounds
      //with all 8 updates each.
      //However, note that out of the 10 x 64 bit words in the header, only
      //one changes (or even just half of it). -> compile code at run-time
      //as method

      ROUNDS(m, m[4]);

      /*     ROUND(0);
           ROUND(1);
           ROUND(2);
           ROUND(3);
           ROUND(4);
           ROUND(5);
           ROUND(6);
           ROUND(7);
           ROUND(8);
           ROUND(9);
           ROUND(10);
           ROUND(11);*/



      //we only care about the first 64 bits of the hash
      ctx.h[0] = blake2b_iv[0] ^ 0x01010020 ^ v[0] ^ v[8];
    
      //convert those first 64bits to big endian.
      hash[0] = ctx.h[0] & 0xFF;
      hash[1] = (ctx.h[0] >> 8) & 0xFF;
      hash[2] = (ctx.h[0] >> 16) & 0xFF;
      hash[3] = (ctx.h[0] >> 24) & 0xFF;
      hash[4] = (ctx.h[0] >> 32) & 0xFF;
      hash[5] = (ctx.h[0] >> 40) & 0xFF;
      hash[6] = (ctx.h[0] >> 48) & 0xFF;
      hash[7] = (ctx.h[0] >> 56) & 0xFF;



      for (int i = 0; i < 32; i++) {
        if (hash[i] < target[i]) {
          found = true;
          break;
        }
        if (hash[i] > target[i]) {
          found = false;
          break;
        }
      }

      if (found == true) {

        *nonceOut = k; //store successful nonce
        return true;
      }


      /*if (ctx.h[0] < target64) {
        *nonceOut = k; //store successful nonce
        return true;
      }*/
    }
    return false;
  }


// Initialization Vector.

 private:
  int nThreads = 1;





};
};
#endif
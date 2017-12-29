#ifndef __NDB_BLAKE2BCPU
#define __NDB_BLAKE2BCPU

#include <stdlib.h>
#include <stdint.h>
#include <string.h>


namespace ndb {

// Cyclic right rotation.
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

// G Mixing function.
#define B2B_G(a, b, c, d, x, y) {   \
    v[a] = v[a] + v[b] + x;         \
    v[d] = ROTR64(v[d] ^ v[a], 32); \
    v[c] = v[c] + v[d];             \
    v[b] = ROTR64(v[b] ^ v[c], 24); \
    v[a] = v[a] + v[b] + y;         \
    v[d] = ROTR64(v[d] ^ v[a], 16); \
    v[c] = v[c] + v[d];             \
    v[b] = ROTR64(v[b] ^ v[c], 63); }



// state context
typedef struct {
  uint8_t b[128];                     // input buffer
  uint64_t h[8];                      // chained state
  uint64_t t[2];                      // total number of bytes
  size_t c;                           // pointer for b[]
  size_t outlen;                      // digest size
} blake2b_ctx;


/*
 * Encode a length len/4 vector of (uint32_t) into a length len vector of
 * (unsigned char) in big-endian form.  Assumes len is a multiple of 4.
 */
static inline void
be32enc_vect(uint32_t *dst, const uint32_t *src, uint32_t len) {
  uint32_t i;

  for (i = 0; i < len; i++)
    dst[i] = htobe32(src[i]);
}


class Blake2bCPU {
 public:

  Blake2bCPU() {
  }

  /* Scans a nonce range.
   *
   * @param[in]  header    The header (80 byte)
   * @param[in]  start     The start offset (little endian!)
   * @param[in]  end       The end offset (little endian!)
   * @param      nonceOut  The nonce out. This is set if we found a valid nonce
   *
   * @return     true if nonce found, false if not.
   */
  bool sia_hash_range( unsigned char* header, uint32_t startNonce, uint32_t endNonce, std::vector<uint8_t>& target, uint32_t* nonceOut) {



    blake2b_ctx ctx;
    uint8_t hash[32];
    bool found = false;

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


    int i;
    uint64_t v[16], m[16];

    ctx.t[0] = 0;                      // input count low word
    ctx.t[1] = 0;                      // input count high word
    ctx.c = 0;                         // pointer within buffer
    ctx.outlen = 32;

    //pre-pad input block
    for (int i = 0; i < 128; i++)
      ctx.b[i] = 0;
    //fill header
    for (int i = 0; i < 80; i++) {
      ctx.b[i] = header[i];
    }
    for (int i = 0; i < 16; i++)            // get little-endian words. same on every iteration except for the one field with the nonce.
      m[i] = B2B_GET64(&ctx.b[8 * i]);


    //set input data length
    ctx.c = 80;
    //---end update

    //---start final
    ctx.t[0] = 80;                // mark last block offset


    for (uint32_t k = startNonce; k < endNonce; k++) {
      cout << "nonce2:" << k << endl;
      //Update. Here we can update only the 32bits in question instead of the full work header.
      header[32] = (k >> 24) & 0xFF;
      header[33] = (k >> 16) & 0xFF;
      header[34] = (k >> 8) & 0xFF;
      header[35] = (k) & 0xFF;
//fill new nonce:
      //update little endian representation of 64bit nonce
      m[4] = B2B_GET64(&header[32]);


      //init:
      for (int i = 0; i < 8; i++)             // state, "param block"
        ctx.h[i] = blake2b_iv[i];
      ctx.h[0] ^= 0x01010000 ^ (0 << 8) ^ 32;

      //This is the same on every iteration, because we just filled h with the same content as on every round.
      //
      //
      //
      //Instead of computing this over and over, we can just precompute 
      //the values that need to be loaded into h and v before
      //we apply the 12 rounds
      //
      //
      for (int i = 0; i < 8; i++) {           // init work variables
        v[i] = ctx.h[i];
        v[i + 8] = blake2b_iv[i];
      }

      v[12] ^= ctx.t[0];                 // low 64 bits of offset
      v[13] ^= ctx.t[1];                 // high 64 bits
      v[14] = ~v[14];   //this is always the last block. (the header we hash is only 80 bytes long, which is less than the size of one block(128b))


      //---start update

      //blake2b_compress(&ctx, 1);           // final block flag = 1
      //------------start compress:

      for (int i = 0; i < 12; i++) {          // twelve rounds
        B2B_G( 0, 4,  8, 12, m[sigma[i][ 0]], m[sigma[i][ 1]]);
        B2B_G( 1, 5,  9, 13, m[sigma[i][ 2]], m[sigma[i][ 3]]);
        B2B_G( 2, 6, 10, 14, m[sigma[i][ 4]], m[sigma[i][ 5]]);
        B2B_G( 3, 7, 11, 15, m[sigma[i][ 6]], m[sigma[i][ 7]]);
        B2B_G( 0, 5, 10, 15, m[sigma[i][ 8]], m[sigma[i][ 9]]);
        B2B_G( 1, 6, 11, 12, m[sigma[i][10]], m[sigma[i][11]]);
        B2B_G( 2, 7,  8, 13, m[sigma[i][12]], m[sigma[i][13]]);
        B2B_G( 3, 4,  9, 14, m[sigma[i][14]], m[sigma[i][15]]);
      }

      //Trace it back from here: which part of v do we need to get the part of h that we need (only first bits!)
      for (int  i = 0; i < 8; ++i )
        ctx.h[i] ^= v[i] ^ v[i + 8];

      //----------------End compress


      // little endian convert and store
      for (int i = 0; i < ctx.outlen; i++) {
        hash[i] =
          (ctx.h[i >> 3] >> (8 * (i & 7))) & 0xFF;
      }
      //-------End final


      //blake2b_update(&ctx, header, 80);
      //blake2b_final(&ctx, &hash);
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
    }
    return false;
  }


// Initialization Vector.

 private:






};
};
#endif
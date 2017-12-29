#ifndef __NDB_BLAKE2BCPU
#define __NDB_BLAKE2BCPU

/*
    This is the full blake implementation that is used to compute the headers.
    It's okay that this is slow.
 */

/*-
 * Copyright 2009 Colin Percival, 2014 savale
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file was originally written by Colin Percival as part of the Tarsnap
 * online backup system.
 */

//#include "config.h"
//#include "miner.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#ifdef __APPLE__

#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#endif

namespace ndb {


#define bswap_16(value)  \
  ((((value) & 0xff) << 8) | ((value) >> 8))


#define bswap_32(value) \
  (((uint32_t)bswap_16((uint16_t)((value) & 0xffff)) << 16) | \
  (uint32_t)bswap_16((uint16_t)((value) >> 16)))
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


uint32_t swab32(uint32_t v) {
  return bswap_32(v);
}
void swab256(void *dest_p, const void *src_p) {
  uint32_t *dest = (uint32_t *)dest_p;
  const uint32_t *src = (uint32_t *)src_p;

  dest[0] = swab32(src[7]);
  dest[1] = swab32(src[6]);
  dest[2] = swab32(src[5]);
  dest[3] = swab32(src[4]);
  dest[4] = swab32(src[3]);
  dest[5] = swab32(src[2]);
  dest[6] = swab32(src[1]);
  dest[7] = swab32(src[0]);
}
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
    int i;
    uint64_t v[16], m[16];


    /*
        for (i = 0; i < 8; i++)             // state, "param block"
          ctx->h[i] = blake2b_iv[i];
        ctx->h[0] ^= 0x01010000 ^ (keylen << 8) ^ outlen;

        ctx->t[0] = 0;                      // input count low word
        ctx->t[1] = 0;                      // input count high word
        ctx->c = 0;                         // pointer within buffer
        ctx->outlen = outlen;

        for (i = keylen; i < 128; i++)      // zero input block
          ctx->b[i] = 0;
        if (keylen > 0) {
          blake2b_update(ctx, key, keylen);
          ctx->c = 128;                   // at the end
        }*/

    // blake2b_init(&ctx, 32, NULL, 0);

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
      for (int i = 32; i < 36; i++) {
        ctx.b[i] = header[i];
      }
      //init:
      for (int i = 0; i < 8; i++)             // state, "param block"
        ctx.h[i] = blake2b_iv[i];
      ctx.h[0] ^= 0x01010000 ^ (0 << 8) ^ 32;

      //This is the same on every iteration
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
      for (int i = 0; i < 16; i++)            // get little-endian words
        m[i] = B2B_GET64(&ctx.b[8 * i]);

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
  const uint64_t blake2b_iv[8] = {
    0x6A09E667F3BCC908, 0xBB67AE8584CAA73B,
    0x3C6EF372FE94F82B, 0xA54FF53A5F1D36F1,
    0x510E527FADE682D1, 0x9B05688C2B3E6C1F,
    0x1F83D9ABFB41BD6B, 0x5BE0CD19137E2179
  };

 private:






};
};
#endif
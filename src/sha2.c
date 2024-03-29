/*
 * FIPS 180-2 SHA-224/256/384/512 implementation
 * Last update: 02/02/2007
 * Issue date:  04/30/2005
 *
 * Copyright (C) 2005, 2007 Olivier Gay <olivier.gay@a3.epfl.ch>
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if 0
#define TEST_VECTORS
#define UNROLL_LOOPS /* Enable loops unrolling */
#endif
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif	// HAVE_CONFIG_H
#include <string.h>
#include <stdint.h>
#ifdef TEST_VECTORS
#include <stdio.h>
#include <stdlib.h>
#ifdef WINDOZE
#include <intrin.h>
#endif  // WINDOZE
#endif  // TEST_VECTORS

#include "sha2.h"

static unsigned int uExsha512Flag = 0;
#ifdef SSESUPPORT
#ifdef __cplusplus
extern "C" {
#endif
extern void sha512_sse4(const void* M, void* D, uint64_t L);
extern void sha512_avx(const void* M, void* D, uint64_t L);
extern void sha512_rorx(const void* M, void* D, uint64_t L);
#ifdef __cplusplus
}
#endif
#endif	// SSESUPPORT
extern int doHAsh ( char * salt, char * password , int rounds);


#define SHFR(x, n)    (x >> n)
#define ROTR(x, n)   ((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define ROTL(x, n)   ((x << n) | (x >> ((sizeof(x) << 3) - n)))
#define CH(x, y, z)  ((x & y) ^ (~x & z))
#define MAJ(x, y, z) ((x & y) ^ (x & z) ^ (y & z))

#define SHA256_F1(x) (ROTR(x,  2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define SHA256_F2(x) (ROTR(x,  6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SHA256_F3(x) (ROTR(x,  7) ^ ROTR(x, 18) ^ SHFR(x,  3))
#define SHA256_F4(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHFR(x, 10))

#define SHA512_F1(x) (ROTR(x, 28) ^ ROTR(x, 34) ^ ROTR(x, 39))
#define SHA512_F2(x) (ROTR(x, 14) ^ ROTR(x, 18) ^ ROTR(x, 41))
#define SHA512_F3(x) (ROTR(x,  1) ^ ROTR(x,  8) ^ SHFR(x,  7))
#define SHA512_F4(x) (ROTR(x, 19) ^ ROTR(x, 61) ^ SHFR(x,  6))

#define UNPACK32(x, str)                      \
{                                             \
    *((str) + 3) = (uint8) ((x)      );       \
    *((str) + 2) = (uint8) ((x) >>  8);       \
    *((str) + 1) = (uint8) ((x) >> 16);       \
    *((str) + 0) = (uint8) ((x) >> 24);       \
}

#define PACK32(str, x)                        \
{                                             \
    *(x) =   ((uint32) *((str) + 3)      )    \
           | ((uint32) *((str) + 2) <<  8)    \
           | ((uint32) *((str) + 1) << 16)    \
           | ((uint32) *((str) + 0) << 24);   \
}

#define UNPACK64(x, str)                      \
{                                             \
    *((str) + 7) = (uint8) ((x)      );       \
    *((str) + 6) = (uint8) ((x) >>  8);       \
    *((str) + 5) = (uint8) ((x) >> 16);       \
    *((str) + 4) = (uint8) ((x) >> 24);       \
    *((str) + 3) = (uint8) ((x) >> 32);       \
    *((str) + 2) = (uint8) ((x) >> 40);       \
    *((str) + 1) = (uint8) ((x) >> 48);       \
    *((str) + 0) = (uint8) ((x) >> 56);       \
}

#define PACK64(str, x)                        \
{                                             \
    *(x) =   ((uint64) *((str) + 7)      )    \
           | ((uint64) *((str) + 6) <<  8)    \
           | ((uint64) *((str) + 5) << 16)    \
           | ((uint64) *((str) + 4) << 24)    \
           | ((uint64) *((str) + 3) << 32)    \
           | ((uint64) *((str) + 2) << 40)    \
           | ((uint64) *((str) + 1) << 48)    \
           | ((uint64) *((str) + 0) << 56);   \
}

/* Macros used for loops unrolling */

#define SHA256_SCR(i)                         \
{                                             \
    w[i] =  SHA256_F4(w[i -  2]) + w[i -  7]  \
          + SHA256_F3(w[i - 15]) + w[i - 16]; \
}

#define SHA512_SCR(i)                         \
{                                             \
    w[i] =  SHA512_F4(w[i -  2]) + w[i -  7]  \
          + SHA512_F3(w[i - 15]) + w[i - 16]; \
}

#define SHA256_EXP(a, b, c, d, e, f, g, h, j)               \
{                                                           \
    t1 = wv[h] + SHA256_F2(wv[e]) + CH(wv[e], wv[f], wv[g]) \
         + sha256_k[j] + w[j];                              \
    t2 = SHA256_F1(wv[a]) + MAJ(wv[a], wv[b], wv[c]);       \
    wv[d] += t1;                                            \
    wv[h] = t1 + t2;                                        \
}

#define SHA512_EXP(a, b, c, d, e, f, g ,h, j)               \
{                                                           \
    t1 = wv[h] + SHA512_F2(wv[e]) + CH(wv[e], wv[f], wv[g]) \
         + sha512_k[j] + w[j];                              \
    t2 = SHA512_F1(wv[a]) + MAJ(wv[a], wv[b], wv[c]);       \
    wv[d] += t1;                                            \
    wv[h] = t1 + t2;                                        \
}
#ifndef SHA512ONLY
uint32 sha224_h0[8] =
            {0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939,
             0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4};

uint32 sha256_h0[8] =
            {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
             0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
#endif // SHA512ONLY
#if (WINDOZE && _MSC_VER < 1500)
#ifndef SHA512ONLY
uint64 sha384_h0[8] =
            {0xcbbb9d5dc1059ed8i64, 0x629a292a367cd507i64,
             0x9159015a3070dd17i64, 0x152fecd8f70e5939i64,
             0x67332667ffc00b31i64, 0x8eb44a8768581511i64,
             0xdb0c2e0d64f98fa7i64, 0x47b5481dbefa4fa4i64};
#endif // SHA512ONLY
uint64 sha512_h0[8] =
            {0x6a09e667f3bcc908i64, 0xbb67ae8584caa73bi64,
             0x3c6ef372fe94f82bi64, 0xa54ff53a5f1d36f1i64,
             0x510e527fade682d1i64, 0x9b05688c2b3e6c1fi64,
             0x1f83d9abfb41bd6bi64, 0x5be0cd19137e2179i64};
#else  // WINDOZE
#ifndef SHA512ONLY
uint64 sha384_h0[8] =
            {0xcbbb9d5dc1059ed8, 0x629a292a367cd507,
             0x9159015a3070dd17, 0x152fecd8f70e5939,
             0x67332667ffc00b31, 0x8eb44a8768581511,
             0xdb0c2e0d64f98fa7, 0x47b5481dbefa4fa4};
#endif // SHA512ONLY
uint64 sha512_h0[8] =
            {0x6a09e667f3bcc908, 0xbb67ae8584caa73b,
             0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
             0x510e527fade682d1, 0x9b05688c2b3e6c1f,
             0x1f83d9abfb41bd6b, 0x5be0cd19137e2179};
#endif  // WINDOZE
#ifndef SHA512ONLY
uint32 sha256_k[64] =
            {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
             0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
             0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
             0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
             0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
             0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
             0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
             0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
             0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
             0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
             0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
             0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
             0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
             0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
             0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
             0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
#endif // SHA512ONLY
#if (WINDOZE && _MSC_VER < 1500)
uint64 sha512_k[80] =
            {0x428a2f98d728ae22i64, 0x7137449123ef65cdi64,
             0xb5c0fbcfec4d3b2fi64, 0xe9b5dba58189dbbci64,
             0x3956c25bf348b538i64, 0x59f111f1b605d019i64,
             0x923f82a4af194f9bi64, 0xab1c5ed5da6d8118i64,
             0xd807aa98a3030242i64, 0x12835b0145706fbei64,
             0x243185be4ee4b28ci64, 0x550c7dc3d5ffb4e2i64,
             0x72be5d74f27b896fi64, 0x80deb1fe3b1696b1i64,
             0x9bdc06a725c71235i64, 0xc19bf174cf692694i64,
             0xe49b69c19ef14ad2i64, 0xefbe4786384f25e3i64,
             0x0fc19dc68b8cd5b5i64, 0x240ca1cc77ac9c65i64,
             0x2de92c6f592b0275i64, 0x4a7484aa6ea6e483i64,
             0x5cb0a9dcbd41fbd4i64, 0x76f988da831153b5i64,
             0x983e5152ee66dfabi64, 0xa831c66d2db43210i64,
             0xb00327c898fb213fi64, 0xbf597fc7beef0ee4i64,
             0xc6e00bf33da88fc2i64, 0xd5a79147930aa725i64,
             0x06ca6351e003826fi64, 0x142929670a0e6e70i64,
             0x27b70a8546d22ffci64, 0x2e1b21385c26c926i64,
             0x4d2c6dfc5ac42aedi64, 0x53380d139d95b3dfi64,
             0x650a73548baf63dei64, 0x766a0abb3c77b2a8i64,
             0x81c2c92e47edaee6i64, 0x92722c851482353bi64,
             0xa2bfe8a14cf10364i64, 0xa81a664bbc423001i64,
             0xc24b8b70d0f89791i64, 0xc76c51a30654be30i64,
             0xd192e819d6ef5218i64, 0xd69906245565a910i64,
             0xf40e35855771202ai64, 0x106aa07032bbd1b8i64,
             0x19a4c116b8d2d0c8i64, 0x1e376c085141ab53i64,
             0x2748774cdf8eeb99i64, 0x34b0bcb5e19b48a8i64,
             0x391c0cb3c5c95a63i64, 0x4ed8aa4ae3418acbi64,
             0x5b9cca4f7763e373i64, 0x682e6ff3d6b2b8a3i64,
             0x748f82ee5defb2fci64, 0x78a5636f43172f60i64,
             0x84c87814a1f0ab72i64, 0x8cc702081a6439eci64,
             0x90befffa23631e28i64, 0xa4506cebde82bde9i64,
             0xbef9a3f7b2c67915i64, 0xc67178f2e372532bi64,
             0xca273eceea26619ci64, 0xd186b8c721c0c207i64,
             0xeada7dd6cde0eb1ei64, 0xf57d4f7fee6ed178i64,
             0x06f067aa72176fbai64, 0x0a637dc5a2c898a6i64,
             0x113f9804bef90daei64, 0x1b710b35131c471bi64,
             0x28db77f523047d84i64, 0x32caab7b40c72493i64,
             0x3c9ebe0a15c9bebci64, 0x431d67c49c100d4ci64,
             0x4cc5d4becb3e42b6i64, 0x597f299cfc657e2ai64,
             0x5fcb6fab3ad6faeci64, 0x6c44198c4a475817i64};
#else	// WINDOZE
uint64 sha512_k[80] =
            {0x428a2f98d728ae22, 0x7137449123ef65cd,
             0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
             0x3956c25bf348b538, 0x59f111f1b605d019,
             0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
             0xd807aa98a3030242, 0x12835b0145706fbe,
             0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
             0x72be5d74f27b896f, 0x80deb1fe3b1696b1,
             0x9bdc06a725c71235, 0xc19bf174cf692694,
             0xe49b69c19ef14ad2, 0xefbe4786384f25e3,
             0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
             0x2de92c6f592b0275, 0x4a7484aa6ea6e483,
             0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
             0x983e5152ee66dfab, 0xa831c66d2db43210,
             0xb00327c898fb213f, 0xbf597fc7beef0ee4,
             0xc6e00bf33da88fc2, 0xd5a79147930aa725,
             0x06ca6351e003826f, 0x142929670a0e6e70,
             0x27b70a8546d22ffc, 0x2e1b21385c26c926,
             0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
             0x650a73548baf63de, 0x766a0abb3c77b2a8,
             0x81c2c92e47edaee6, 0x92722c851482353b,
             0xa2bfe8a14cf10364, 0xa81a664bbc423001,
             0xc24b8b70d0f89791, 0xc76c51a30654be30,
             0xd192e819d6ef5218, 0xd69906245565a910,
             0xf40e35855771202a, 0x106aa07032bbd1b8,
             0x19a4c116b8d2d0c8, 0x1e376c085141ab53,
             0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
             0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb,
             0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
             0x748f82ee5defb2fc, 0x78a5636f43172f60,
             0x84c87814a1f0ab72, 0x8cc702081a6439ec,
             0x90befffa23631e28, 0xa4506cebde82bde9,
             0xbef9a3f7b2c67915, 0xc67178f2e372532b,
             0xca273eceea26619c, 0xd186b8c721c0c207,
             0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
             0x06f067aa72176fba, 0x0a637dc5a2c898a6,
             0x113f9804bef90dae, 0x1b710b35131c471b,
             0x28db77f523047d84, 0x32caab7b40c72493,
             0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
             0x4cc5d4becb3e42b6, 0x597f299cfc657e2a,
             0x5fcb6fab3ad6faec, 0x6c44198c4a475817};
#endif	// WINDOZE

/* SHA-256 functions */
#ifndef SHA512ONLY
void sha256_transf(sha256_ctx *ctx, const unsigned char *message,
                   unsigned int block_nb)
{
    uint32 w[64];
    uint32 wv[8];
    uint32 t1, t2;
    const unsigned char *sub_block;
    int i;

#ifndef UNROLL_LOOPS
    int j;
#endif

    for (i = 0; i < (int) block_nb; i++) {
        sub_block = message + (i << 6);

#ifndef UNROLL_LOOPS
        for (j = 0; j < 16; j++) {
            PACK32(&sub_block[j << 2], &w[j]);
        }

        for (j = 16; j < 64; j++) {
            SHA256_SCR(j);
        }

        for (j = 0; j < 8; j++) {
            wv[j] = ctx->h[j];
        }

        for (j = 0; j < 64; j++) {
            t1 = wv[7] + SHA256_F2(wv[4]) + CH(wv[4], wv[5], wv[6])
                + sha256_k[j] + w[j];
            t2 = SHA256_F1(wv[0]) + MAJ(wv[0], wv[1], wv[2]);
            wv[7] = wv[6];
            wv[6] = wv[5];
            wv[5] = wv[4];
            wv[4] = wv[3] + t1;
            wv[3] = wv[2];
            wv[2] = wv[1];
            wv[1] = wv[0];
            wv[0] = t1 + t2;
        }

        for (j = 0; j < 8; j++) {
            ctx->h[j] += wv[j];
        }
#else
        PACK32(&sub_block[ 0], &w[ 0]); PACK32(&sub_block[ 4], &w[ 1]);
        PACK32(&sub_block[ 8], &w[ 2]); PACK32(&sub_block[12], &w[ 3]);
        PACK32(&sub_block[16], &w[ 4]); PACK32(&sub_block[20], &w[ 5]);
        PACK32(&sub_block[24], &w[ 6]); PACK32(&sub_block[28], &w[ 7]);
        PACK32(&sub_block[32], &w[ 8]); PACK32(&sub_block[36], &w[ 9]);
        PACK32(&sub_block[40], &w[10]); PACK32(&sub_block[44], &w[11]);
        PACK32(&sub_block[48], &w[12]); PACK32(&sub_block[52], &w[13]);
        PACK32(&sub_block[56], &w[14]); PACK32(&sub_block[60], &w[15]);

        SHA256_SCR(16); SHA256_SCR(17); SHA256_SCR(18); SHA256_SCR(19);
        SHA256_SCR(20); SHA256_SCR(21); SHA256_SCR(22); SHA256_SCR(23);
        SHA256_SCR(24); SHA256_SCR(25); SHA256_SCR(26); SHA256_SCR(27);
        SHA256_SCR(28); SHA256_SCR(29); SHA256_SCR(30); SHA256_SCR(31);
        SHA256_SCR(32); SHA256_SCR(33); SHA256_SCR(34); SHA256_SCR(35);
        SHA256_SCR(36); SHA256_SCR(37); SHA256_SCR(38); SHA256_SCR(39);
        SHA256_SCR(40); SHA256_SCR(41); SHA256_SCR(42); SHA256_SCR(43);
        SHA256_SCR(44); SHA256_SCR(45); SHA256_SCR(46); SHA256_SCR(47);
        SHA256_SCR(48); SHA256_SCR(49); SHA256_SCR(50); SHA256_SCR(51);
        SHA256_SCR(52); SHA256_SCR(53); SHA256_SCR(54); SHA256_SCR(55);
        SHA256_SCR(56); SHA256_SCR(57); SHA256_SCR(58); SHA256_SCR(59);
        SHA256_SCR(60); SHA256_SCR(61); SHA256_SCR(62); SHA256_SCR(63);

        wv[0] = ctx->h[0]; wv[1] = ctx->h[1];
        wv[2] = ctx->h[2]; wv[3] = ctx->h[3];
        wv[4] = ctx->h[4]; wv[5] = ctx->h[5];
        wv[6] = ctx->h[6]; wv[7] = ctx->h[7];

        SHA256_EXP(0,1,2,3,4,5,6,7, 0); SHA256_EXP(7,0,1,2,3,4,5,6, 1);
        SHA256_EXP(6,7,0,1,2,3,4,5, 2); SHA256_EXP(5,6,7,0,1,2,3,4, 3);
        SHA256_EXP(4,5,6,7,0,1,2,3, 4); SHA256_EXP(3,4,5,6,7,0,1,2, 5);
        SHA256_EXP(2,3,4,5,6,7,0,1, 6); SHA256_EXP(1,2,3,4,5,6,7,0, 7);
        SHA256_EXP(0,1,2,3,4,5,6,7, 8); SHA256_EXP(7,0,1,2,3,4,5,6, 9);
        SHA256_EXP(6,7,0,1,2,3,4,5,10); SHA256_EXP(5,6,7,0,1,2,3,4,11);
        SHA256_EXP(4,5,6,7,0,1,2,3,12); SHA256_EXP(3,4,5,6,7,0,1,2,13);
        SHA256_EXP(2,3,4,5,6,7,0,1,14); SHA256_EXP(1,2,3,4,5,6,7,0,15);
        SHA256_EXP(0,1,2,3,4,5,6,7,16); SHA256_EXP(7,0,1,2,3,4,5,6,17);
        SHA256_EXP(6,7,0,1,2,3,4,5,18); SHA256_EXP(5,6,7,0,1,2,3,4,19);
        SHA256_EXP(4,5,6,7,0,1,2,3,20); SHA256_EXP(3,4,5,6,7,0,1,2,21);
        SHA256_EXP(2,3,4,5,6,7,0,1,22); SHA256_EXP(1,2,3,4,5,6,7,0,23);
        SHA256_EXP(0,1,2,3,4,5,6,7,24); SHA256_EXP(7,0,1,2,3,4,5,6,25);
        SHA256_EXP(6,7,0,1,2,3,4,5,26); SHA256_EXP(5,6,7,0,1,2,3,4,27);
        SHA256_EXP(4,5,6,7,0,1,2,3,28); SHA256_EXP(3,4,5,6,7,0,1,2,29);
        SHA256_EXP(2,3,4,5,6,7,0,1,30); SHA256_EXP(1,2,3,4,5,6,7,0,31);
        SHA256_EXP(0,1,2,3,4,5,6,7,32); SHA256_EXP(7,0,1,2,3,4,5,6,33);
        SHA256_EXP(6,7,0,1,2,3,4,5,34); SHA256_EXP(5,6,7,0,1,2,3,4,35);
        SHA256_EXP(4,5,6,7,0,1,2,3,36); SHA256_EXP(3,4,5,6,7,0,1,2,37);
        SHA256_EXP(2,3,4,5,6,7,0,1,38); SHA256_EXP(1,2,3,4,5,6,7,0,39);
        SHA256_EXP(0,1,2,3,4,5,6,7,40); SHA256_EXP(7,0,1,2,3,4,5,6,41);
        SHA256_EXP(6,7,0,1,2,3,4,5,42); SHA256_EXP(5,6,7,0,1,2,3,4,43);
        SHA256_EXP(4,5,6,7,0,1,2,3,44); SHA256_EXP(3,4,5,6,7,0,1,2,45);
        SHA256_EXP(2,3,4,5,6,7,0,1,46); SHA256_EXP(1,2,3,4,5,6,7,0,47);
        SHA256_EXP(0,1,2,3,4,5,6,7,48); SHA256_EXP(7,0,1,2,3,4,5,6,49);
        SHA256_EXP(6,7,0,1,2,3,4,5,50); SHA256_EXP(5,6,7,0,1,2,3,4,51);
        SHA256_EXP(4,5,6,7,0,1,2,3,52); SHA256_EXP(3,4,5,6,7,0,1,2,53);
        SHA256_EXP(2,3,4,5,6,7,0,1,54); SHA256_EXP(1,2,3,4,5,6,7,0,55);
        SHA256_EXP(0,1,2,3,4,5,6,7,56); SHA256_EXP(7,0,1,2,3,4,5,6,57);
        SHA256_EXP(6,7,0,1,2,3,4,5,58); SHA256_EXP(5,6,7,0,1,2,3,4,59);
        SHA256_EXP(4,5,6,7,0,1,2,3,60); SHA256_EXP(3,4,5,6,7,0,1,2,61);
        SHA256_EXP(2,3,4,5,6,7,0,1,62); SHA256_EXP(1,2,3,4,5,6,7,0,63);

        ctx->h[0] += wv[0]; ctx->h[1] += wv[1];
        ctx->h[2] += wv[2]; ctx->h[3] += wv[3];
        ctx->h[4] += wv[4]; ctx->h[5] += wv[5];
        ctx->h[6] += wv[6]; ctx->h[7] += wv[7];
#endif /* !UNROLL_LOOPS */
    }
}

void sha256(const unsigned char *message, unsigned int len, unsigned char *digest)
{
    sha256_ctx ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, message, len);
    sha256_final(&ctx, digest);
}

void sha256_init(sha256_ctx *ctx)
{
#ifndef UNROLL_LOOPS
    int i;
    for (i = 0; i < 8; i++) {
        ctx->h[i] = sha256_h0[i];
    }
#else
    ctx->h[0] = sha256_h0[0]; ctx->h[1] = sha256_h0[1];
    ctx->h[2] = sha256_h0[2]; ctx->h[3] = sha256_h0[3];
    ctx->h[4] = sha256_h0[4]; ctx->h[5] = sha256_h0[5];
    ctx->h[6] = sha256_h0[6]; ctx->h[7] = sha256_h0[7];
#endif /* !UNROLL_LOOPS */

    ctx->len = 0;
    ctx->tot_len = 0;
}

void sha256_update(sha256_ctx *ctx, const unsigned char *message,
                   unsigned int len)
{
    unsigned int block_nb;
    unsigned int new_len, rem_len, tmp_len;
    const unsigned char *shifted_message;

    tmp_len = SHA256_BLOCK_SIZE - ctx->len;
    rem_len = len < tmp_len ? len : tmp_len;

    memcpy(&ctx->block[ctx->len], message, rem_len);

    if (ctx->len + len < SHA256_BLOCK_SIZE) {
        ctx->len += len;
        return;
    }

    new_len = len - rem_len;
    block_nb = new_len / SHA256_BLOCK_SIZE;

    shifted_message = message + rem_len;

    sha256_transf(ctx, ctx->block, 1);
    sha256_transf(ctx, shifted_message, block_nb);

    rem_len = new_len % SHA256_BLOCK_SIZE;

    memcpy(ctx->block, &shifted_message[block_nb << 6],
           rem_len);

    ctx->len = rem_len;
    ctx->tot_len += (block_nb + 1) << 6;
}

void sha256_final(sha256_ctx *ctx, unsigned char *digest)
{
    unsigned int block_nb;
    unsigned int pm_len;
    unsigned int len_b;

#ifndef UNROLL_LOOPS
    int i;
#endif

    block_nb = (1 + ((SHA256_BLOCK_SIZE - 9)
                     < (ctx->len % SHA256_BLOCK_SIZE)));

    len_b = (ctx->tot_len + ctx->len) << 3;
    pm_len = block_nb << 6;

    memset(ctx->block + ctx->len, 0, pm_len - ctx->len);
    ctx->block[ctx->len] = 0x80;
    UNPACK32(len_b, ctx->block + pm_len - 4);

    sha256_transf(ctx, ctx->block, block_nb);

#ifndef UNROLL_LOOPS
    for (i = 0 ; i < 8; i++) {
        UNPACK32(ctx->h[i], &digest[i << 2]);
    }
#else
   UNPACK32(ctx->h[0], &digest[ 0]);
   UNPACK32(ctx->h[1], &digest[ 4]);
   UNPACK32(ctx->h[2], &digest[ 8]);
   UNPACK32(ctx->h[3], &digest[12]);
   UNPACK32(ctx->h[4], &digest[16]);
   UNPACK32(ctx->h[5], &digest[20]);
   UNPACK32(ctx->h[6], &digest[24]);
   UNPACK32(ctx->h[7], &digest[28]);
#endif /* !UNROLL_LOOPS */
}
#endif // SHA512ONLY
/* SHA-512 functions */

void sha512_transf(sha512_ctx *ctx, const unsigned char *message,
                   unsigned int block_nb)
{
    uint64 w[80];
    uint64 wv[8];
    uint64 t1, t2;
    const unsigned char *sub_block;
    int i, j;

    for (i = 0; i < (int) block_nb; i++) {
        sub_block = message + (i << 7);

#ifndef UNROLL_LOOPS
        for (j = 0; j < 16; j++) {
            PACK64(&sub_block[j << 3], &w[j]);
        }

        for (j = 16; j < 80; j++) {
            SHA512_SCR(j);
        }

        for (j = 0; j < 8; j++) {
            wv[j] = ctx->h[j];
        }

        for (j = 0; j < 80; j++) {
            t1 = wv[7] + SHA512_F2(wv[4]) + CH(wv[4], wv[5], wv[6])
                + sha512_k[j] + w[j];
            t2 = SHA512_F1(wv[0]) + MAJ(wv[0], wv[1], wv[2]);
            wv[7] = wv[6];
            wv[6] = wv[5];
            wv[5] = wv[4];
            wv[4] = wv[3] + t1;
            wv[3] = wv[2];
            wv[2] = wv[1];
            wv[1] = wv[0];
            wv[0] = t1 + t2;
        }

        for (j = 0; j < 8; j++) {
            ctx->h[j] += wv[j];
        }
#else
        PACK64(&sub_block[  0], &w[ 0]); PACK64(&sub_block[  8], &w[ 1]);
        PACK64(&sub_block[ 16], &w[ 2]); PACK64(&sub_block[ 24], &w[ 3]);
        PACK64(&sub_block[ 32], &w[ 4]); PACK64(&sub_block[ 40], &w[ 5]);
        PACK64(&sub_block[ 48], &w[ 6]); PACK64(&sub_block[ 56], &w[ 7]);
        PACK64(&sub_block[ 64], &w[ 8]); PACK64(&sub_block[ 72], &w[ 9]);
        PACK64(&sub_block[ 80], &w[10]); PACK64(&sub_block[ 88], &w[11]);
        PACK64(&sub_block[ 96], &w[12]); PACK64(&sub_block[104], &w[13]);
        PACK64(&sub_block[112], &w[14]); PACK64(&sub_block[120], &w[15]);

        SHA512_SCR(16); SHA512_SCR(17); SHA512_SCR(18); SHA512_SCR(19);
        SHA512_SCR(20); SHA512_SCR(21); SHA512_SCR(22); SHA512_SCR(23);
        SHA512_SCR(24); SHA512_SCR(25); SHA512_SCR(26); SHA512_SCR(27);
        SHA512_SCR(28); SHA512_SCR(29); SHA512_SCR(30); SHA512_SCR(31);
        SHA512_SCR(32); SHA512_SCR(33); SHA512_SCR(34); SHA512_SCR(35);
        SHA512_SCR(36); SHA512_SCR(37); SHA512_SCR(38); SHA512_SCR(39);
        SHA512_SCR(40); SHA512_SCR(41); SHA512_SCR(42); SHA512_SCR(43);
        SHA512_SCR(44); SHA512_SCR(45); SHA512_SCR(46); SHA512_SCR(47);
        SHA512_SCR(48); SHA512_SCR(49); SHA512_SCR(50); SHA512_SCR(51);
        SHA512_SCR(52); SHA512_SCR(53); SHA512_SCR(54); SHA512_SCR(55);
        SHA512_SCR(56); SHA512_SCR(57); SHA512_SCR(58); SHA512_SCR(59);
        SHA512_SCR(60); SHA512_SCR(61); SHA512_SCR(62); SHA512_SCR(63);
        SHA512_SCR(64); SHA512_SCR(65); SHA512_SCR(66); SHA512_SCR(67);
        SHA512_SCR(68); SHA512_SCR(69); SHA512_SCR(70); SHA512_SCR(71);
        SHA512_SCR(72); SHA512_SCR(73); SHA512_SCR(74); SHA512_SCR(75);
        SHA512_SCR(76); SHA512_SCR(77); SHA512_SCR(78); SHA512_SCR(79);

        wv[0] = ctx->h[0]; wv[1] = ctx->h[1];
        wv[2] = ctx->h[2]; wv[3] = ctx->h[3];
        wv[4] = ctx->h[4]; wv[5] = ctx->h[5];
        wv[6] = ctx->h[6]; wv[7] = ctx->h[7];

        j = 0;

        do {
            SHA512_EXP(0,1,2,3,4,5,6,7,j); j++;
            SHA512_EXP(7,0,1,2,3,4,5,6,j); j++;
            SHA512_EXP(6,7,0,1,2,3,4,5,j); j++;
            SHA512_EXP(5,6,7,0,1,2,3,4,j); j++;
            SHA512_EXP(4,5,6,7,0,1,2,3,j); j++;
            SHA512_EXP(3,4,5,6,7,0,1,2,j); j++;
            SHA512_EXP(2,3,4,5,6,7,0,1,j); j++;
            SHA512_EXP(1,2,3,4,5,6,7,0,j); j++;
        } while (j < 80);

        ctx->h[0] += wv[0]; ctx->h[1] += wv[1];
        ctx->h[2] += wv[2]; ctx->h[3] += wv[3];
        ctx->h[4] += wv[4]; ctx->h[5] += wv[5];
        ctx->h[6] += wv[6]; ctx->h[7] += wv[7];
#endif /* !UNROLL_LOOPS */
    }
}
#ifdef SSESUPPORT
unsigned int SetSha512Kernel ( int iSha512Kernel )
{
	// check for valid kernel setting. If valid, set kernel.
	if ( iSha512Kernel >= 0 && iSha512Kernel < 4 ) {
		uExsha512Flag = (unsigned int)iSha512Kernel;
	}
	// return current kernel setting
	return(uExsha512Flag);
}
#endif	// SSESUPPORT
void sha512(const unsigned char *message, unsigned int len,
            unsigned char *digest)
{
    sha512_ctx ctx;

    sha512_init(&ctx);
    sha512_update(&ctx, message, len);
    sha512_final(&ctx, digest);
}

void sha512_init(sha512_ctx *ctx)
{
#ifndef UNROLL_LOOPS
    int i;
    for (i = 0; i < 8; i++) {
        ctx->h[i] = sha512_h0[i];
    }
#else
    ctx->h[0] = sha512_h0[0]; ctx->h[1] = sha512_h0[1];
    ctx->h[2] = sha512_h0[2]; ctx->h[3] = sha512_h0[3];
    ctx->h[4] = sha512_h0[4]; ctx->h[5] = sha512_h0[5];
    ctx->h[6] = sha512_h0[6]; ctx->h[7] = sha512_h0[7];
#endif /* !UNROLL_LOOPS */

    ctx->len = 0;
    ctx->tot_len = 0;
}

void sha512_update(sha512_ctx *ctx, const unsigned char *message,
                   unsigned int len)
{
    unsigned int block_nb;
    unsigned int new_len, rem_len, tmp_len;
    const unsigned char *shifted_message;

    tmp_len = SHA512_BLOCK_SIZE - ctx->len;
    rem_len = len < tmp_len ? len : tmp_len;

    memcpy(&ctx->block[ctx->len], message, rem_len);

    if (ctx->len + len < SHA512_BLOCK_SIZE) {
        ctx->len += len;
        return;
    }

    new_len = len - rem_len;
    block_nb = new_len / SHA512_BLOCK_SIZE;


    shifted_message = message + rem_len;

    switch (uExsha512Flag) {
    	case 0:
    		sha512_transf(ctx, ctx->block, 1);
    		sha512_transf(ctx, shifted_message, block_nb);
    		break;
#ifdef SSESUPPORT
    	case 1:
    	    sha512_sse4(ctx->block, ctx->h, (uint64_t)1);
    	    sha512_sse4(shifted_message, ctx->h, (uint64_t)block_nb);
    	    break;

    	case 2:
    	    sha512_avx(ctx->block, ctx->h, (uint64_t)1);
    	    sha512_avx(shifted_message, ctx->h, (uint64_t)block_nb);
    	    break;

    	case 3:
    	    sha512_rorx(ctx->block, ctx->h, (uint64_t)1);
    	    sha512_rorx(shifted_message, ctx->h, (uint64_t)block_nb);
    	    break;
#endif	// SSESUPPORT
    	default:
    		sha512_transf(ctx, ctx->block, 1);
    		sha512_transf(ctx, shifted_message, block_nb);

    }

    rem_len = new_len % SHA512_BLOCK_SIZE;

    memcpy(ctx->block, &shifted_message[block_nb << 7],
           rem_len);

    ctx->len = rem_len;
    ctx->tot_len += (block_nb + 1) << 7;
}

void sha512_final(sha512_ctx *ctx, unsigned char *digest)
{
    unsigned int block_nb;
    unsigned int pm_len;
    unsigned int len_b;

#ifndef UNROLL_LOOPS
    int i;
#endif

    block_nb = 1 + ((SHA512_BLOCK_SIZE - 17)
                     < (ctx->len % SHA512_BLOCK_SIZE));

    len_b = (ctx->tot_len + ctx->len) << 3;
    pm_len = block_nb << 7;

    memset(ctx->block + ctx->len, 0, pm_len - ctx->len);
    ctx->block[ctx->len] = 0x80;
    UNPACK32(len_b, ctx->block + pm_len - 4);

    switch (uExsha512Flag) {
    case 0:
    	sha512_transf(ctx, ctx->block, block_nb);
    	break;
#ifdef SSESUPPORT
    case 1:
        sha512_sse4(ctx->block, ctx->h, block_nb);
        break;

    case 2:
        sha512_avx(ctx->block, ctx->h, block_nb);
        break;

    case 3:
        sha512_rorx(ctx->block, ctx->h, block_nb);
        break;
#endif	// SSESUPPORT
    default:
    	sha512_transf(ctx, ctx->block, block_nb);

    }

#ifndef UNROLL_LOOPS
    for (i = 0 ; i < 8; i++) {
        UNPACK64(ctx->h[i], &digest[i << 3]);
    }
#else
    UNPACK64(ctx->h[0], &digest[ 0]);
    UNPACK64(ctx->h[1], &digest[ 8]);
    UNPACK64(ctx->h[2], &digest[16]);
    UNPACK64(ctx->h[3], &digest[24]);
    UNPACK64(ctx->h[4], &digest[32]);
    UNPACK64(ctx->h[5], &digest[40]);
    UNPACK64(ctx->h[6], &digest[48]);
    UNPACK64(ctx->h[7], &digest[56]);
#endif /* !UNROLL_LOOPS */
}

/* SHA-384 functions */
#ifndef SHA512ONLY
void sha384(const unsigned char *message, unsigned int len,
            unsigned char *digest)
{
    sha384_ctx ctx;

    sha384_init(&ctx);
    sha384_update(&ctx, message, len);
    sha384_final(&ctx, digest);
}

void sha384_init(sha384_ctx *ctx)
{
#ifndef UNROLL_LOOPS
    int i;
    for (i = 0; i < 8; i++) {
        ctx->h[i] = sha384_h0[i];
    }
#else
    ctx->h[0] = sha384_h0[0]; ctx->h[1] = sha384_h0[1];
    ctx->h[2] = sha384_h0[2]; ctx->h[3] = sha384_h0[3];
    ctx->h[4] = sha384_h0[4]; ctx->h[5] = sha384_h0[5];
    ctx->h[6] = sha384_h0[6]; ctx->h[7] = sha384_h0[7];
#endif /* !UNROLL_LOOPS */

    ctx->len = 0;
    ctx->tot_len = 0;
}

void sha384_update(sha384_ctx *ctx, const unsigned char *message,
                   unsigned int len)
{
    unsigned int block_nb;
    unsigned int new_len, rem_len, tmp_len;
    const unsigned char *shifted_message;

    tmp_len = SHA384_BLOCK_SIZE - ctx->len;
    rem_len = len < tmp_len ? len : tmp_len;

    memcpy(&ctx->block[ctx->len], message, rem_len);

    if (ctx->len + len < SHA384_BLOCK_SIZE) {
        ctx->len += len;
        return;
    }

    new_len = len - rem_len;
    block_nb = new_len / SHA384_BLOCK_SIZE;

    shifted_message = message + rem_len;

    sha512_transf(ctx, ctx->block, 1);
    sha512_transf(ctx, shifted_message, block_nb);

    rem_len = new_len % SHA384_BLOCK_SIZE;

    memcpy(ctx->block, &shifted_message[block_nb << 7],
           rem_len);

    ctx->len = rem_len;
    ctx->tot_len += (block_nb + 1) << 7;
}

void sha384_final(sha384_ctx *ctx, unsigned char *digest)
{
    unsigned int block_nb;
    unsigned int pm_len;
    unsigned int len_b;

#ifndef UNROLL_LOOPS
    int i;
#endif

    block_nb = (1 + ((SHA384_BLOCK_SIZE - 17)
                     < (ctx->len % SHA384_BLOCK_SIZE)));

    len_b = (ctx->tot_len + ctx->len) << 3;
    pm_len = block_nb << 7;

    memset(ctx->block + ctx->len, 0, pm_len - ctx->len);
    ctx->block[ctx->len] = 0x80;
    UNPACK32(len_b, ctx->block + pm_len - 4);

    sha512_transf(ctx, ctx->block, block_nb);

#ifndef UNROLL_LOOPS
    for (i = 0 ; i < 6; i++) {
        UNPACK64(ctx->h[i], &digest[i << 3]);
    }
#else
    UNPACK64(ctx->h[0], &digest[ 0]);
    UNPACK64(ctx->h[1], &digest[ 8]);
    UNPACK64(ctx->h[2], &digest[16]);
    UNPACK64(ctx->h[3], &digest[24]);
    UNPACK64(ctx->h[4], &digest[32]);
    UNPACK64(ctx->h[5], &digest[40]);
#endif /* !UNROLL_LOOPS */
}

/* SHA-224 functions */

void sha224(const unsigned char *message, unsigned int len,
            unsigned char *digest)
{
    sha224_ctx ctx;

    sha224_init(&ctx);
    sha224_update(&ctx, message, len);
    sha224_final(&ctx, digest);
}

void sha224_init(sha224_ctx *ctx)
{
#ifndef UNROLL_LOOPS
    int i;
    for (i = 0; i < 8; i++) {
        ctx->h[i] = sha224_h0[i];
    }
#else
    ctx->h[0] = sha224_h0[0]; ctx->h[1] = sha224_h0[1];
    ctx->h[2] = sha224_h0[2]; ctx->h[3] = sha224_h0[3];
    ctx->h[4] = sha224_h0[4]; ctx->h[5] = sha224_h0[5];
    ctx->h[6] = sha224_h0[6]; ctx->h[7] = sha224_h0[7];
#endif /* !UNROLL_LOOPS */

    ctx->len = 0;
    ctx->tot_len = 0;
}

void sha224_update(sha224_ctx *ctx, const unsigned char *message,
                   unsigned int len)
{
    unsigned int block_nb;
    unsigned int new_len, rem_len, tmp_len;
    const unsigned char *shifted_message;

    tmp_len = SHA224_BLOCK_SIZE - ctx->len;
    rem_len = len < tmp_len ? len : tmp_len;

    memcpy(&ctx->block[ctx->len], message, rem_len);

    if (ctx->len + len < SHA224_BLOCK_SIZE) {
        ctx->len += len;
        return;
    }

    new_len = len - rem_len;
    block_nb = new_len / SHA224_BLOCK_SIZE;

    shifted_message = message + rem_len;

    sha256_transf(ctx, ctx->block, 1);
    sha256_transf(ctx, shifted_message, block_nb);

    rem_len = new_len % SHA224_BLOCK_SIZE;

    memcpy(ctx->block, &shifted_message[block_nb << 6],
           rem_len);

    ctx->len = rem_len;
    ctx->tot_len += (block_nb + 1) << 6;
}

void sha224_final(sha224_ctx *ctx, unsigned char *digest)
{
    unsigned int block_nb;
    unsigned int pm_len;
    unsigned int len_b;

#ifndef UNROLL_LOOPS
    int i;
#endif

    block_nb = (1 + ((SHA224_BLOCK_SIZE - 9)
                     < (ctx->len % SHA224_BLOCK_SIZE)));

    len_b = (ctx->tot_len + ctx->len) << 3;
    pm_len = block_nb << 6;

    memset(ctx->block + ctx->len, 0, pm_len - ctx->len);
    ctx->block[ctx->len] = 0x80;
    UNPACK32(len_b, ctx->block + pm_len - 4);

    sha256_transf(ctx, ctx->block, block_nb);

#ifndef UNROLL_LOOPS
    for (i = 0 ; i < 7; i++) {
        UNPACK32(ctx->h[i], &digest[i << 2]);
    }
#else
   UNPACK32(ctx->h[0], &digest[ 0]);
   UNPACK32(ctx->h[1], &digest[ 4]);
   UNPACK32(ctx->h[2], &digest[ 8]);
   UNPACK32(ctx->h[3], &digest[12]);
   UNPACK32(ctx->h[4], &digest[16]);
   UNPACK32(ctx->h[5], &digest[20]);
   UNPACK32(ctx->h[6], &digest[24]);
#endif /* !UNROLL_LOOPS */
}
#endif // SHA512ONLY
#ifdef TEST_VECTORS

/* FIPS 180-2 Validation tests */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


void test(const char *vector, unsigned char *digest,
          unsigned int digest_size)
{
    char output[2 * SHA512_DIGEST_SIZE + 1];
    int i;

    output[2 * digest_size] = '\0';

    for (i = 0; i < (int) digest_size ; i++) {
       sprintf(output + 2 * i, "%02x", digest[i]);
    }

    fprintf(stderr,"H: %s\n", output);
    if (strlen(vector) != strlen(output)) fprintf(stderr,"vector lengths don't agree\nstrlen(vector)=%i, strlen(output)=%i\n",(int)strlen(vector),(int)strlen(output));
    if (strcmp(vector, output)) {
        fprintf(stderr, "Test failed.\n");
        fprintf(stderr, "vector=\"%s\"\n",vector);
        fprintf(stderr, "output=\"%s\"\n",output);
//        exit(EXIT_FAILURE);
    }
}

#ifdef SSESUPPORT
void sha512_sseupdate(sha512_ctx *ctx, const unsigned char *message,
                   unsigned int len)
{
    unsigned int block_nb;
    unsigned int new_len, rem_len, tmp_len;
    const unsigned char *shifted_message;

    tmp_len = SHA512_BLOCK_SIZE - ctx->len;
    rem_len = len < tmp_len ? len : tmp_len;

    memcpy(&ctx->block[ctx->len], message, rem_len);

    if (ctx->len + len < SHA512_BLOCK_SIZE) {
        ctx->len += len;
        return;
    }

    new_len = len - rem_len;
    block_nb = new_len / SHA512_BLOCK_SIZE;
//    fprintf(stderr,"number of blocks=%i\n",block_nb);
    shifted_message = message + rem_len;


//    sha512_transf(ctx, ctx->block, 1);
//    sha512_transf(ctx, shifted_message, block_nb);

// // looks like this    void sha512_sse4(const void* M, void* D, uint64_t L);
    sha512_sse4(ctx->block, ctx->h, (uint64_t)1);
    sha512_sse4(shifted_message, ctx->h, (uint64_t)block_nb);

    rem_len = new_len % SHA512_BLOCK_SIZE;

    memcpy(ctx->block, &shifted_message[block_nb << 7],
           rem_len);

    ctx->len = rem_len;
    ctx->tot_len += (block_nb + 1) << 7;
}

void sha512_ssefinal(sha512_ctx *ctx, unsigned char *digest)
{
    unsigned int block_nb;
    unsigned int pm_len;
    unsigned int len_b;

#ifndef UNROLL_LOOPS
    int i;
#endif

    block_nb = 1 + ((SHA512_BLOCK_SIZE - 17)
                     < (ctx->len % SHA512_BLOCK_SIZE));

    len_b = (ctx->tot_len + ctx->len) << 3;
    pm_len = block_nb << 7;

    memset(ctx->block + ctx->len, 0, pm_len - ctx->len);
    ctx->block[ctx->len] = 0x80;
    UNPACK32(len_b, ctx->block + pm_len - 4);

    sha512_sse4(ctx->block, ctx->h, block_nb);

#ifndef UNROLL_LOOPS
    for (i = 0 ; i < 8; i++) {
        UNPACK64(ctx->h[i], &digest[i << 3]);
    }
#else
    UNPACK64(ctx->h[0], &digest[ 0]);
    UNPACK64(ctx->h[1], &digest[ 8]);
    UNPACK64(ctx->h[2], &digest[16]);
    UNPACK64(ctx->h[3], &digest[24]);
    UNPACK64(ctx->h[4], &digest[32]);
    UNPACK64(ctx->h[5], &digest[40]);
    UNPACK64(ctx->h[6], &digest[48]);
    UNPACK64(ctx->h[7], &digest[56]);
#endif /* !UNROLL_LOOPS */
}


void sha512sse(const unsigned char *message, unsigned int len,
            unsigned char *digest)
{
    sha512_ctx ctx;

    sha512_init(&ctx);
    sha512_sseupdate(&ctx, message, len);
    sha512_ssefinal(&ctx, digest);
}



void sha512_avxupdate(sha512_ctx *ctx, const unsigned char *message,
                   unsigned int len)
{
    unsigned int block_nb;
    unsigned int new_len, rem_len, tmp_len;
    const unsigned char *shifted_message;

    tmp_len = SHA512_BLOCK_SIZE - ctx->len;
    rem_len = len < tmp_len ? len : tmp_len;

    memcpy(&ctx->block[ctx->len], message, rem_len);

    if (ctx->len + len < SHA512_BLOCK_SIZE) {
        ctx->len += len;
        return;
    }

    new_len = len - rem_len;
    block_nb = new_len / SHA512_BLOCK_SIZE;
//    fprintf(stderr,"number of blocks=%i\n",block_nb);
    shifted_message = message + rem_len;


//    sha512_transf(ctx, ctx->block, 1);
//    sha512_transf(ctx, shifted_message, block_nb);

// // looks like this    void sha512_sse4(const void* M, void* D, uint64_t L);
    sha512_avx(ctx->block, ctx->h, (uint64_t)1);
    sha512_avx(shifted_message, ctx->h, (uint64_t)block_nb);

    rem_len = new_len % SHA512_BLOCK_SIZE;

    memcpy(ctx->block, &shifted_message[block_nb << 7],
           rem_len);

    ctx->len = rem_len;
    ctx->tot_len += (block_nb + 1) << 7;
}

void sha512_avxfinal(sha512_ctx *ctx, unsigned char *digest)
{
    unsigned int block_nb;
    unsigned int pm_len;
    unsigned int len_b;

#ifndef UNROLL_LOOPS
    int i;
#endif

    block_nb = 1 + ((SHA512_BLOCK_SIZE - 17)
                     < (ctx->len % SHA512_BLOCK_SIZE));

    len_b = (ctx->tot_len + ctx->len) << 3;
    pm_len = block_nb << 7;

    memset(ctx->block + ctx->len, 0, pm_len - ctx->len);
    ctx->block[ctx->len] = 0x80;
    UNPACK32(len_b, ctx->block + pm_len - 4);

    sha512_avx(ctx->block, ctx->h, block_nb);

#ifndef UNROLL_LOOPS
    for (i = 0 ; i < 8; i++) {
        UNPACK64(ctx->h[i], &digest[i << 3]);
    }
#else
    UNPACK64(ctx->h[0], &digest[ 0]);
    UNPACK64(ctx->h[1], &digest[ 8]);
    UNPACK64(ctx->h[2], &digest[16]);
    UNPACK64(ctx->h[3], &digest[24]);
    UNPACK64(ctx->h[4], &digest[32]);
    UNPACK64(ctx->h[5], &digest[40]);
    UNPACK64(ctx->h[6], &digest[48]);
    UNPACK64(ctx->h[7], &digest[56]);
#endif /* !UNROLL_LOOPS */
}

void sha512avx(const unsigned char *message, unsigned int len,
            unsigned char *digest)
{
    sha512_ctx ctx;

    sha512_init(&ctx);
    sha512_avxupdate(&ctx, message, len);
    sha512_avxfinal(&ctx, digest);
}



void sha512_rorxupdate(sha512_ctx *ctx, const unsigned char *message,
                   unsigned int len)
{
    unsigned int block_nb;
    unsigned int new_len, rem_len, tmp_len;
    const unsigned char *shifted_message;

    tmp_len = SHA512_BLOCK_SIZE - ctx->len;
    rem_len = len < tmp_len ? len : tmp_len;

    memcpy(&ctx->block[ctx->len], message, rem_len);

    if (ctx->len + len < SHA512_BLOCK_SIZE) {
        ctx->len += len;
        return;
    }

    new_len = len - rem_len;
    block_nb = new_len / SHA512_BLOCK_SIZE;
//    fprintf(stderr,"number of blocks=%i\n",block_nb);
    shifted_message = message + rem_len;


//    sha512_transf(ctx, ctx->block, 1);
//    sha512_transf(ctx, shifted_message, block_nb);

// // looks like this    void sha512_sse4(const void* M, void* D, uint64_t L);
    sha512_rorx(ctx->block, ctx->h, (uint64_t)1);
    sha512_rorx(shifted_message, ctx->h, (uint64_t)block_nb);

    rem_len = new_len % SHA512_BLOCK_SIZE;

    memcpy(ctx->block, &shifted_message[block_nb << 7],
           rem_len);

    ctx->len = rem_len;
    ctx->tot_len += (block_nb + 1) << 7;
}

void sha512_rorxfinal(sha512_ctx *ctx, unsigned char *digest)
{
    unsigned int block_nb;
    unsigned int pm_len;
    unsigned int len_b;

#ifndef UNROLL_LOOPS
    int i;
#endif

    block_nb = 1 + ((SHA512_BLOCK_SIZE - 17)
                     < (ctx->len % SHA512_BLOCK_SIZE));

    len_b = (ctx->tot_len + ctx->len) << 3;
    pm_len = block_nb << 7;

    memset(ctx->block + ctx->len, 0, pm_len - ctx->len);
    ctx->block[ctx->len] = 0x80;
    UNPACK32(len_b, ctx->block + pm_len - 4);

    sha512_rorx(ctx->block, ctx->h, block_nb);

#ifndef UNROLL_LOOPS
    for (i = 0 ; i < 8; i++) {
        UNPACK64(ctx->h[i], &digest[i << 3]);
    }
#else
    UNPACK64(ctx->h[0], &digest[ 0]);
    UNPACK64(ctx->h[1], &digest[ 8]);
    UNPACK64(ctx->h[2], &digest[16]);
    UNPACK64(ctx->h[3], &digest[24]);
    UNPACK64(ctx->h[4], &digest[32]);
    UNPACK64(ctx->h[5], &digest[40]);
    UNPACK64(ctx->h[6], &digest[48]);
    UNPACK64(ctx->h[7], &digest[56]);
#endif /* !UNROLL_LOOPS */
}

void sha512rorx(const unsigned char *message, unsigned int len,
            unsigned char *digest)
{
    sha512_ctx ctx;

    sha512_init(&ctx);
    sha512_rorxupdate(&ctx, message, len);
    sha512_rorxfinal(&ctx, digest);
}
#endif	// SSESUPPORT



int main(void)
{
    static const char *vectors[4][5] =
    {   /* SHA-224 */
        {
        "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7",
        "75388b16512776cc5dba5da1fd890150b0c6455cb4f58b1952522525",
        "20794655980c91d8bbb4c1ea97618a4bf03f42581948b2ee4ee7ad67",
        "0",
        "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f",
        },
        /* SHA-256 */
        {
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
        "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
        "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0",
        "0",
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        },
        /* SHA-384 */
        {
        "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed"
        "8086072ba1e7cc2358baeca134c825a7",
        "09330c33f71147e83d192fc782cd1b4753111b173b3b05d22fa08086e3b0f712"
        "fcc7c71a557e2db966c3e9fa91746039",
        "9d0e1809716474cb086e834e310a4a1ced149e9c00f248527972cec5704c2a5b"
        "07b8b3dc38ecc4ebae97ddd87f3d8985",
        "0",
        "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da"
        "274edebfe76f65fbd51ad2f14898b95b",
        },
        /* SHA-512 */
        {
        "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
        "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f",
        "8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018"
        "501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909",
        "e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973eb"
        "de0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e4eadb217ad8cc09b",
        "523df363aa22c25478a478d25e6945f842509df8b77b0c755a40538a9a239ae1"
        "78b3225f516bdb9e29f6d1cb7de4f2cf74e67fd85f6d9817c48890d686d51838",
        "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"
        "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e",
        }
    };
    static const char * hithere ="Hi There";  // should be (hex) 523df363aa22c25478a478d25e6945f842509df8b77b0c755a40538a9a239ae178b3225f516bdb9e‌​29f6d1cb7de4f2cf74e67fd85f6d9817c48890d686d51838
    static const char * blankstr ="";  // should be:
/*
 *
SHA224("")
0x d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f
SHA256("")
0x e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855
SHA384("")
0x 38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b
SHA512("")
0x cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e
SHA512/224("")
0x 6ed0dd02806fa89e25de060c19d3ac86cabb87d6a0ddd05c333b84f4
SHA512/256("")
0x c672b8d1ef56ed28ab87c3622c5114069bdd3ad7b8f9737498d0c01ecef0967a
 *
 */
    static const char message1[] = "abc";
#ifndef SHA512ONLY
    static const char message2a[] = "abcdbcdecdefdefgefghfghighijhi"
                                    "jkijkljklmklmnlmnomnopnopq";
#endif
    static const char message2b[] = "abcdefghbcdefghicdefghijdefghijkefghij"
                                    "klfghijklmghijklmnhijklmnoijklmnopjklm"
                                    "nopqklmnopqrlmnopqrsmnopqrstnopqrstu";
    unsigned char *message3;
    unsigned int message3_len = 1000000;
#ifdef DOTIMING
#define NUMOFLOOPS	10000
#else
#define NUMOFLOOPS	1
#endif
//    unsigned int message3_len = 127;
    unsigned char digest[SHA512_DIGEST_SIZE];
#ifdef SSESUPPORT
    unsigned char intelmessage[SHA512_DIGEST_SIZE];
#endif	// SSESUPPORT
    time_t mytime;
    time_t mytime0;
    int i;
// note that because of the way that the PACK and UNPACK macros are defined,
// the hash algorithms should work correctly on both big and little endian machines
    unsigned int edtest = 16909060;
#ifdef WORDS_BIGENDIAN
    fprintf(stderr, "This machine is Big Endian!\n");
#else
    fprintf(stderr, "This machine is Little Endian.\n");
#endif
    fprintf(stderr, "      0x%08x\n      0x",edtest);
    for (int ixx=0;ixx<4;ixx++) fprintf(stderr,"%02x",*((unsigned char*)(&edtest)+ixx));
    fprintf(stderr,"\n\n");
    message3 = (unsigned char *)malloc(message3_len);
    if (message3 == NULL) {
        fprintf(stderr, "Can't allocate memory\n");
        return -1;
    }
    memset(message3, 'a', message3_len);
// test linux hash routine
	fprintf(stderr,"Test linux passwd hashing...\n");
	if(doHAsh ( (char *)"Y8Kk7.cZ", (char *)"password" , 5000)) return(1);
	fprintf(stderr,"should be:\n%s\n","$6$Y8Kk7.cZ$1vVest4bV/0WDJIe9fO7m/YpUOykmduM5IzDCE6Hj3W0WdrmEw8xP6vW4wxuCLaSPG/9wveo4NoUcBVAvxOeU0");
#ifndef DOTIMING
	return(0);
#endif
    fprintf(stderr,"SHA-2 FIPS 180-2 Validation tests\n\n");
#ifndef SHA512ONLY
    fprintf(stderr,"SHA-224 Test vectors\n");

    sha224((const unsigned char *) message1, strlen(message1), digest);
    test(vectors[0][0], digest, SHA224_DIGEST_SIZE);
    sha224((const unsigned char *) message2a, strlen(message2a), digest);
    test(vectors[0][1], digest, SHA224_DIGEST_SIZE);
    sha224(message3, message3_len, digest);
    test(vectors[0][2], digest, SHA224_DIGEST_SIZE);
    fprintf(stderr,"\n");

    fprintf(stderr,"SHA-256 Test vectors\n");

    sha256((const unsigned char *) message1, strlen(message1), digest);
    test(vectors[1][0], digest, SHA256_DIGEST_SIZE);
    sha256((const unsigned char *) message2a, strlen(message2a), digest);
    test(vectors[1][1], digest, SHA256_DIGEST_SIZE);
    sha256(message3, message3_len, digest);
    test(vectors[1][2], digest, SHA256_DIGEST_SIZE);
    fprintf(stderr,"\n");

    fprintf(stderr,"SHA-384 Test vectors\n");

    sha384((const unsigned char *) message1, strlen(message1), digest);
    test(vectors[2][0], digest, SHA384_DIGEST_SIZE);
    sha384((const unsigned char *)message2b, strlen(message2b), digest);
    test(vectors[2][1], digest, SHA384_DIGEST_SIZE);
    sha384(message3, message3_len, digest);
    test(vectors[2][2], digest, SHA384_DIGEST_SIZE);
    fprintf(stderr,"\n");
#endif  //  SHA512ONLY
    fprintf(stderr,"SHA-512 Test vectors\nnumber of loops = %i\n",NUMOFLOOPS);

    sha512((const unsigned char *) message1, (unsigned int)strlen(message1), digest);
    test(vectors[3][0], digest, SHA512_DIGEST_SIZE);
    sha512((const unsigned char *) message2b, (unsigned int)strlen(message2b), digest);
    test(vectors[3][1], digest, SHA512_DIGEST_SIZE);

    mytime0 = time(NULL);
    fprintf(stderr,"start time: %s",ctime(&mytime0));
    for (i=0;i<NUMOFLOOPS;i++) sha512(message3, message3_len, digest);
    mytime = time(NULL);
    fprintf(stderr,"end   time: %s",ctime(&mytime));
    fprintf(stderr,"%0.2lf hash/sec\n",((double) NUMOFLOOPS)/difftime(mytime,mytime0));
    test(vectors[3][2], digest, SHA512_DIGEST_SIZE);
    sha512((const unsigned char *)hithere, (unsigned int)strlen(hithere), digest);
    test(vectors[3][3], digest, SHA512_DIGEST_SIZE);
    sha512((const unsigned char *)blankstr, (unsigned int)strlen(blankstr), digest);
    test(vectors[3][4], digest, SHA512_DIGEST_SIZE);
    fprintf(stderr,"\n");

#ifdef SSESUPPORT
// get couinfo
#ifdef WINDOZE
  int b[4] = { -1 };

  for (int a = 0; a < 5; a++)
  {
    __cpuid(b, a);
    fprintf(stderr,"The code %i gives %i, %i, %i, %i\n",a,b[0],b[1],b[2],b[3] );
  }
#endif	// WINDOZE
   unsigned int index = 0;
   unsigned int index2 = 0;
   unsigned int regs[4];
//    int sum;
    for (index = 0; index < 8; index++)
    {
    	if (index==6)continue;
    	if (index==3)continue;
#ifndef WINDOZE
    __asm__ __volatile__(
#if defined(__x86_64__) || defined(_M_AMD64) || defined (_M_X64)
        "pushq %%rbx     \n\t" /* save %rbx */
#else	// defined...
        "pushl %%ebx     \n\t" /* save %ebx */
#endif	// defined...
        "cpuid            \n\t"
        "movl %%ebx ,%[ebx]  \n\t" /* write the result into output var */
#if defined(__x86_64__) || defined(_M_AMD64) || defined (_M_X64)
        "popq %%rbx \n\t"
#else	// defined...
        "popl %%ebx \n\t"
#endif	// defined...
        : "=a"(regs[0]), [ebx] "=r"(regs[1]), "=c"(regs[2]), "=d"(regs[3])
        : "a"(index), "c"(index2));
#else   // WINDOZE
        __cpuid(regs, index);
#endif  // WINDOZE
		if (index == 0){
			for (i=4; i<8; i++) {
				fprintf(stderr,"%c" ,((char *)regs)[i]);
			}
			for (i=12; i<16; i++) {
				fprintf(stderr,"%c" ,((char *)regs)[i]);
			}
			for (i=8; i<12; i++) {
				fprintf(stderr,"%c" ,((char *)regs)[i]);
			}
			fprintf(stderr,"\n");
		} else {
			fprintf(stderr,"The code %i gives ADX=%x, EBX=%x, ECX=%x, EDX=%x\n",index,regs[0],regs[1],regs[2],regs[3] );
			if (index == 1) {
				fprintf(stderr,"stepping=%i\n",regs[0]&0xf);
				fprintf(stderr,"model=%i\n",(regs[0]>>4)&0xf);
				fprintf(stderr,"family=%i\n",(regs[0]>>8)&0xf);
				fprintf(stderr,"processor type=%i\n",(regs[0]>>12)&0x3);
				fprintf(stderr,"extened model=%i\n",(regs[0]>>16)&0xf);
				fprintf(stderr,"extened family=%i\n",(regs[0]>>20)&0xff);
				fprintf(stderr,"sse4.1=%i\n",(regs[2]>>19)&0x1);
				fprintf(stderr,"sse4.2=%i\n",(regs[2]>>20)&0x1);
				fprintf(stderr,"avx=%i\n",(regs[2]>>28)&0x1);
			}
			if (index == 7) {
				fprintf(stderr,"avx2=%i\n",(regs[1]>>5)&0x1);
			}
		}
    }


    unsigned int uMyCpu = myCpuInfo();
    fprintf(stderr,"\nmyCpuInfo = %u\n",uMyCpu);
    fprintf(stderr,"current sha512 kernel setting is = %u\n",SetSha512Kernel(-1));
    fprintf(stderr,"new sha512 kernel setting is = %u\n\n",SetSha512Kernel((int)uMyCpu));

    fprintf(stderr,"Enhanced SHA-512 Test vectors\n");

    sha512((const unsigned char *) message1, (unsigned int)strlen(message1), digest);
    test(vectors[3][0], digest, SHA512_DIGEST_SIZE);
    sha512((const unsigned char *) message2b, (unsigned int)strlen(message2b), digest);
    test(vectors[3][1], digest, SHA512_DIGEST_SIZE);
    mytime0 = time(NULL);
    fprintf(stderr,"start time: %s",ctime(&mytime0));
    for (i=0;i<NUMOFLOOPS;i++) sha512(message3, message3_len, digest);
    mytime = time(NULL);
    fprintf(stderr,"end   time: %s",ctime(&mytime));
    fprintf(stderr,"%0.2lf hash/sec\n",((double) NUMOFLOOPS)/difftime(mytime,mytime0));
    test(vectors[3][2], digest, SHA512_DIGEST_SIZE);
    sha512((const unsigned char *)hithere, (unsigned int)strlen(hithere), digest);
    test(vectors[3][3], digest, SHA512_DIGEST_SIZE);
    sha512((const unsigned char *)blankstr, (unsigned int)strlen(blankstr), digest);
    test(vectors[3][4], digest, SHA512_DIGEST_SIZE);
    fprintf(stderr,"\n");

    fprintf(stderr,"\nsha512_sse4 Test vectors\n");

    for (i=0;i<SHA512_DIGEST_SIZE;i++)intelmessage[i]='\000';
    for (i=0;i<SHA512_DIGEST_SIZE;i++)digest[i]='\000';
//     strcpy((char *)intelmessage,hithere);
     sha512sse((const unsigned char *)intelmessage, 0, digest);
     test(vectors[3][4], digest, SHA512_DIGEST_SIZE);
     sha512sse((const unsigned char *) message1, (unsigned int)strlen(message1), digest);
     test(vectors[3][0], digest, SHA512_DIGEST_SIZE);
     sha512sse((const unsigned char *) message2b, (unsigned int)strlen(message2b), digest);
     test(vectors[3][1], digest, SHA512_DIGEST_SIZE);
     mytime0 = time(NULL);
     fprintf(stderr,"start time: %s",ctime(&mytime0));
     for (i=0;i<NUMOFLOOPS;i++) sha512sse(message3, message3_len, digest);
     mytime = time(NULL);
     fprintf(stderr,"end   time: %s",ctime(&mytime));
     fprintf(stderr,"%0.2lf hash/sec\n",((double) NUMOFLOOPS)/difftime(mytime,mytime0));
     test(vectors[3][2], digest, SHA512_DIGEST_SIZE);   //// this one fails
     sha512sse((const unsigned char *)hithere, (unsigned int)strlen(hithere), digest);
     test(vectors[3][3], digest, SHA512_DIGEST_SIZE);

     fprintf(stderr,"\n");

     fprintf(stderr,"sha512_avx Test vectors\n");

    sha512avx((const unsigned char *)intelmessage, 0, digest);
    test(vectors[3][4], digest, SHA512_DIGEST_SIZE);
    sha512avx((const unsigned char *) message1, (unsigned int)strlen(message1), digest);
    test(vectors[3][0], digest, SHA512_DIGEST_SIZE);
    sha512avx((const unsigned char *) message2b, (unsigned int)strlen(message2b), digest);
    test(vectors[3][1], digest, SHA512_DIGEST_SIZE);
    mytime0 = time(NULL);
    fprintf(stderr,"start time: %s",ctime(&mytime0));
    for (i=0;i<NUMOFLOOPS;i++) sha512avx(message3, message3_len, digest);
    mytime = time(NULL);
    fprintf(stderr,"end   time: %s",ctime(&mytime));
    fprintf(stderr,"%0.2lf hash/sec\n",((double) NUMOFLOOPS)/difftime(mytime,mytime0));
    test(vectors[3][2], digest, SHA512_DIGEST_SIZE);   //// this one fails
    sha512avx((const unsigned char *)hithere, (unsigned int)strlen(hithere), digest);
    test(vectors[3][3], digest, SHA512_DIGEST_SIZE);

    fprintf(stderr,"\n");

    fprintf(stderr,"sha512_rorx Test vectors\n");

    sha512rorx((const unsigned char *)intelmessage, 0, digest);
    test(vectors[3][4], digest, SHA512_DIGEST_SIZE);
    sha512rorx((const unsigned char *) message1, (unsigned int)strlen(message1), digest);
    test(vectors[3][0], digest, SHA512_DIGEST_SIZE);
    sha512rorx((const unsigned char *) message2b, (unsigned int)strlen(message2b), digest);
    test(vectors[3][1], digest, SHA512_DIGEST_SIZE);
    mytime0 = time(NULL);
    fprintf(stderr,"start time: %s",ctime(&mytime0));
    for (i=0;i<NUMOFLOOPS;i++) sha512rorx(message3, message3_len, digest);
    mytime = time(NULL);
    fprintf(stderr,"end   time: %s",ctime(&mytime));
    fprintf(stderr,"%0.2lf hash/sec\n",((double) NUMOFLOOPS)/difftime(mytime,mytime0));
    test(vectors[3][2], digest, SHA512_DIGEST_SIZE);   //// this one fails
    sha512rorx((const unsigned char *)hithere, (unsigned int)strlen(hithere), digest);
    test(vectors[3][3], digest, SHA512_DIGEST_SIZE);
#endif	// SSESUPPORT
    fprintf(stderr,"\n");

    fprintf(stderr,"All tests completed.\n");

    return 0;
}

#endif /* TEST_VECTORS */


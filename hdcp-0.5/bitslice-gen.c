#define __STDC_FORMAT_MACROS /* Get the PRI* macros */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

void BitSliceK_print(int K)
{
  int i, j, k, m;
  static const uint64_t mask[6] = {
    UINT64_C(0x5555555555555555), UINT64_C(0x3333333333333333), UINT64_C(0x0f0f0f0f0f0f0f0f), 
    UINT64_C(0x00ff00ff00ff00ff), UINT64_C(0x0000ffff0000ffff), UINT64_C(0x00000000ffffffff)
  };

  printf("/* Auto-generated by %s */\n"
         "static inline void BitSlice%d(int slen, bsvec_t *src, int dlen, uint32_t *dst)\n"
         "{\n"
         " bsvec_t i, a, t[32];\n"
         "\n"
         "  memcpy(t, src, slen*sizeof(bsvec_t));\n"
	 "  memset(t+slen, 0, (32-slen)*sizeof(bsvec_t));\n"
         "\n",
         __func__, K);

  m = 0;
  for (i = 1; i < 32; i <<= 1) {
    for (j = 0; j < 32; j += 2*i) {
      for (k = 0; k < i; k++) {
	if (j + k < K) {
	  printf("  a = (t[%2d] ^ (t[%2d] << %2d)) & UINT64_C(0x%016" PRIx64 ");\n", 
		 j + k, j + i + k, i, ~mask[m]);
	  printf("  t[%2d] ^= a;\n", j + k);
	  printf("  t[%2d] ^= a >> %2d;\n", j + i + k, i);
	}
      }
    }
    m++;
  }

  printf("  for (i = 0; i < 32 && i < dlen; i++)\n"
         "    dst[i] = t[i];\n"
         "  for (i = 32; i < 64 && i < dlen; i++)\n"
         "    dst[i] = t[i-32] >> 32;\n"
         "}\n");
}

void preamble_sse2(void)
{
  printf("#ifdef __SSE2__\n"
         "typedef long long int v2di __attribute__((vector_size(16)));\n"
         "#endif\n\n");
}

void BitSliceK_sse2_print(int K)
{
  static const uint64_t mask[6] = {
    UINT64_C(0xaaaaaaaaaaaaaaaa), UINT64_C(0xcccccccccccccccc), UINT64_C(0xf0f0f0f0f0f0f0f0),  
    UINT64_C(0xff00ff00ff00ff00), UINT64_C(0xffff0000ffff0000), UINT64_C(0xffffffff00000000)
  };
  int i, j, k, m;

  printf("/* Auto-generated by %s */\n"
         "static inline void BitSlice%d(int slen, bsvec_t *src, int dlen, uint32_t *dst)\n"
         "{\n"
         "  int i;\n"
         "  uint64_t a;\n"
         "  union { uint64_t t[64]; v2di vt[32]; } u;\n"
         "  v2di va;\n"
         "  static const v2di bsmask128[6] = {\n"
         "    { UINT64_C(0xaaaaaaaaaaaaaaaa), UINT64_C(0xaaaaaaaaaaaaaaaa) }, \n"
         "    { UINT64_C(0xcccccccccccccccc), UINT64_C(0xcccccccccccccccc) }, \n"
         "    { UINT64_C(0xf0f0f0f0f0f0f0f0), UINT64_C(0xf0f0f0f0f0f0f0f0) }, \n"
         "    { UINT64_C(0xff00ff00ff00ff00), UINT64_C(0xff00ff00ff00ff00) }, \n"
         "    { UINT64_C(0xffff0000ffff0000), UINT64_C(0xffff0000ffff0000) }, \n"
         "    { UINT64_C(0xffffffff00000000), UINT64_C(0xffffffff00000000) }\n"
         "  };\n"
         "\n"
         "  memset(&u, 0, sizeof(u));\n"
         "  memcpy(u.t, src, slen*sizeof(uint64_t));\n",
         __func__, K);

  m = 0;
  i = 1;
  for (j = 0; j < 32; j += 2*i) {
    for (k = 0; k < i; k++) {
      if (j + k < K) {
        printf("  a = (u.t[%d] ^ (u.t[%d] << %d)) & UINT64_C(0x%016" PRIx64 ");\n"
               "  u.t[%d] ^= a;\n"
               "  u.t[%d] ^= a >> %d;\n",
               j + k, j + i + k, i, mask[m],
               j + k,
               j + i + k, i);
      }
    }
  }
  m++;

  for (i = 1; i < 16; i <<= 1) {
    for (j = 0; j < 16; j += 2*i) {
      for (k = 0; k < i; k++) {
        if (j + k < K) {
          printf("    va = (u.vt[%d] ^ __builtin_ia32_psllqi128(u.vt[%d], %d)) & bsmask128[%d];\n"
                 "    u.vt[%d] ^= va;\n"
                 "    u.vt[%d] ^= __builtin_ia32_psrlqi128(va, %d);\n",
                 j + k, j + i + k, 2*i, m,
                 j + k,
                 j + i + k, 2*i);
        }
      }
    }
    m++;
  }

  printf("  for (i = 0; i < dlen; i++)\n"
         "    dst[i] = u.t[i & 0x1f] >> (i & 0xe0);\n"
         "}\n");
}

void BitSliceK_all(int K)
{
  printf("#ifdef __SSE2__\n\n");
  BitSliceK_sse2_print(K);
  printf("\n#else\n\n");
  BitSliceK_print(K);
  printf("\n#endif /* __SSE2__ */\n");
}

void preamble_all(void)
{
  /* preamble() */
  preamble_sse2();
}

int main(int argc, char **argv)
{
  preamble_all();
  BitSliceK_all(24);
  BitSliceK_all(32);

  return 0;
}

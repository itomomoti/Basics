#ifndef INCLUDE_GUARD_BitsUtil
#define INCLUDE_GUARD_BitsUtil

#include <stdint.h> // include uint64_t etc.
#include <assert.h>

namespace bits{
  extern const uint64_t TBL_LoBits[];

  extern const uint64_t * const TBL_LoBitsPlus;

  extern const uint8_t TBL_PopCnt8[];

  extern const uint8_t * const TBL_Sel8;

  extern const uint8_t * const TBL_Sel8_0base;


  /**
     return the maximum uint value with w bits (w in [0, 64])
   */
  inline uint64_t UINTW_MAX(uint8_t w) {
    return TBL_LoBits[w];
  }


  inline uint8_t cnt64(uint64_t val) {
    return __builtin_popcountll(val);
  }


  inline uint8_t bitSize(uint64_t val) {
    if(val) {
      return 64 - __builtin_clzll(val);
    }
    return 1;
  }


  /* i in [1, 64]
   * the answer shoud be in the word w
   */
  inline uint8_t sel64(uint64_t w, uint64_t i) {
    for (int j = 0; j < 7; ++j) {
      const uint8_t x = w & 0xff;
      const uint64_t c = TBL_PopCnt8[x];
      if (i <= c) {
        return j*8 + TBL_Sel8[(i<<8) + x];
      }
      i -= c;
      w >>= 8;
    }
    return 56 + TBL_Sel8[(i<<8) + w];
  }


  inline uint64_t sel(const uint64_t * words, uint64_t i) {
    uint64_t ret = 0;
    uint8_t cnt = cnt64(*words);
    while (i > cnt) {
      ret += 64;
      i -= cnt;
      cnt = cnt64(*++words);
    };
    return sel64(*words, i);
  }


  inline uint64_t cnt(const uint64_t * words, uint64_t i)
  {
    uint64_t ret = 0;
    while (i >= 64) {
      ret += cnt64(*(words++));
      i -= 64;
    }
    ret += cnt64((*words) >> (64 - i - 1));
    return ret;
  }


  inline uint64_t readWBitsHead(const uint64_t * p, const uint8_t offset, const uint8_t w, const uint64_t mask) {
    const uint8_t loBits = 64 - offset;
    uint64_t ret = (*p) >> offset;
    if (loBits < w) {
      ++p;
      ret |= *p << loBits;
    }
    return ret & mask;
  }


  inline uint64_t readWBits(const uint64_t * p, const uint64_t pos, const uint8_t w, const uint64_t mask) {
    // return readWBitsHead(p + (pos >> 6), pos & 0x3f, w, mask); // Unfortunately, this single-line code is slower than the below (just expanded) code
    p += (pos >> 6);
    const uint8_t offset = pos & 0x3f;
    const uint8_t loBits = 64 - offset;
    uint64_t ret = (*p) >> offset;
    if (loBits < w) {
      ++p;
      ret |= *p << loBits;
    }
    return ret & mask;
  }


  inline uint64_t readWBitsInWord(const uint64_t * p, const uint8_t offset, const uint64_t mask) {
    uint64_t ret = (*p) >> offset;
    return ret & mask;
  }


  inline void writeWBitsHead(const uint64_t val, uint64_t * p, const uint8_t offset, const uint8_t w, const uint64_t mask) {
    const uint8_t loBits = 64 - offset;
    *p &= ~(mask << offset);
    *p |= (val << offset);
    if (loBits < w) {
      ++p;
      *p &= ~(mask >> loBits);
      *p |= val >> loBits;
    }
  }


  inline void writeWBits(const uint64_t val, uint64_t * p, const uint64_t pos, const uint8_t w, const uint64_t mask) {
    // writeWBitsHead(val, p + (pos >> 6), pos & 0x3f, w, mask); // Unfortunately, this single-line code is slower than the below (just expanded) code
    p += (pos >> 6);
    const uint8_t offset = pos & 0x3f;
    const uint8_t loBits = 64 - offset;
    *p &= ~(mask << offset);
    *p |= (val << offset);
    if (loBits < w) {
      ++p;
      *p &= ~(mask >> loBits);
      *p |= val >> loBits;
    }
  }


  inline void mvBitsBwd_DiffOffs(const uint64_t * src, uint8_t srcOffset, uint64_t * tgt, uint8_t tgtOffset, uint64_t bits) {
    assert(srcOffset != tgtOffset);
    assert(srcOffset <= 63);
    assert(tgtOffset <= 63);
    uint8_t diff1, diff2;
    uint64_t val = *src >> srcOffset;
    val <<= tgtOffset;
    val |= *tgt & UINTW_MAX(tgtOffset);
    if (srcOffset > tgtOffset) {
      diff1 = srcOffset - tgtOffset;
      diff2 = 64 - diff1;
      val |= *++src << diff2;
    } else {
      diff2 = tgtOffset - srcOffset;
      diff1 = 64 - diff2;
    }
    for (bits += tgtOffset; bits > 64; bits -= 64) {
      *tgt++ = val;
      val = *src >> diff1;
      val |= *++src << diff2;
    }
    const uint64_t mask = UINTW_MAX(bits);
    *tgt &= ~mask;
    *tgt |= val & mask;
  }


  inline void mvBitsFwd_DiffOffs(const uint64_t * src, uint8_t srcOffset, uint64_t * tgt, uint8_t tgtOffset, uint64_t bits) {
    assert(srcOffset != tgtOffset);
    assert(srcOffset <= 63);
    assert(tgtOffset <= 63);
    uint8_t diff1, diff2;
    uint64_t val = *src & UINTW_MAX(srcOffset);
    if (srcOffset > tgtOffset) {
      diff2 = srcOffset - tgtOffset;
      diff1 = 64 - diff2;
      val >>= diff2;
    } else {
      diff1 = tgtOffset - srcOffset;
      diff2 = 64 - diff1;
      val <<= diff1;
      val |= *--src >> diff2;
    }
    val |= *tgt & ~UINTW_MAX(tgtOffset);
    for (bits += 64 - tgtOffset; bits > 64; bits -= 64) {
      *tgt-- = val;
      val = *src << diff1;
      val |= *--src >> diff2;
    }
    const uint64_t mask = UINTW_MAX(64 - bits);
    *tgt &= mask;
    *tgt |= val & ~mask;
  }


  inline void mvBitsBwd_SameOffs(const uint64_t * src, uint64_t * tgt, uint8_t offset, uint64_t bits) {
    assert(offset <= 63);
    const uint64_t mask1 = UINTW_MAX(offset);
    uint64_t val = *tgt & mask1;
    val |= *src & ~mask1;
    for (bits += offset; bits > 64; bits -= 64) {
      *tgt++ = val;
      val = *++src;
    }
    const uint64_t mask2 = UINTW_MAX(bits);
    *tgt &= ~mask2;
    *tgt |= val & mask2;
  }


  inline void mvBitsFwd_SameOffs(const uint64_t * src, uint64_t * tgt, uint8_t offset, uint64_t bits) {
    assert(offset <= 63);
    const uint64_t mask1 = UINTW_MAX(offset);
    uint64_t val = *src & mask1;
    val |= *tgt & ~mask1;
    for (bits += 64 - offset; bits > 64; bits -= 64) {
      *tgt-- = val;
      val = *--src;
    }
    const uint64_t mask2 = UINTW_MAX(64 - bits);
    *tgt &= mask2;
    *tgt |= val & ~mask2;
  }


  inline void mvBitsBwd(const uint64_t * src, uint8_t srcOffset, uint64_t * tgt, uint8_t tgtOffset, uint64_t bits) {
    if (srcOffset != tgtOffset) {
      mvBitsBwd_DiffOffs(src, srcOffset, tgt, tgtOffset, bits);
    } else {
      mvBitsBwd_SameOffs(src, tgt, srcOffset, bits);
    }
  }


  // NOTE: The pair <src, srcOffset> points to the ending (right next) position of the bits to be copied
  inline void mvBitsFwd(const uint64_t * src, uint8_t srcOffset, uint64_t * tgt, uint8_t tgtOffset, uint64_t bits) {
    if (srcOffset != tgtOffset) {
      mvBitsFwd_DiffOffs(src, srcOffset, tgt, tgtOffset, bits);
    } else {
      mvBitsFwd_SameOffs(src, tgt, srcOffset, bits);
    }
  }


  inline void mvBits(const uint64_t * src, uint8_t srcOffset, uint64_t * tgt, uint8_t tgtOffset, uint64_t bits) {
    if (src < tgt || (src == tgt && srcOffset < tgtOffset)) { // RL should be used: values are copied from Right to Left
      const uint64_t srcBits = bits + srcOffset;
      const uint64_t tgtBits = bits + tgtOffset;
      mvBitsFwd(src + (srcBits >> 6), srcBits & 0x3f, tgt + (tgtBits >> 6), tgtBits & 0x3f, bits);
    } else { // LR should be used: values are copied from Left to Right
      mvBitsBwd(src, srcOffset, tgt, tgtOffset, bits);
    }
  }
}




#endif

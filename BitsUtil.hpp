/*!
 * Copyright (c) 2017 Tomohiro I
 *
 * This program is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
/*!
 * @file BitsUtil.hpp
 * @brief useful functions to work on bits.
 * @author Tomohiro I
 * @date 2017-02-23
 * 
 * @note
 *  For a uint64_t array words[0..n) of length n, let words[[0..64*n)) denote a bit vector defined as follows:
 *  The i-th bit of words[[0..64*n)) is the (i%64)-th (low significant) bit of words[i/64].
 *  We use the following terminology:
 *  - bit-pos: position (0base) in a bit vector.
 *  - bit-region: sub-vector of a bit vector.
 *  - offset: bit-pos in a word, i.e., offset is in [0, 64).
 */
#ifndef INCLUDE_GUARD_BitsUtil
#define INCLUDE_GUARD_BitsUtil

#include <stdint.h> // include uint64_t etc.
#include <assert.h>


/*!
 * @namespace ctcbits
 * @brief bits functions specialized to run in compile time.
 */
namespace ctcbits
{
  /*!
   * @brief compile time specialized version of bits::UINTW_MAX.
   */
  constexpr uint64_t UINTW_MAX
  (
   uint8_t w //!< bit-width in [0, 64].
   ) {
    if (w >= 64) {
      return UINT64_MAX;
    } else {
      return (UINT64_C(1) << w) - 1;
    }
  }
}


/*!
 * @namespace bits
 * @brief useful functions to work on bits.
 */
namespace bits
{
  extern const uint64_t TBL_LoBits[];

  extern const uint64_t * const TBL_LoBitsPlus;

  extern const uint8_t TBL_PopCnt8[];

  extern const uint8_t * const TBL_Sel8;

  extern const uint8_t * const TBL_Sel8_0base;


  /*!
   * @brief return the maximum 64bit uint with 'w' bits.
   */
  inline uint64_t UINTW_MAX
  (
   uint8_t w //!< bit-width in [0, 64].
   ) {
    assert(w <= 64);

    return TBL_LoBits[w];
  }


  /*!
   * @brief return popcount(val).
   */
  constexpr uint8_t cnt64
  (
   uint64_t val
   ) {
    return __builtin_popcountll(val);
  }


  /*!
   * @brief return # of bits needed to represent 'val' (where we think that 0 needs 1 bit).
   */
  constexpr uint8_t bitSize
  (
   uint64_t val
   ) {
    if(val) {
      return 64 - __builtin_clzll(val);
    }
    return 1;
  }


  /*!
   * @brief return bit-pos of the 'rank'-th 1 in 'word'.
   */
  inline uint8_t sel64
  (
   uint64_t word, //!< 64bit uint
   uint64_t rank //!< in [1, 64].
   ) {
    assert(1 <= rank && rank <= 64);
    assert(cnt64(word) >= rank); //! @pre word must contain at least 'rank' 1's.

    for (int j = 0; j < 7; ++j) {
      const uint8_t x = word & 0xff;
      const uint64_t c = TBL_PopCnt8[x];
      if (rank <= c) {
        return j*8 + TBL_Sel8[(rank<<8) + x];
      }
      rank -= c;
      word >>= 8;
    }
    return 56 + TBL_Sel8[(rank<<8) + word];
  }


  /*!
   * @brief return bit-pos of the 'rank'-th 1 in words[0..].
   * @pre 'rank'-th 1 must be found before going out of bounds.
   */
  inline uint64_t sel
  (
   const uint64_t * words,
   uint64_t rank
   ) {
    uint64_t ret = 0;
    uint8_t cnt = cnt64(*words);
    while (rank > cnt) {
      ret += 64;
      rank -= cnt;
      cnt = cnt64(*++words);
    };
    return sel64(*words, rank);
  }


  /*!
   * @brief return # of 1's in words[[0..i]].
   * @pre the bit-region must not be out of bounds.
   */
  inline uint64_t cnt
  (
   const uint64_t * words, //!< pointer to uint64_t array
   uint64_t i //!< bit-pos specifying the last position of the bit-region.
   ) {
    uint64_t ret = 0;
    while (i >= 64) {
      ret += cnt64(*(words++));
      i -= 64;
    }
    ret += cnt64((*words) >> (64 - i - 1));
    return ret;
  }


  /*!
   * @brief read 'w'-bits written in the bit-region beginning at p[[offset..]].
   * @return value represented by p[[offset..offset+w)).
   * @pre the bit-region must not be out of bounds.
   */
  inline uint64_t readWBitsHead
  (
   const uint64_t * p, //!< pointer to words that contains the beginning of the bit-region.
   const uint8_t offset, //!< in [0, 64).
   const uint8_t w, //!< bit-width in [0, 64].
   const uint64_t mask //!< UINTW_MAX(w).
   ) {
    assert(offset < 64);
    assert(w <= 64);

    const uint8_t loBits = 64 - offset;
    uint64_t ret = (*p) >> offset;
    if (loBits < w) {
      ++p;
      ret |= *p << loBits;
    }
    return ret & mask;
  }


  /*!
   * @brief read 'w'-bits written in the bit-region beginning at p[[bitPos..]].
   * @return value represented by p[[bitPos..bitPos+w)).
   * @pre the bit-region must not be out of bounds.
   */
  inline uint64_t readWBits
  (
   const uint64_t * p, //!< pointer to words.
   const uint64_t bitPos, //!< bit-pos specifying the beginning position of the bit-region
   const uint8_t w, //!< bit-width in [0, 64].
   const uint64_t mask //!< UINTW_MAX(w).
   ) {
    assert(w <= 64);

    p += (bitPos >> 6);
    const uint8_t offset = bitPos & 0x3f;
    const uint8_t loBits = 64 - offset;
    uint64_t ret = (*p) >> offset;
    if (loBits < w) {
      ++p;
      ret |= *p << loBits;
    }
    return ret & mask;
  }


  /*!
   * @brief simplified version of ::readWBitsHead. This function does not read inter-words bits.
   */
  inline uint64_t readWBitsInWord
  (
   const uint64_t * p,
   const uint8_t offset,
   const uint64_t mask
   ) {
    assert(offset < 64);

    uint64_t ret = (*p) >> offset;
    return ret & mask;
  }


  /*!
   * @brief write 'w'-bit value 'val' to the bit-region beginning at p[[offset..]].
   * @pre the bit-region must not be out of bounds.
   */
  inline void writeWBitsHead
  (
   const uint64_t val, //!< in [0, 2^w).
   uint64_t * p, //!< pointer to words that contains the beginning of the bit-region.
   const uint8_t offset, //!< in [0, 64).
   const uint8_t w, //!< bit-width in [0, 64].
   const uint64_t mask //!< UINTW_MAX(w).
   ) {
    assert(offset < 64);
    assert(w <= 64);
    assert(val == 0 || bitSize(val) <= w);

    const uint8_t loBits = 64 - offset;
    *p &= ~(mask << offset);
    *p |= (val << offset);
    if (loBits < w) {
      ++p;
      *p &= ~(mask >> loBits);
      *p |= val >> loBits;
    }
  }


  /*!
   * @brief write 'w'-bit value 'val' to the bit-region beginning at p[[bitPos..]].
   * @pre the bit-region must not be out of bounds.
   */
  inline void writeWBits
  (
   const uint64_t val, //!< in [0, 2^w).
   uint64_t * p, //!< p pointer to words.
   const uint64_t bitPos, //!< bit-pos.
   const uint8_t w, //!< w bit-width in [0, 64].
   const uint64_t mask //!< UINTW_MAX(w).
   ) {
    assert(w <= 64);
    assert(val == 0 || bitSize(val) <= w);

    p += (bitPos >> 6);
    const uint8_t offset = bitPos & 0x3f;
    const uint8_t loBits = 64 - offset;
    *p &= ~(mask << offset);
    *p |= (val << offset);
    if (loBits < w) {
      ++p;
      *p &= ~(mask >> loBits);
      *p |= val >> loBits;
    }
  }


  inline void mvBitsBwd_DiffOffs
  (
   const uint64_t * src,
   uint8_t srcOffset,
   uint64_t * tgt,
   uint8_t tgtOffset,
   uint64_t bitLen
   ) {
    assert(srcOffset != tgtOffset); //! @pre srcOffset and tgtOffset are different.
    assert(srcOffset <= 63);
    assert(tgtOffset <= 63);
    assert(src >= tgt); //! @pre 'position of src-region' >= 'position of tgt-region'.

    uint8_t diff1, diff2;
    uint64_t val = *src >> srcOffset;
    val <<= tgtOffset;
    val |= *tgt & UINTW_MAX(tgtOffset);
    bitLen += tgtOffset;
    if (srcOffset > tgtOffset) {
      diff1 = srcOffset - tgtOffset;
      diff2 = 64 - diff1;
      if (bitLen <= diff2) {
        goto last;
      }
      val |= *++src << diff2;
    } else {
      diff2 = tgtOffset - srcOffset;
      diff1 = 64 - diff2;
    }
    while (bitLen > 64) {
      *tgt++ = val;
      bitLen -= 64;
      val = *src >> diff1;
      if (bitLen <= diff2) { goto last; }
      val |= *++src << diff2;
    }
  last:
    const uint64_t mask = UINTW_MAX(bitLen);
    *tgt &= ~mask;
    *tgt |= val & mask;
  }


  inline void mvBitsFwd_DiffOffs
  (
   const uint64_t * src,
   uint8_t srcOffset,
   uint64_t * tgt,
   uint8_t tgtOffset,
   uint64_t bitLen
   ) {
    assert(srcOffset != tgtOffset); //! @pre srcOffset and tgtOffset are different.
    assert(srcOffset <= 63);
    assert(tgtOffset <= 63);
    assert(src < tgt || (src == tgt && srcOffset < tgtOffset)); //! @pre 'position of src-region' < 'position of tgt-region'.

    if (srcOffset == 0) {
      srcOffset = 64;
      --src;
    } else if (tgtOffset == 0) {
      tgtOffset = 64;
      --tgt;
    }
    uint8_t diff1, diff2;
    uint64_t val = *tgt & ~UINTW_MAX(tgtOffset);
    bitLen += 64 - tgtOffset;
    if (srcOffset > tgtOffset) {
      diff2 = srcOffset - tgtOffset;
      diff1 = 64 - diff2;
      val |= (*src & UINTW_MAX(srcOffset)) >> diff2;
    } else {
      diff1 = tgtOffset - srcOffset;
      diff2 = 64 - diff1;
      val |= (*src & UINTW_MAX(srcOffset)) << diff1;
      if (bitLen <= diff2) {
        goto last;
      }
      val |= *(--src) >> diff2;
    }
    while (bitLen > 64) {
      *tgt-- = val;
      bitLen -= 64;
      val = *src << diff1;
      if (bitLen <= diff2) { goto last; }
      val |= *(--src) >> diff2;
    }
  last:
    const uint64_t mask = UINTW_MAX(64 - bitLen);
    *tgt &= mask;
    *tgt |= val & ~mask;
  }


  inline void mvBitsBwd_SameOffs
  (
   const uint64_t * src,
   uint64_t * tgt,
   uint8_t offset,
   uint64_t bitLen
   ) {
    assert(offset <= 63); //! @pre offset in [0..64).
    assert(src >= tgt); //! @pre 'position of src-region' >= 'position of tgt-region'.

    const uint64_t mask1 = UINTW_MAX(offset);
    uint64_t val = *tgt & mask1;
    val |= *src & ~mask1;
    for (bitLen += offset; bitLen > 64; bitLen -= 64) {
      *tgt++ = val;
      val = *++src;
    }
    const uint64_t mask2 = UINTW_MAX(bitLen);
    *tgt &= ~mask2;
    *tgt |= val & mask2;
  }


  inline void mvBitsFwd_SameOffs
  (
   const uint64_t * src,
   uint64_t * tgt,
   uint8_t offset,
   uint64_t bitLen
   ) {
    assert(offset <= 63); //! @pre offset in [0..64).
    assert(src < tgt); //! @pre 'position of src-region' < 'position of tgt-region'.

    if (offset == 0) {
      offset = 64;
      --src;
      --tgt;
    }
    const uint64_t mask1 = UINTW_MAX(offset);
    uint64_t val = *src & mask1;
    val |= *tgt & ~mask1;
    for (bitLen += 64 - offset; bitLen > 64; bitLen -= 64) {
      *tgt-- = val;
      val = *--src;
    }
    const uint64_t mask2 = UINTW_MAX(64 - bitLen);
    *tgt &= mask2;
    *tgt |= val & ~mask2;
  }


  /*!
   * @brief move bits backwardly from src-region to tgt-region (see also ::mvBits).
   * @attention this function should be used when bits are moved backwardly, i.e., 'position of src-region' >= 'position of tgt-region'.
   */
  inline void mvBitsBwd
  (
   const uint64_t * src,
   uint8_t srcOffset,
   uint64_t * tgt,
   uint8_t tgtOffset,
   uint64_t bitLen
   ) {
    assert(src > tgt || (src == tgt && srcOffset >= tgtOffset)); //! @pre 'position of src-region' >= 'position of tgt-region'.

    if (srcOffset != tgtOffset) {
      mvBitsBwd_DiffOffs(src, srcOffset, tgt, tgtOffset, bitLen);
    } else {
      mvBitsBwd_SameOffs(src, tgt, srcOffset, bitLen);
    }
  }


  /*!
   * @brief move bits forwardly from src-region to tgt-region (see also ::mvBits).
   * @attention this function should be used when bits are moved forwardly, i.e., 'position of src-region' < 'position of tgt-region'.
   * @attention src-region is designated by the bit-region *ending (excluded)* at 'srcOffset' bit of 'src' and of length 'bitLen'.
   *            tgt-region is similarly defined.
   */
  inline void mvBitsFwd
  (
   const uint64_t * src,
   uint8_t srcOffset,
   uint64_t * tgt,
   uint8_t tgtOffset,
   uint64_t bitLen
   ) {
    assert(src < tgt || (src == tgt && srcOffset < tgtOffset)); //! @pre 'position of src-region' < 'position of tgt-region'

    if (srcOffset != tgtOffset) {
      mvBitsFwd_DiffOffs(src, srcOffset, tgt, tgtOffset, bitLen);
    } else {
      mvBitsFwd_SameOffs(src, tgt, srcOffset, bitLen);
    }
  }


  /*!
   * @brief move bits from src-region to tgt-region.
   * @attention when src-region and tgt-region overlap, the overlapped part of src-region is overwritten.
   *            The other part of src-region is not changed.
   * @note
   *  the process will be forwarded to one of the four functions based on the following two (orthogonal) criteria.
   *  - move bits forwardly or backwardly.
   *  - offsets are same or not.
   */
  inline void mvBits
  (
   const uint64_t * src,
   uint8_t srcOffset,
   uint64_t * tgt,
   uint8_t tgtOffset,
   uint64_t bitLen
   ) {
    if (src < tgt || (src == tgt && srcOffset < tgtOffset)) {
      const uint64_t srcBits = bitLen + srcOffset; // 'src + srcBitLen' points to the end bit position of src-region
      const uint64_t tgtBits = bitLen + tgtOffset;
      mvBitsFwd(src + (srcBits >> 6), srcBits & 0x3f, tgt + (tgtBits >> 6), tgtBits & 0x3f, bitLen);
    } else {
      mvBitsBwd(src, srcOffset, tgt, tgtOffset, bitLen);
    }
  }
}




#endif

/*!
 * Copyright (c) 2017 Tomohiro I
 *
 * This program is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
/*!
 * @file RankVec.hpp
 * @brief Bit vector supporting fast rank queries and select queries based on it.
 * @author Tomohiro I
 * @date 2017-08-20
 */
#ifndef INCLUDE_GUARD_RankVec
#define INCLUDE_GUARD_RankVec

#include <stdint.h> // include uint64_t etc.
#include <assert.h>

#include <iostream>
#include <iterator>
#include <algorithm>

#include "BasicSearch.hpp"
#include "BitVec.hpp"
#include "BitsUtil.hpp"
#include "MemUtil.hpp"

/*!
 * @brief Bit vector supporting fast rank queries and select queries based on it.
 * @tparam BSIZE_M: Block size of a middle block. It should be a power of two.
 * @tparam BSIZE_T: Block size of a top block. It should be a power of two (strictly) smaller than 2^16.
 */
template<uint16_t BSIZE_M = 256, uint16_t BSIZE_T = 4096>
class RankVec
{
  BitVec bv;
  uint16_t * blockM_;
  uint64_t * blockT_;

public:
  RankVec
  (
   size_t capacity = 0 //!< Initial capacity.
   ) : bv(), blockM_(nullptr), blockT_(nullptr) {
    assert(capacity <= ctcbits::UINTW_MAX(58));

    reserve(capacity);
  }


  /*!
   * @brief Get read-only array pointer.
   */
  const uint64_t * getConstArrayPtr() const noexcept
  {
    return bv.getConstArrayPtr();
  }


  /*!
   * @brief Read 'w'-bits written in the bit-region beginning at array_[[bitPos..]].
   * @return Value represented by array_[[bitPos..bitPos+w)).
   * @pre The bit-region must not be out of bounds.
   */
  uint64_t readWBits
  (
   const uint64_t bitPos, //!< Bit-pos specifying the beginning position of the bit-region
   const uint8_t w, //!< Bit-width in [0, 64].
   const uint64_t mask //!< UINTW_MAX(w).
   ) const noexcept {
    assert(w <= 64);

    return bv.readWBits(bitPos, w, mask);
  }


  /*!
   * @brief Simplified version of ::readWBits that can be used when reading bits in a single word.
   */
  uint64_t readWBits_S
  (
   const uint64_t bitPos, //!< Bit-pos specifying the beginning position of the bit-region
   const uint64_t mask //!< UINTW_MAX(w).
   ) const noexcept {
    assert(bits::bitSize(mask) + (bitPos & 0x3f) <= 64);

    return bv.readWBits_S(bitPos, mask);
  }


  /*!
   * @brief Read bit at 'bitPos'.
   */
  uint64_t readBit
  (
   const uint64_t bitPos //!< in [0, capacity).
   ) const noexcept {
    assert(bitPos < bv.capacity());

    return bv.readWBits_S(bitPos);
  }


  /*!
   * @brief Write a bit 'val' at 'bitPos'.
   * @pre The resulting size should be within capacity.
   */
  void appendBit
  (
   const bool val //!< Bool value.
   ) {
    assert(bv.size() < bv.capacity());

    const auto pos = bv.size();
    bv.resize(pos + 1);
    bv.writeBit(val, pos);

    if (pos == 0) {
      blockT_[0] = blockM_[0] = val;
      return;
    }

    const auto idxT = pos / BSIZE_T;
    const auto remT = pos % BSIZE_T;
    const auto idxM = (pos / BSIZE_M) - idxT;
    if ((pos % BSIZE_M) == 0) {
      if (remT == 0) {
        blockT_[idxT] = blockT_[idxT - 1];
        blockM_[idxM] = 0;
      } else if (remT < BSIZE_T - BSIZE_M) {
        blockM_[idxM] = blockM_[idxM - 1];
      }
    }
    blockT_[idxT] += val;
    if (remT < BSIZE_T - BSIZE_M) {
      blockM_[idxM] += val;
    }
  }


  /*!
   * @brief Rank query.
   */
  uint64_t rank
  (
   const uint64_t pos //!< pos in [0, size).
   ) const noexcept {
    assert(pos < bv.size());

    const auto idxT = pos / BSIZE_T;
    const auto remT = pos % BSIZE_T;
    const auto idxM = (pos / BSIZE_M) - idxT;
    uint64_t rank = (idxT)? blockT_[idxT - 1] : 0;
    if (remT >= BSIZE_M) {
      rank += blockM_[idxM - 1];
    }
    return rank + bits::cnt(bv.getConstArrayPtr() + (pos / BSIZE_M * BSIZE_M / 64), pos % BSIZE_M);
  }


  /*!
   * @brief Select query.
   * @pre The answer must be found.
   * @attention O(log size) time.
   */
  uint64_t select
  (
   uint64_t rank //!< 1base rank [1, rank(bv.size()-1)].
   ) const noexcept {
    assert(rank > 0);
    assert(rank <= blockT_[(bv.size() - 1) / BSIZE_T]);

    const auto idxT = basic_search::lb(blockT_, rank, 0, ((bv.size() - 1) / BSIZE_T) + 1);
    const auto posT = idxT * BSIZE_T;
    rank -= (idxT > 0)? blockT_[idxT - 1] : 0;
    const auto idxM = (posT / BSIZE_M) - idxT;
    uint64_t i = 0;
    while (i < (BSIZE_T / BSIZE_M) - 1 && blockM_[idxM + i] < rank) {
      ++i;
    }
    rank -= (i > 0)? blockM_[idxM + i - 1] : 0;
    const auto posM = posT + i * BSIZE_M;
    return posM + bits::sel(bv.getConstArrayPtr() + (posM / 64), rank);
  }


  /*!
   * @brief Predecessor query.
   * @note Here bit vector is considered to represent a set of uint by the set of positions of set bits.
   * @return The largest set bit position smaller than or equal to val. Return UINT64_MAX when not found.
   */
  uint64_t pred
  (
   uint64_t val //!< val (>= 0).
   ) const noexcept {
    const auto size = bv.size();
    if (size == 0) {
      return UINT64_MAX;
    } else if (val >= size) {
      val = size - 1;
    }

    const auto ans = bits::pred(getConstArrayPtr(), val, std::min(UINT64_C(2), 1 + val / 64));
    if (ans != UINT64_MAX) {
      return ans;
    }
    const auto r = rank(val);
    if (r > 0) {
      return select(r);
    } else {
      return UINT64_MAX;
    }
  }


  /*!
   * @brief Successor query.
   * @note Here bit vector is considered to represent a set of uint by the set of positions of set bits.
   * @return The smallest set bit position larger than or equal to val. Return UINT64_MAX when not found.
   */
  uint64_t succ
  (
   uint64_t val //!< val (>= 0).
   ) const noexcept {
    const auto size = bv.size();
    if (val >= size) {
      return UINT64_MAX;
    }

    const auto ans = bits::succ(getConstArrayPtr(), val, std::min(UINT64_C(2), 1 + (size - val - 1) / 64));
    if (ans < size) {
      return ans;
    }
    const auto r = rank(val);
    if (r < blockT_[(bv.size() - 1) / BSIZE_T]) {
      return select(r+1);
    } else {
      return UINT64_MAX;
    }
  }


  /*!
   * @brief Get current capacity.
   */
  size_t capacity() const noexcept {
    return bv.capacity();
  }


  /*!
   * @brief Get current size.
   */
  size_t size() const noexcept {
    return bv.size();
  }


  /*!
   * @brief Calculate total memory usage in bytes.
   */
  size_t calcMemBytes() const noexcept {
    const auto capacity = bv.capacity();
    const auto lenT = (capacity + BSIZE_T - 1) / BSIZE_T;
    const auto lenM = (capacity + BSIZE_M - 1) / BSIZE_M - lenT;
    size_t bytes = bv.calcMemBytes();
    bytes += sizeof(blockM_[0]) * lenM;
    bytes += sizeof(blockT_[0]) * lenT;
    bytes += sizeof(*this);
    return bytes;
  }


  /*!
   * @brief Return if no element is stored.
   */
  bool empty() const noexcept {
    return bv.empty();
  }


  /*!
   * @brief Clear vector. It only changes size to zero.
   */
  void clear() noexcept {
    bv.resize(0);
  }


  /*!
   * @brief Reserve.
   * @note
   *   This function does not shrink 'capacity_'
   *   If 'newCapacity > oldCapacity', realloc arrays. 'size' is unchanged.
  */
  void reserve
  (
   const size_t newCapacity
   ) {
    assert(newCapacity <= ctcbits::UINTW_MAX(58));

    const auto oldCapacity = bv.capacity();
    if (newCapacity > oldCapacity) {
      bv.reserve(newCapacity);
      const auto lenT = (newCapacity + BSIZE_T - 1) / BSIZE_T;
      const auto lenM = (newCapacity + BSIZE_M - 1) / BSIZE_M - lenT;
      memutil::realloc_AbortOnFail<uint16_t>(blockM_, lenM);
      memutil::realloc_AbortOnFail<uint64_t>(blockT_, lenT);
    }
  }


  /*!
   * @brief Shrink vector to fit current bit-length in use.
   */
  void shrink_to_fit() {
    const auto size = bv.size();
    if (size > 0) {
      bv.shrink_to_fit();
      const auto lenT = (size + BSIZE_T - 1) / BSIZE_T;
      const auto lenM = (size + BSIZE_M - 1) / BSIZE_M - lenT;
      memutil::realloc_AbortOnFail<uint16_t>(blockM_, lenM);
      memutil::realloc_AbortOnFail<uint64_t>(blockT_, lenT);
    } else {
      memutil::safefree(blockM_);
      memutil::safefree(blockT_);
    }
  }


  void printDebugInfo() const {
    const auto lenT = (bv.size() + BSIZE_T - 1) / BSIZE_T;
    const auto lenM = (bv.size() + BSIZE_M - 1) / BSIZE_M - lenT;
    for (uint64_t i = 0; i < lenT; ++i) {
      std::cout << blockT_[i] << ", ";
    }
    std::cout << std::endl;
    for (uint64_t i = 0; i < lenM; ++i) {
      std::cout << blockM_[i] << ", ";
    }
    std::cout << std::endl;
    bv.printDebugInfo();
  }
};

#endif

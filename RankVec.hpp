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
   ) {
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
   ) {
    assert(bits::bitSize(mask) + (bitPos & 0x3f) <= 64);

    return bv.readWBits_S(bitPos, mask);
  }


  /*!
   * @brief Read bit at 'bitPos'.
   */
  uint64_t readBit
  (
   const uint64_t bitPos //!< in [0, capacity).
   ) const {
    assert(bitPos < bv.capacity());

    return bv.readWBits_S(bitPos);
  }


  /*!
   * @brief Write a bit 'val' at 'bitPos'.
   * @attention It does not check out-of-bounds error.
   */
  // void appendBit
  // (
  //  const bool val, //!< Bool value.
  //  ) {
  //   assert(bv.size() < bv.capacity());

  //   bv.writeBit(val, bv.size());
  // }


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
      memutil::realloc_AbortOnFail<uint64_t>(blockM_, lenM);
      memutil::realloc_AbortOnFail<uint64_t>(blockM_, lenT);
    }
  }


  /*!
   * @brief Resize 'size' to 'newSize'. It realloc arrays if needed.
   * @note
   *   This function does not shrink 'capacity'
   *   If 'newSize > capacity', realloc arrays by calling RankVec::reserve.
  */
  void resize
  (
   const size_t newSize
   ) {
    assert(newSize <= ctcbits::UINTW_MAX(58));

    const auto capacity = bv.capacity();
    if (newSize > capacity) {
      reserve(newSize);
    }
    bv.resize(newSize);
  }


  /*!
   * @brief Variant of RankVec::resize: If 'newSize > capacity_', it just returns false. Otherwise resize and return true.
   */
  bool resizeWithoutReserve
  (
   const size_t newSize
   ) noexcept {
    assert(newSize <= ctcbits::UINTW_MAX(58));

    return bv.resizeWithoutReserve(newSize);
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
      memutil::realloc_AbortOnFail<uint64_t>(blockM_, lenM);
      memutil::realloc_AbortOnFail<uint64_t>(blockM_, lenT);
    } else {
      memutil::safefree(blockM_);
      memutil::safefree(blockT_);
    }
  }
};

#endif

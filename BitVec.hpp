/*!
 * Copyright (c) 2017 Tomohiro I
 *
 * This program is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
/*!
 * @file BitVec.hpp
 * @brief Variable length bit vector wrapping functions in BitsUtil.
 * @author Tomohiro I
 * @date 2017-08-09
 */
#ifndef INCLUDE_GUARD_BitVec
#define INCLUDE_GUARD_BitVec

#include <stdint.h> // include uint64_t etc.
#include <assert.h>

#include <iostream>
#include <iterator>
#include <algorithm>

#include "WBitsVec.hpp"
#include "BitsUtil.hpp"
#include "MemUtil.hpp"

/*!
 * @brief Variable length bit vector wrapping functions in BitsUtil.
 * @attention
 *   For technical reason, capacity is limited to '2^58 - 1' due to compatibility with WBitsVec.
 */
class BitVec
{
  uint64_t * array_; //!< Array to store bits.
  size_t capacity_; //!< Current capacity (must be in [0, 2^58)).
  size_t size_; //!< Current size (must be in [0, capacity_]).

public:
  using iterator = WBitsVecIterator;

public:
  /*!
   * @brief Constructor.
   */
  BitVec
  (
   size_t capacity = 0 //!< Initial capacity.
   ) : array_(nullptr), capacity_(0), size_(0) {
    assert(capacity <= ctcbits::UINTW_MAX(58));

    reserve(capacity);
  }


  ~BitVec()
  {
    free(array_);
  }


  //// Copy
  /*!
   * @brief Copy constructor.
   * @attention
   *   Since the contents of 'other' are copied, it may take time when other.size_ is large.
   */
  BitVec
  (
   const BitVec & other
   ) : array_(nullptr), capacity_(0), size_(other.size_) {
    reserve(other.capacity_);
    if (size_ > 0) {
      bits::mvBits(other.array_, 0, array_, 0, size());
    }
  }


  /*!
   * @brief Assignment operator.
   * @attention
   *   If 'lhs' != 'rhs'
   *   - Since the contents of 'rhs' are copied, it may take time when other.size_ is large.
   *   - The contents of 'lhs' are freed.
   */
  BitVec& operator=
  (
   const BitVec & other
   ) {
    if (this != &other) {
      clear(); shrink_to_fit(); // clear() & shrink_to_fit() free array_
      reserve(other.capacity_);
      resize(other.size_);
      if (size_ > 0) {
        bits::mvBits(other.array_, 0, array_, 0, size_);
      }
    }
    return *this;
  }


  //// Move
  /*!
   * @brief Move constructor.
   * @attention
   *   'other' is initialized to an object with capacity = 0.
   */
  BitVec
  (
   BitVec && other
   ) : array_(other.array_), capacity_(other.capacity_), size_(other.size_) {
    other.array_ = nullptr;
    other.size_ = other.capacity_ = 0;
  }


  /*!
   * @brief Move operator.
   * @attention
   *   If 'lhs' != 'rhs'
   *   - The original contents of 'lhs' are freed.
   *   - 'rhs' is initialized to an object with capacity = 0.
   */
  BitVec operator=
  (
   BitVec && other
   ) {
    if (this != &other) {
      clear(); shrink_to_fit(); // clear() & shrink_to_fit() free array_
      array_ = other.array_;
      capacity_ = other.capacity_;
      size_ = other.size_;
      other.array_ = nullptr;
      other.size_ = other.capacity_ = 0;
    }
    return *this;
  }


  /*!
   * @brief Get iterator at given idx.
   */
  BitVec::iterator getItrAt
  (
   size_t idx
   ) noexcept {
    return WBitsVecIterator(array_, idx, 1);
  }


  /*!
   * @brief Get iterator points to the first element.
   */
  BitVec::iterator begin() noexcept {
    return getItrAt(0);
  }


  /*!
   * @brief Get iterator points to the end (right next to the last element).
   */
  BitVec::iterator end() noexcept {
    return getItrAt(size_);
  }


  /*!
   * @brief Get read-only array pointer.
   */
  const uint64_t * getConstArrayPtr() const noexcept
  {
    return array_;
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

    return bits::readWBits(array_, bitPos, w, mask);
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

    return bits::readWBits_S(array_, bitPos, mask);
  }


  /*!
   * @brief Read bit at 'bitPos'.
   */
  uint64_t readBit
  (
   const uint64_t bitPos //!< in [0, capacity_).
   ) const noexcept {
    assert(bitPos < capacity_);

    return bits::readWBits_S(array_ + (bitPos >> 6), bitPos & 0x3f, ctcbits::UINTW_MAX(1));
  }


  /*!
   * @brief Write 'w'-bit value 'val' to the bit-region beginning at array_[[bitPos..]].
   * @pre The bit-region must not be out of bounds.
   */
  void writeWBits
  (
   const uint64_t val, //!< in [0, 2^w).
   const uint64_t bitPos, //!< Bit-pos.
   const uint8_t w, //!< Bit-width in [0, 64].
   const uint64_t mask //!< UINTW_MAX(w).
   ) {
    assert(w <= 64);
    assert(val == 0 || bits::bitSize(val) <= w);

    bits::writeWBits(val, array_, bitPos, w, mask);
  }


  /*!
   * @brief Simplified version of ::writeWBits that can be used when writing bits in a single word.
   */
  void writeWBits_S
  (
   const uint64_t val, //!< in [0, 2^w).
   const uint64_t bitPos, //!< Bit-pos.
   const uint64_t mask //!< UINTW_MAX(w).
   ) {
    assert(bits::bitSize(mask) + (bitPos & 0x3f) <= 64);
    assert(bits::bitSize(val) <= bits::bitSize(mask));

    bits::writeWBits_S(val, array_, bitPos, mask);
  }


  /*!
   * @brief Write a bit 'val' at 'bitPos'.
   */
  void writeBit
  (
   const bool val, //!< Bool value.
   const size_t bitPos //!< in [0, capacity_).
   ) {
    assert(bitPos < capacity_);

    bits::writeWBits_S(val, array_ + (bitPos >> 6), bitPos & 0x3f, ctcbits::UINTW_MAX(1));
  }


  /*!
   * @brief Get current capacity.
   */
  size_t capacity() const noexcept {
    return capacity_;
  }


  /*!
   * @brief Get current size.
   */
  size_t size() const noexcept {
    return size_;
  }


  /*!
   * @brief Calculate total memory usage in bytes.
   */
  size_t calcMemBytes() const noexcept {
    return sizeof(*this) + sizeof(uint64_t) * ((capacity_ + 63) / 64);
  }


  /*!
   * @brief Return if no element is stored.
   */
  bool empty() const noexcept {
    return (size_ == 0);
  }


  /*!
   * @brief Clear vector. It only changes size to zero.
   */
  void clear() noexcept {
    size_ = 0;
  }


  /*!
   * @brief Reserve enough space to store 'newCapacity_ * w_' bits.
   * @note
   *   This function does not shrink 'capacity_'
   *   If 'newCapacity > capacity_', expand 'array_' to store 'newCapacity * w_' bits.
   *   'size_' is unchanged.
  */
  void reserve
  (
   const size_t newCapacity
   ) {
    assert(newCapacity <= ctcbits::UINTW_MAX(58));

    if (newCapacity > capacity_) {
      capacity_ = newCapacity;
      const size_t len = (capacity_ + 63) / 64; // +63 for roundup
      memutil::realloc_AbortOnFail<uint64_t>(array_, len);
    }
  }


  /*!
   * @brief Resize 'size_' to 'newSize'. It expands 'array_' if needed.
   * @note
   *   This function does not shrink 'capacity_'
   *   If 'newSize > capacity_', expand 'array_' by calling BitVec::reserve.
  */
  void resize
  (
   const size_t newSize
   ) {
    assert(newSize <= ctcbits::UINTW_MAX(58));

    if (newSize > capacity_) {
      reserve(newSize);
    }
    size_ = newSize;
  }


  /*!
   * @brief Variant of BitVec::resize: If 'newSize > capacity_', it just returns false. Otherwise resize and return true.
   */
  bool resizeWithoutReserve
  (
   const size_t newSize
   ) noexcept {
    assert(newSize <= ctcbits::UINTW_MAX(58));

    if (newSize <= capacity_) {
      size_ = newSize;
      return true;
    }
    return false;
  }


  /*!
   * @brief Shrink vector to fit current bit-length in use.
   */
  void shrink_to_fit() {
    if (size_ > 0) {
      const size_t newLen = (size_ + 63) / 64; // +63 for roundup
      memutil::realloc_AbortOnFail<uint64_t>(array_, newLen);
    } else {
      memutil::safefree(array_);
    }
    capacity_ = size_;
  }


  void printDebugInfo() const noexcept {
    auto size = this->size();
    for (uint64_t i = 0; i < (size + 63) / 64; ++i) {
      for (uint64_t j = 0; j < 64; ++j) {
        std::cout << readBit(64 * i + 63 - j);
      }
      std::cout << " ";
    }
    std::cout << std::endl;
  }
};

#endif

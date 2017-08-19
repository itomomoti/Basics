/*!
 * Copyright (c) 2017 Tomohiro I
 *
 * This program is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
/*!
 * @file BitVec.hpp
 * @brief Bit vector and its iterator. Some implementations are simplified compared to BitsWVec with w = 1.
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

class BitVec;

/*!
 * @brief Variable length bit vector with functions in BitsUtil.
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
   *   'other' is initialized to empty BitVec object.
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
   *   - 'rhs' is initialized to empty BitVec object.
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
   * @brief Read value at 'idx'.
   * @note We do not provide access by [] operator.
   */
  uint64_t read
  (
   const size_t idx //!< in [0, capacity_).
   ) const {
    assert(idx < capacity_);

    return bits::readWBitsInWord(array_ + (idx >> 6), idx & 0x3f, ctcbits::UINTW_MAX(1));
  }


  /*!
   * @brief Write 'val' at 'idx'.
   * @note We do not provide access by [] operator.
   */
  void write
  (
   const bool val, //!< in [0, 1].
   const size_t idx //!< in [0, capacity_).
   ) {
    assert(idx < capacity_);

    bits::writeWBitsInWord(val, array_ + (idx >> 6), idx & 0x3f, ctcbits::UINTW_MAX(1));
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
};

#endif

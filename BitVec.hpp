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

#include "Uncopyable.hpp"
#include "BitsUtil.hpp"
#include "MemUtil.hpp"

class BitVec;

/*!
 * @brief Iterator for BitVec.
 * @attention
 *   An iterator is invalidated without notification when BitVec object is destroyed or reallocated.
 *   Using invalidated iterator would cause problems.
 * @note
 *   Unfortunately bit vector is only qualified to be input_iterator due to the current design of STL.
 *   (see e.g. http://www.boost.org/doc/libs/1_63_0/libs/iterator/doc/index.html).
 */
class BitVecIterator :
  public std::iterator<std::input_iterator_tag, uint64_t>
{
  uint64_t * const array_;
  uint64_t pos_;

public:
  /*!
   * @brief Public constructor.
   */
  BitVecIterator(uint64_t * array, uint64_t pos) noexcept
    : array_(array), pos_(pos)
  {}
  //// copy
  BitVecIterator(const BitVecIterator & itr) noexcept = default;
  BitVecIterator& operator=(const BitVecIterator & itr) noexcept = default;
  //// move
  BitVecIterator(BitVecIterator && itr) noexcept = default;
  BitVecIterator& operator=(BitVecIterator && itr) noexcept = default;


  /*!
   * @brief Read value at the position pointed by iterator.
   */
  uint64_t read() const {
    return bits::readWBitsInWord(array_ + (pos_ >> 6), pos_ & 0x3f, ctcbits::UINTW_MAX(1));
  }


  /*!
   * @brief Write 'val' at the position pointed by iterator.
   */
  void write
  (
   const uint64_t val
   ) {
    bits::writeWBitsInWord(val, array_ + (pos_ >> 6), pos_ & 0x3f, ctcbits::UINTW_MAX(1));
  }


  ////// operator
  //// *itr
  uint64_t operator*() {
    return this->read();
  }

  //// ++itr
  BitVecIterator & operator++() {
    pos_ += 1;
    return *this;
  }

  //// itr++
  BitVecIterator operator++(int) {
    BitVecIterator tmp(*this);
    ++(*this);
    return tmp;
  }

  //// --itr
  BitVecIterator & operator--() {
    pos_ -= 1;
    return *this;
  }

  //// itr--
  BitVecIterator operator--(int) {
    BitVecIterator tmp(*this);
    --(*this);
    return tmp;
  }

  //// *this != itr
  bool operator!=(const BitVecIterator & itr) {
    return (pos_ != itr.pos_ || array_ != itr.array_);
  }

  //// *this == itr
  bool operator==(const BitVecIterator & itr) {
    return !(*this != itr);
  }

  ////// add more operators
  //// itr += diff
  BitVecIterator & operator+=(const difference_type diff) noexcept {
    pos_ += diff;
    return *this;
  }

  //// itr -= diff
  BitVecIterator & operator-=(const difference_type diff) noexcept {
    *this += (-1 * diff);
    return *this;
  }

  //// itr + diff
  friend BitVecIterator operator+(const BitVecIterator & itr, const difference_type diff) noexcept {
    const int64_t pos = itr.pos_ + diff;
    return BitVecIterator(itr.array_, pos);
  }

  //// diff + itr
  friend BitVecIterator operator+(const difference_type diff, const BitVecIterator & itr) noexcept {
    return itr + diff;
  }

  //// itr - diff
  friend BitVecIterator operator-(const BitVecIterator & itr, const difference_type diff) noexcept {
    return itr + (-1 * diff);
  }

  //// lhs - rhs
  friend difference_type operator-(const BitVecIterator & lhs, const BitVecIterator & rhs) noexcept {
    return static_cast<int64_t>(lhs.pos_) - static_cast<int64_t>(rhs.pos_);
  }

  //// lhs < rhs
  friend bool operator<(const BitVecIterator & lhs, const BitVecIterator & rhs) noexcept {
    return (lhs.pos_ < rhs.pos_ || lhs.array_ < rhs.array_);
  }

  //// rhs < lhs
  friend bool operator>(const BitVecIterator & lhs, const BitVecIterator & rhs) noexcept {
    return (rhs < lhs);
  }

  //// lhs <= rhs
  friend bool operator<=(const BitVecIterator & lhs, const BitVecIterator & rhs) noexcept {
    return !(lhs > rhs);
  }

  //// lhs >= rhs
  friend bool operator>=(const BitVecIterator & lhs, const BitVecIterator & rhs) noexcept {
    return !(lhs < rhs);
  }


  /*!
   * @brief Move values from src-region to tgt-region with same bit-width.
   * @pre The bit-width of src and tgt should be same.
   */
  friend void mvBA_SameW
  (
   BitVecIterator & src, //!< Iterator specifying the beginning position of src.
   BitVecIterator & tgt, //!< Iterator specifying the beginning position of tgt.
   const uint64_t num //!< Number of elements to move.
   ) {
    bits::mvBits(src.array_ + (src.pos_ >> 6), src.pos_ & 0x3f, tgt.array_ + (tgt.pos_ >> 6), tgt.pos_ & 0x3f, num);
  }


  /*!
   * @brief Move values from src-region to tgt-region.
   * @attention When src-region and tgt-region overlap, the overlapped part of src-region is overwritten.
   *            The other part of src-region is not changed.
   * @note
   *   The bit-width of src and tgt can be different.
   */
  friend void mvBA
  (
   BitVecIterator & src, //!< Iterator specifying the beginning position of src.
   BitVecIterator & tgt, //!< Iterator specifying the beginning position of tgt.
   const uint64_t num //!< Number of elements to move.
   ) {
    mvBA_SameW(src, tgt, num);
  }


  friend void mvBA_SameW(BitVecIterator && src, BitVecIterator && tgt, const uint64_t num) {
    bits::mvBits(src.array_ + (src.pos_ >> 6), src.pos_ & 0x3f, tgt.array_ + (tgt.pos_ >> 6), tgt.pos_ & 0x3f, num);
  }


  friend void mvBA(BitVecIterator && src, BitVecIterator && tgt, const uint64_t num) {
    mvBA_SameW(src, tgt, num);
  }
};



/*!
 * @brief Bit vector.
 * @attention
 *   For technical reason, capacity is limited to '2^58 - 1' due to compatibility with WBitsVec.
 * @attention
 *   We prohibit to copy an object of BitVec. Use smart pointer to distribute an objcet.
 */
class BitVec
  : Uncopyable
{
  uint64_t * array_; //!< Array to store values.
  size_t capacity_; //!< Current capacity (must be in [0, 2^58)).
  size_t size_; //!< Current size (must be in [0, capacity_]).

public:
  using iterator = BitVecIterator;

public:
  BitVec
  (
   size_t capacity = 0 //!< Initial capacity.
   ) : array_(NULL), capacity_(capacity), size_(0) {
    assert(capacity_ <= ctcbits::UINTW_MAX(58));

    if (capacity > 0) {
      const size_t len = (capacity_ + 63) / 64; // +63 for roundup
      array_ = memutil::malloc_AbortOnFail<uint64_t>(len);
    }
  }


  ~BitVec()
  {
    free(array_);
  }


  /*!
   * @brief Get iterator at given idx.
   */
  BitVec::iterator getItrAt
  (
   size_t idx
   ) noexcept {
    return BitVecIterator(array_, idx);
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

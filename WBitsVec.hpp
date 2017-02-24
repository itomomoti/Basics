/*!
 * Copyright (c) 2017 Tomohiro I
 *
 * This program is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
/*!
 * @file WBitsVec.hpp
 * @brief W-bits packed vector and its iterator.
 * @author Tomohiro I
 * @date 2017-01-24
 */
#ifndef INCLUDE_GUARD_WBitsVec
#define INCLUDE_GUARD_WBitsVec

#include <stdint.h> // include uint64_t etc.
#include <assert.h>

#include <iostream>
#include <iterator>
#include <algorithm>

#include "Uncopyable.hpp"
#include "BitsUtil.hpp"
#include "MemUtil.hpp"

class WBitsVec;

/*!
 * @brief Iterator for WBitsVec.
 * @note
 * Unfortunately packed vector is only qualified to be input_iterator due to the current design of STL.
 * (see e.g. http://www.boost.org/doc/libs/1_63_0/libs/iterator/doc/index.html).
 */
class WBitsVecIterator :
  public std::iterator<std::input_iterator_tag, uint64_t>
{
  friend WBitsVec; // To use private constructors of WBitsVecIterator in WBitsVec.

  uint64_t * const array_;
  uint64_t pos_;
  const uint8_t w_;

private:
  /*!
   * @brief Private constructor called by WBitsVec.
   */
  WBitsVecIterator (uint64_t * array, uint64_t pos, uint8_t w) noexcept
    : array_(array), pos_(pos), w_(w)
  {}

public:
  //// copy
  WBitsVecIterator(const WBitsVecIterator & itr) noexcept = default;
  WBitsVecIterator& operator=(const WBitsVecIterator & itr) noexcept = default;
  //// move
  WBitsVecIterator(WBitsVecIterator && itr) noexcept = default;
  WBitsVecIterator& operator=(WBitsVecIterator && itr) noexcept = default;


  /*!
   * @brief Read value at the position pointed by iterator.
   */
  uint64_t read() const {
    return bits::readWBits(array_, pos_, w_, bits::UINTW_MAX(w_));
  }


  /*!
   * @brief Write 'val' at the position pointed by iterator.
   */
  void write
  (
   const uint64_t val
   ) {
    bits::writeWBits(val, array_, pos_, w_, bits::UINTW_MAX(w_));
  }


  ////// operator
  //// *itr
  uint64_t operator*() {
    return this->read();
  }

  //// ++itr
  WBitsVecIterator & operator++() {
    pos_ += w_;
    return *this;
  }

  //// itr++
  WBitsVecIterator operator++(int) {
    WBitsVecIterator tmp(*this);
    ++(*this);
    return tmp;
  }

  //// --itr
  WBitsVecIterator & operator--() {
    pos_ -= w_;
    return *this;
  }

  //// itr--
  WBitsVecIterator operator--(int) {
    WBitsVecIterator tmp(*this);
    --(*this);
    return tmp;
  }

  //// *this != itr
  bool operator!=(const WBitsVecIterator & itr) {
    return (pos_ != itr.pos_ || array_ != itr.array_);
  }

  //// *this == itr
  bool operator==(const WBitsVecIterator & itr) {
    return !(*this != itr);
  }

  ////// add more operators
  //// itr += diff
  WBitsVecIterator & operator+=(const difference_type diff) noexcept {
    pos_ += static_cast<int64_t>(w_) * diff;
    return *this;
  }

  //// itr -= diff
  WBitsVecIterator & operator-=(const difference_type diff) noexcept {
    *this += (-1 * diff);
    return *this;
  }

  //// itr + diff
  friend WBitsVecIterator operator+(const WBitsVecIterator & itr, const difference_type diff) noexcept {
    const int64_t pos = itr.pos_ + static_cast<int64_t>(itr.w_) * diff;
    return WBitsVecIterator(itr.array_, pos, itr.w_);
  }

  //// diff + itr
  friend WBitsVecIterator operator+(const difference_type diff, const WBitsVecIterator & itr) noexcept {
    return itr + diff;
  }

  //// itr - diff
  friend WBitsVecIterator operator-(const WBitsVecIterator & itr, const difference_type diff) noexcept {
    return itr + (-1 * diff);
  }

  //// lhs - rhs
  friend difference_type operator-(const WBitsVecIterator & lhs, const WBitsVecIterator & rhs) noexcept {
    assert(lhs.w_ == rhs.w_);
    return (static_cast<int64_t>(lhs.pos_) - static_cast<int64_t>(rhs.pos_)) / lhs.w_;
  }

  //// lhs < rhs
  friend bool operator<(const WBitsVecIterator & lhs, const WBitsVecIterator & rhs) noexcept {
    return (lhs.pos_ < rhs.pos_ || lhs.array_ < rhs.array_);
  }

  //// rhs < lhs
  friend bool operator>(const WBitsVecIterator & lhs, const WBitsVecIterator & rhs) noexcept {
    return (rhs < lhs);
  }

  //// lhs <= rhs
  friend bool operator<=(const WBitsVecIterator & lhs, const WBitsVecIterator & rhs) noexcept {
    return !(lhs > rhs);
  }

  //// lhs >= rhs
  friend bool operator>=(const WBitsVecIterator & lhs, const WBitsVecIterator & rhs) noexcept {
    return !(lhs < rhs);
  }


  /*!
   * @brief Move values from src-region to tgt-region with same bit-width.
   * @pre The bit-width of src and tgt should be same.
   */
  friend void mvWBA_SameW
  (
   WBitsVecIterator & src, //!< Iterator specifying the beginning position of src.
   WBitsVecIterator & tgt, //!< Iterator specifying the beginning position of tgt.
   const uint64_t num //!< Number of elements to move.
   ) {
    assert(src.w_ == tgt.w_);
    bits::mvBits(src.array_ + (src.pos_ >> 6), src.pos_ & 0x3f, tgt.array_ + (tgt.pos_ >> 6), tgt.pos_ & 0x3f, num * src.w_);
  }


  /*!
   * @brief Move values from src-region to tgt-region with different bit-width.
   */
  friend void mvWBA_DiffW
  (
   WBitsVecIterator & src, //!< Iterator specifying the beginning position of src.
   WBitsVecIterator & tgt, //!< Iterator specifying the beginning position of tgt.
   const uint64_t num //!< Number of elements to move.
   ) {
    for (uint64_t i = 0; i < num; ++i, ++src, ++tgt) {
      assert(src.read() <= bits::UINTW_MAX(tgt.w_));
      tgt.write(src.read());
    }
  }


  /*!
   * @brief Move values from src-region to tgt-region.
   * @attention When src-region and tgt-region overlap, the overlapped part of src-region is overwritten.
   *            The other part of src-region is not changed.
   * @note
   * The bit-width of src and tgt can be different.
   */
  friend void mvWBA
  (
   WBitsVecIterator & src, //!< Iterator specifying the beginning position of src.
   WBitsVecIterator & tgt, //!< Iterator specifying the beginning position of tgt.
   const uint64_t num //!< Number of elements to move.
   ) {
    if (src.w_ == tgt.w_) {
      mvWBA_SameW(src, tgt, num);
    } else {
      mvWBA_DiffW(src, tgt, num);
    }
  }


  friend void mvWBA_SameW(WBitsVecIterator && src, WBitsVecIterator && tgt, const uint64_t num) {
    assert(src.w_ == tgt.w_);

    bits::mvBits(src.array_ + (src.pos_ >> 6), src.pos_ & 0x3f, tgt.array_ + (tgt.pos_ >> 6), tgt.pos_ & 0x3f, num * src.w_);
  }


  friend void mvWBA_DiffW(WBitsVecIterator && src, WBitsVecIterator && tgt, const uint64_t num) {
    for (uint64_t i = 0; i < num; ++i, ++src, ++tgt) {
      assert(src.read() <= bits::UINTW_MAX(tgt.w_));
      tgt.write(src.read());
    }
  }


  friend void mvWBA(WBitsVecIterator && src, WBitsVecIterator && tgt, const uint64_t num) {
    if (src.w_ == tgt.w_) {
      mvWBA_SameW(src, tgt, num);
    } else {
      mvWBA_DiffW(src, tgt, num);
    }
  }
};



/*!
 * @brief W-bits packed vector. Bit-width 'w' and capacity can be changed dynamically.
 * @note
 * For technical reason, capacity is limited to '2^58 - 1' so that 'capacity * w' does not overflow.
 */
class WBitsVec
  : Uncopyable
{
  uint64_t * array_; //!< Array to store values.
  size_t capacity_; //!< Current capacity (must be in [0, 2^58)).
  size_t size_; //!< Current size (must be in [0, capacity_]).
  uint8_t w_; //!< Current bit-width (must be in [1, 64]).

public:
  using iterator = WBitsVecIterator;

public:
  WBitsVec
  (
   uint8_t w = 1, //!< Initial bit-width.
   size_t capacity = 0 //!< Initial capacity.
   ) : array_(NULL), capacity_(capacity), size_(0), w_(w) {
    assert(capacity_ <= bits::UINTW_MAX(58));
    assert(0 < w_ && w_ <= 64);

    if (capacity > 0) {
      const size_t len = (capacity_ * w_ + 63) / 64; // +63 for roundup
      array_ = memutil::mymalloc<uint64_t>(len);
    }
  }


  ~WBitsVec()
  {
    free(array_);
  }


  /*!
   * @brief Get iterator at given idx.
   */
  WBitsVec::iterator getItrAt
  (
   size_t idx
   ) noexcept {
    return WBitsVecIterator(array_, idx * w_, w_);
  }


  /*!
   * @brief Get iterator points to the first element.
   */
  WBitsVec::iterator begin() noexcept {
    return getItrAt(0);
  }


  /*!
   * @brief Get iterator points to the end (right next to the last element).
   */
  WBitsVec::iterator end() noexcept {
    return getItrAt(size_);
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
    
    return bits::readWBits(array_, idx * w_, w_, bits::UINTW_MAX(w_));
  }


  /*!
   * @brief Write 'val' at 'idx'.
   * @note We do not provide access by [] operator.
   */
  void write
  (
   const uint64_t val, //!< in [0, 2^w_).
   const size_t idx //!< in [0, capacity_).
   ) {
    assert(idx < capacity_);
    assert(val <= bits::UINTW_MAX(w_));

    bits::writeWBits(val, array_, idx * w_, w_, bits::UINTW_MAX(w_));
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
    return sizeof(*this) + sizeof(uint64_t) * ((capacity_ * w_ + 63) / 64);
  }


  /*!
   * @brief Return if no element is stored.
   */
  bool empty() const noexcept {
    return (size_ == 0);
  }


  /*!
   * @brief Clear vector. It only changes 'size_' to zero.
   */
  void clear() noexcept {
    size_ = 0;
  }


  /*!
   * @brief Reserve enough space to store 'newCapacity_ * w_' bits.
   * @note
   * This function does not shrink 'capacity_'
   * If 'newCapacity > capacity_', expand 'array_' to store 'newCapacity * w_' bits.
   * 'size_' is unchanged.
  */
  void reserve
  (
   const size_t newCapacity
   ) {
    assert(newCapacity <= bits::UINTW_MAX(58));

    if (newCapacity > capacity_) {
      capacity_ = newCapacity;
      const size_t len = (capacity_ * w_ + 63) / 64; // +63 for roundup
      memutil::myrealloc<uint64_t>(array_, len);
    }
  }


  /*!
   * @brief Resize 'size_' to 'newSize'. It expands 'array_' if needed.
   * @note
   * This function does not shrink 'capacity_'
   * If 'newSize > capacity_', expand 'array_' by calling WBitsVec::reserve.
  */
  void resize
  (
   const size_t newSize
   ) {
    assert(newSize <= bits::UINTW_MAX(58));

    if (newSize > capacity_) {
      reserve(newSize);
    }
    size_ = newSize;
  }


  /*!
   * @brief Variant of ::resize: If 'newSize > capacity_', it just returns false. Otherwise resize and return true.
   */
  bool resizeWithoutReserve
  (
   const size_t newSize
   ) noexcept {
    assert(newSize <= bits::UINTW_MAX(58));

    if (newSize <= capacity_) {
      size_ = newSize;
      return true;
    }
    return false;
  }


  /*!
   * @brief Shrink vector to fit current bit-length.
   */
  void shrink_to_fit() {
    if (size_ > 0) {
      const size_t newLen = (size_ * w_ + 63) / 64; // +63 for roundup
      memutil::myrealloc<uint64_t>(array_, newLen);
    } else {
      memutil::myfree(array_);
    }
    capacity_ = size_;
  }


  /*!
   * @brief Change w_ and/or capacity_.
   * @note
   * This function will do the following two tasks:
   * 1. convert values of w_ bits to those of w bits, and renew w_.
   *    NOTE: If w < w_, the w_ - w most significant bits for each value will be discarded.
   * 2. realloc 'array_' if needed (or we can give 'newCapacity' to expand 'capacity_' explicitly).
   *    NOTE: 'array_' will never shrink.
   *
   * @note
   * TIPS: We do not provide a function to change only w_ (i.e., the setter for w_)
   *       because most of the case changing w_ would be accompanied by conversion (and reallocation if needed).
   *       If you want to change w_ but do not want to convert nor realloc, do the followings:
   *       WBAObj.clear();
   *       WBAObj.convert(w);
   */
  void convert(const uint8_t w, size_t newCapacity = 0) {
    newCapacity = std::max(capacity_, newCapacity);
    const size_t oldLen = (capacity_ * w_ + 63) / 64; // +63 for roundup
    const size_t newLen = (newCapacity * w + 63) / 64; // +63 for roundup
    capacity_ = newCapacity;
    if (newLen > oldLen) {
      memutil::myrealloc<uint64_t>(array_, newLen);
    }
    if (w == w_) {
      return; // do nothing
    }
    // convert the values in array
    if (w > w_) {
      for (uint64_t i = this->size() - 1; i != UINT64_MAX; --i) {
        bits::writeWBits(this->read(i), array_, i * w, w, bits::UINTW_MAX(w));
      }
    } else { // w < w_ (w_ - w most significant bits for each value will be discarded)
      for (uint64_t i = 0; i < this->size(); ++i) {
        bits::writeWBits(this->read(i) & bits::UINTW_MAX(w), array_, i * w, w, bits::UINTW_MAX(w));
      }
    }
    w_ = w; // set w_
  }


  /*!
   * @brief Get current bit-width of each element.
   */
  uint8_t getW() const noexcept {
    return w_;
  }
};

#endif

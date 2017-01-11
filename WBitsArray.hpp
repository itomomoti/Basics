#ifndef INCLUDE_GUARD_WBitsArray
#define INCLUDE_GUARD_WBitsArray

#include <stdint.h> // include uint64_t etc.
#include <assert.h>

#include <iostream>
#include <iterator>
#include <algorithm>

#include "Uncopyable.hpp"
#include "BitsUtil.hpp"

#define mymalloc(p,n,t) {p = (t *)malloc((n)*sizeof(*p)); if ((p)==NULL) {printf("not enough memory at line %d\n",__LINE__); exit(1);};}
#define myrealloc(p,n,t) {p = (t *)realloc((p),(n)*sizeof(*p)); if ((p)==NULL) {printf("not enough memory at line %d\n",__LINE__); exit(1);};}

class WBitsArray;

class WBitsArrayIterator :
  public std::iterator<std::input_iterator_tag, uint64_t>
{
  friend WBitsArray; // To use private constructors of WBitsArrayIterator in WBitsArray

  uint64_t * const array_;
  uint64_t pos_;
  const uint8_t w_;

private:
  //// Called by WBitsArray
  WBitsArrayIterator (uint64_t * array, uint64_t pos, uint8_t w) noexcept
    : array_(array), pos_(pos), w_(w)
  {}

public:
  //// copy
  WBitsArrayIterator(const WBitsArrayIterator & itr) noexcept = default;
  WBitsArrayIterator& operator=(const WBitsArrayIterator & itr) noexcept = default;
  //// move
  WBitsArrayIterator(WBitsArrayIterator && itr) noexcept = default;
  WBitsArrayIterator& operator=(WBitsArrayIterator && itr) noexcept = default;

  uint64_t read() const {
    return bits::readWBits(array_, pos_, w_, bits::UINTW_MAX(w_));
  }

  void write(const uint64_t val) {
    bits::writeWBits(val, array_, pos_, w_, bits::UINTW_MAX(w_));
  }

  ////// operator
  //// *itr
  uint64_t operator*() {
    return this->read();
  }

  //// ++itr
  WBitsArrayIterator & operator++() {
    pos_ += w_;
    return *this;
  }

  //// itr++
  WBitsArrayIterator operator++(int) {
    WBitsArrayIterator tmp(*this);
    ++(*this);
    return tmp;
  }

  //// --itr
  WBitsArrayIterator & operator--() {
    pos_ -= w_;
    return *this;
  }

  //// itr--
  WBitsArrayIterator operator--(int) {
    WBitsArrayIterator tmp(*this);
    --(*this);
    return tmp;
  }

  //// *this != itr
  bool operator!=(const WBitsArrayIterator & itr) {
    return (pos_ != itr.pos_ || array_ != itr.array_);
  }

  //// *this == itr
  bool operator==(const WBitsArrayIterator & itr) {
    return !(*this != itr);
  }

  ////// add more operators
  //// itr += diff
  WBitsArrayIterator & operator+=(const difference_type diff) noexcept {
    pos_ += static_cast<int64_t>(w_) * diff;
    return *this;
  }

  //// itr -= diff
  WBitsArrayIterator & operator-=(const difference_type diff) noexcept {
    *this += (-1 * diff);
    return *this;
  }

  //// itr + diff
  friend WBitsArrayIterator operator+(const WBitsArrayIterator & itr, const difference_type diff) noexcept {
    const int64_t pos = itr.pos_ + static_cast<int64_t>(itr.w_) * diff;
    return WBitsArrayIterator(itr.array_, pos, itr.w_);
  }

  //// diff + itr
  friend WBitsArrayIterator operator+(const difference_type diff, const WBitsArrayIterator & itr) noexcept {
    return itr + diff;
  }

  //// itr - diff
  friend WBitsArrayIterator operator-(const WBitsArrayIterator & itr, const difference_type diff) noexcept {
    return itr + (-1 * diff);
  }

  //// lhs - rhs
  friend difference_type operator-(const WBitsArrayIterator & lhs, const WBitsArrayIterator & rhs) noexcept {
    assert(lhs.w_ == rhs.w_);
    return (static_cast<int64_t>(lhs.pos_) - static_cast<int64_t>(rhs.pos_)) / lhs.w_;
  }

  //// lhs < rhs
  friend bool operator<(const WBitsArrayIterator & lhs, const WBitsArrayIterator & rhs) noexcept {
    return (lhs.pos_ < rhs.pos_ || lhs.array_ < rhs.array_);
  }

  //// rhs < lhs
  friend bool operator>(const WBitsArrayIterator & lhs, const WBitsArrayIterator & rhs) noexcept {
    return (rhs < lhs);
  }

  //// lhs <= rhs
  friend bool operator<=(const WBitsArrayIterator & lhs, const WBitsArrayIterator & rhs) noexcept {
    return !(lhs > rhs);
  }

  //// lhs >= rhs
  friend bool operator>=(const WBitsArrayIterator & lhs, const WBitsArrayIterator & rhs) noexcept {
    return !(lhs < rhs);
  }

  /**
     NOTE: src and tgt regions can overlap. In that case, src region might lose its original values.
   */
  //// mvWBA_SameW
  friend void mvWBA_SameW(WBitsArrayIterator & src, WBitsArrayIterator & tgt, const uint64_t num) {
    assert(src.w_ == tgt.w_);
    bits::copyBits(src.array_ + (src.pos_ >> 6), src.pos_ & 0x3f, tgt.array_ + (tgt.pos_ >> 6), tgt.pos_ & 0x3f, num * src.w_);
  }

  //// mvWBA_DiffW
  friend void mvWBA_DiffW(WBitsArrayIterator & src, WBitsArrayIterator & tgt, const uint64_t num) {
    for (uint64_t i = 0; i < num; ++i, ++src, ++tgt) {
      assert(src.read() <= bits::UINTW_MAX(tgt.w_));
      tgt.write(src.read());
    }
  }

  //// copy values
  friend void mvWBA(WBitsArrayIterator & src, WBitsArrayIterator & tgt, const uint64_t num) {
    if (src.w_ == tgt.w_) {
      mvWBA_SameW(src, tgt, num);
    } else {
      mvWBA_DiffW(src, tgt, num);
    }
  }

  //// copy_SameW
  friend void mvWBA_SameW(WBitsArrayIterator && src, WBitsArrayIterator && tgt, const uint64_t num) {
    assert(src.w_ == tgt.w_);
    bits::copyBits(src.array_ + (src.pos_ >> 6), src.pos_ & 0x3f, tgt.array_ + (tgt.pos_ >> 6), tgt.pos_ & 0x3f, num * src.w_);
  }

  //// copy_DistW
  friend void mvWBA_DistW(WBitsArrayIterator && src, WBitsArrayIterator && tgt, const uint64_t num) {
    for (uint64_t i = 0; i < num; ++i, ++src, ++tgt) {
      assert(src.read() <= bits::UINTW_MAX(tgt.w_));
      tgt.write(src.read());
    }
  }

  //// copy values
  friend void mvWBA(WBitsArrayIterator && src, WBitsArrayIterator && tgt, const uint64_t num) {
    if (src.w_ == tgt.w_) {
      mvWBA_SameW(src, tgt, num);
    } else {
      mvWBA_DiffW(src, tgt, num);
    }
  }
};




class WBitsArray
  : Uncopyable
{
  uint64_t * array_;
  uint8_t w_;
  size_t capacity_;
  size_t size_;

	static const int8_t realloc_param_ = 2; // realloc size is "realloc_param_" times larger than the current size

public:
  using iterator = WBitsArrayIterator;

public:
  WBitsArray(uint8_t w = 1, size_t capacity = 0) : array_(NULL), w_(w), capacity_(capacity), size_(0)
  {
    assert(capacity_ <= bits::UINTW_MAX(58));
    assert(w_ <= 64);
    const size_t len = capacity_ * w_ / 64 + 2; // +1 for roundup, and another +1 for margin
    mymalloc(array_, len, uint64_t);
  }


  ~WBitsArray()
  {
    free(array_);
  }


  //// get iterator
  WBitsArray::iterator getItrAt(size_t idx) noexcept {
    return WBitsArrayIterator(array_, idx * w_, w_);
  }


  WBitsArray::iterator begin() noexcept {
    return getItrAt(0);
  }


  WBitsArray::iterator end() noexcept {
    return getItrAt(size_);
  }


  /** read/write
      We do not provide access with 'wbArrayObj[i]'
   */
  uint64_t read(const size_t idx) const
  {
    assert(idx <= capacity_);
    return bits::readWBits(array_, idx * w_, w_, bits::UINTW_MAX(w_));
  }


  void write(const uint64_t val, const size_t idx)
  {
    assert(idx <= capacity_);
    assert(val <= bits::UINTW_MAX(w_));
    bits::writeWBits(val, array_, idx * w_, w_, bits::UINTW_MAX(w_));
  }


  // 
  size_t capacity() const noexcept {
    return capacity_;
  }


  size_t size() const noexcept {
    return size_;
  }


  size_t calcSpace() const noexcept {
    return sizeof(this) + (capacity_ * w_ / 8) + 2 * sizeof(uint64_t);
  }


  bool empty() const noexcept {
    return (size_ == 0);
  }


  void clear() noexcept {
    size_ = 0;
  }


  /**
     reserve does not shrink 'capacity_'
     If 'newCapacity' > 'capacity_', expand 'array_' to store ('newCapacity' * 'w_') bits.
     'size_' is unchanged.
  */
  void reserve(const size_t newCapacity) {
    assert(newCapacity <= bits::UINTW_MAX(58));
    if (newCapacity > capacity_) {
      capacity_ = newCapacity;
      const size_t len = capacity_ * w_ / 64 + 2; // +1 for roundup, and another +1 for margin
      myrealloc(array_, len, uint64_t);
    }
  }


  void resize(const size_t newSize) {
    assert(newSize <= bits::UINTW_MAX(58));
    if (newSize > capacity_) {
      reserve(newSize);
    }
    size_ = newSize;
  }


  /**
     'convert' will do the following two tasks:
     (1) convert values of w_ bits to those of w bits, and renew w_.
         NOTE: If w < w_, the w_ - w significant bits for each value will be discarded.
     (2) realloc 'array_' if needed (or we can give 'newCapacity' to expand 'capacity_' explicitly).
         NOTE: 'array_' will never shrink.
     TIPS: We do not provide a function to change only w_ (i.e., the setter for w_)
           because most of the case changing w_ would be accompanied by conversion (and reallocation if needed).
           If you want to change w_ but do not want to convert nor realloc, do the followings:
           wbArrayObj.clear();
           wbArrayObj.convert(w);
  */
  void convert(const uint8_t w, size_t newCapacity = 0) {
    newCapacity = std::max(capacity_, newCapacity);
    const size_t oldLen = capacity_ * w_ / 64 + 2; // +1 for roundup, and another +1 for margin
    const size_t newLen = newCapacity * w / 64 + 2; // +1 for roundup, and another +1 for margin
    capacity_ = newCapacity;
    if (newLen > oldLen) {
      myrealloc(array_, newLen, uint64_t);
    }
    if (w == w_) {
      return; // do nothing
    }
    // convert the values in array
    if (w > w_) {
      for (uint64_t i = this->size() - 1; i != UINT64_MAX; --i) {
        bits::writeWBits(this->read(i), array_, i * w, w, bits::UINTW_MAX(w));
      }
    } else { // w < w_ (w_ - w significant bits for each value will be discarded)
      for (uint64_t i = 0; i < this->size(); ++i) {
        bits::writeWBits(this->read(i) & bits::UINTW_MAX(w), array_, i * w, w, bits::UINTW_MAX(w));
      }
    }
    w_ = w; // set w_
  }


  uint8_t getW() const noexcept {
    return w_;
  }
};

#endif

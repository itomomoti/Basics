#ifndef INCLUDE_GUARD_WBitsArray
#define INCLUDE_GUARD_WBitsArray

#include <stdint.h> // include uint64_t etc.
#include <assert.h>

#include <iterator>
#include <algorithm>

#include "Uncopyable.hpp"
#include "BitsUtil.hpp"


class WBitsArray;

// WBitsArrayIterator is used for a reference of WBitsArray
class WBitsArrayIterator :
  public std::iterator<std::input_iterator_tag, uint64_t>
{
  friend WBitsArray; // To use private constructors of WBitsArrayIterator in WBitsArray

  uint64_t * const array_;
  uint64_t pos_;
  const uint8_t w_;

private:
  //// Called by WBitsArray
  WBitsArrayIterator (uint64_t * array, uint8_t pos, uint8_t w) noexcept
    : array_(array), pos_(pos), w_(w)
  {}

public:
  //// copy
  WBitsArrayIterator(const WBitsArrayIterator & itr) noexcept = default;
  WBitsArrayIterator& operator=(const WBitsArrayIterator & itr) noexcept = default;
  //// move
  WBitsArrayIterator(WBitsArrayIterator && itr) noexcept = default;
  WBitsArrayIterator& operator=(WBitsArrayIterator && itr) noexcept = default;

  inline uint64_t read() const {
    return bits::readWBits(array_, pos_, w_, bits::getMaxW(w_));
  }

  inline void write(const uint64_t val) {
    bits::writeWBits(val, array_, pos_, w_, bits::getMaxW(w_));
  }

  ////// operator
  //// *itr
  inline uint64_t operator*() {
    return this->read();
  }

  //// ++itr
  inline WBitsArrayIterator & operator++() {
    pos_ += w_;
    return *this;
  }

  //// itr++
  inline WBitsArrayIterator operator++(int) {
    WBitsArrayIterator tmp(*this);
    ++(*this);
    return tmp;
  }

  //// --itr
  inline WBitsArrayIterator & operator--() {
    pos_ -= w_;
    return *this;
  }

  //// itr--
  inline WBitsArrayIterator operator--(int) {
    WBitsArrayIterator tmp(*this);
    --(*this);
    return tmp;
  }

  //// *this != itr
  inline bool operator!=(const WBitsArrayIterator & itr) {
    return (array_ != itr.array_ || pos_ != itr.pos_);
  }

  //// *this == itr
  inline bool operator==(const WBitsArrayIterator & itr) {
    return !(*this != itr);
  }

  ////// add more operators
  //// itr += diff
  inline WBitsArrayIterator & operator+=(const difference_type diff) noexcept {
    pos_ += static_cast<int64_t>(w_) * diff;
    return *this;
  }

  //// itr -= diff
  inline WBitsArrayIterator & operator-=(const difference_type diff) noexcept {
    *this += (-1 * diff);
    return *this;
  }

  //// itr + diff
  inline friend WBitsArrayIterator operator+(const WBitsArrayIterator & itr, const difference_type diff) noexcept {
    const int64_t pos = itr.pos_ + static_cast<int64_t>(itr.w_) * diff;
    return WBitsArrayIterator(itr.array_, pos, itr.w_);
  }

  //// diff + itr
  inline friend WBitsArrayIterator operator+(const difference_type diff, const WBitsArrayIterator & itr) noexcept {
    return itr + diff;
  }

  //// itr - diff
  inline friend WBitsArrayIterator operator-(const WBitsArrayIterator & itr, const difference_type diff) noexcept {
    return itr + (-1 * diff);
  }

  //// lhs - rhs
  inline friend difference_type operator-(const WBitsArrayIterator & lhs, const WBitsArrayIterator & rhs) noexcept {
    assert(lhs.w_ == rhs.w_);
    return (static_cast<int64_t>(lhs.pos_) - static_cast<int64_t>(rhs.pos_)) / lhs.w_;
  }

  //// lhs < rhs
  inline friend bool operator<(const WBitsArrayIterator & lhs, const WBitsArrayIterator & rhs) noexcept {
    return (lhs.pos_ < rhs.pos_);
  }

  //// rhs < lhs
  inline friend bool operator>(const WBitsArrayIterator & lhs, const WBitsArrayIterator & rhs) noexcept {
    return (rhs < lhs);
  }

  //// lhs <= rhs
  inline friend bool operator<=(const WBitsArrayIterator & lhs, const WBitsArrayIterator & rhs) noexcept {
    return !(lhs > rhs);
  }

  //// lhs >= rhs
  inline friend bool operator>=(const WBitsArrayIterator & lhs, const WBitsArrayIterator & rhs) noexcept {
    return !(lhs < rhs);
  }

  //// swap values
  // inline void swap(WBitsArrayIterator& lhs, WBitsArrayIterator& rhs) noexcept {
  //   assert(lhs.w_ == rhs.w_);
  //   const uint64_t lval = *lhs;
  //   lhs.write(*rhs);
  //   rhs.write(lval);
  // }
};

// template<>
// inline void std::swap<WBitsArrayIterator>(WBitsArrayIterator & lhs, WBitsArrayIterator & rhs) noexcept {
//   uint64_t lval = *lhs;
//   lhs.write(*rhs);
//   rhs.write(lval);
// }



class WBitsArray
  : Uncopyable
{
  size_t capacity_;
  size_t size_;
  uint8_t w_;
  uint64_t * array_;

	static const int8_t realloc_param_ = 2; // realloc size is "realloc_param_" times larger than the current size

public:
  using iterator = WBitsArrayIterator;

public:
  WBitsArray(size_t capacity, uint8_t w) : capacity_(capacity), size_(0), w_(w)
  {
    assert(capacity_ <= bits::getMaxW(58));
    assert(w_ <= 64);
    size_t len = capacity_ * w_ / 64 + 2; // +1 for roundup, and another +1 for margin
    array_ = NULL;
    array_ = new uint64_t[len];
    if (array_ == NULL) {
      abort();
    }
  }


  ~WBitsArray()
  {
    delete[] array_;
  }


  //// get iterator
  inline WBitsArray::iterator getItrAt(size_t idx) noexcept {
    return WBitsArrayIterator(array_, idx * w_, w_);
  }


  inline WBitsArray::iterator begin() noexcept {
    return getItrAt(0);
  }


  inline WBitsArray::iterator end() noexcept {
    return getItrAt(size_);
  }


  ////
  // We do not support "A[i]" to read/write
  inline uint64_t read(const size_t idx) const
  {
    assert(idx <= capacity_);
    return bits::readWBits(array_, idx * w_, w_, bits::getMaxW(w_));
  }


  inline void write(const uint64_t val, const size_t idx)
  {
    assert(idx <= capacity_);
    assert(val <= bits::getMaxW(w_));
    bits::writeWBits(val, array_, idx * w_, w_, bits::getMaxW(w_));
  }


  // 
  inline size_t capacity() const noexcept {
    return capacity_;
  }


  inline size_t size() const noexcept {
    return size_;
  }


  size_t calcSpace() const noexcept {
    return sizeof(this) + (capacity_ * w_ / 8) + 2 * sizeof(uint64_t);
  }


  inline bool empty() const noexcept {
    return (size_ == 0);
  }


  inline void clear() noexcept {
    size_ = 0;
  }


  inline void reserve() {
    //
  }


  inline void resize(const size_t newSize) {
    if (newSize <= capacity_) {
      size_ = newSize;
    } else {
      // reserve
    }
  }


  inline void setW(const uint8_t w, const bool convert = true) {
    w_ = w;
    if (convert) {
      // convert the values in array
    }
  }


  inline uint8_t getW() const noexcept {
    return w_;
  }
};

#endif

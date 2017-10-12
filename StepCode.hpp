/*!
 * Copyright (c) 2017 Tomohiro I
 *
 * This program is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
/*!
 // * @file StepCode.hpp
 // * @brief W-bits packed vector and its iterator.
 // * @author Tomohiro I
 // * @date 2017-01-24
 */
#ifndef INCLUDE_GUARD_StepCode
#define INCLUDE_GUARD_StepCode

#include <stdint.h> // include uint64_t etc.
#include <assert.h>

#include <iostream>
#include <iterator>
#include <algorithm>

#include "BitVec.hpp"
#include "BitsUtil.hpp"
#include "MemUtil.hpp"

/*!
 * @brief Simple space economic code for uints in which each uint is stored in 4*i bits with smallest possible integer i.
 * @tparam
 *   MAXNUM: Max number of uints to be stored. It is fixed as StepCode is intended to be used for small MAXNUM.
 * @attention
 *   We do not provide a constant-time random access.
 * @par notation
 *   Suppose that i-th (0base) uint occupies w_i bits (remark that w_i in {4, 8, 12, 16, ..., 64}).
 *   w_i/4 - 1 is called wCode of w_i (remark that w_i/4 - 1 in {0, 1, 2, 3, ..., 7}). wCodes are stored in wCodes_ using 4 bits each.
 */
template <uint32_t MAXNUM, typename SizeT = uint32_t>
class StepCode
{
public:
  static constexpr uint8_t STEP{4}; // Bits of each step.
  static constexpr uint8_t WCBITS{4}; // Bits of each wCode.
  static constexpr uint8_t WCNUM{16}; // Number of wCodes in 64 bits.


  static uint64_t calcWCode
  (
   const uint64_t val //!< in [0, 2^{64}).
   ) noexcept {
    return (bits::bitSize(val) - 1) / STEP;
  }


  static uint8_t calcStepW
  (
   const uint64_t val //!< in [0, 2^{64}).
   ) noexcept {
    return (bits::bitSize(val) + 3) / STEP * STEP;
  }


  /*!
   * @brief Read bit-width of "idxW"-th (0base) values.
   */
  static uint8_t readW_Arr
  (
   const uint64_t * wCodesArray,
   const uint64_t idxW //!< "(idxW + 1) * 4 - 1"-th bit of wCodesArray should not be out-of-bound.
   ) noexcept {
    return STEP * (bits::readWBits_S(wCodesArray, WCBITS * idxW, ctcbits::UINTW_MAX(WCBITS)) + 1);
  }


  /*!
   * @brief Read bit-width of "idxW"-th (0base) values.
   */
  static void writeWCode_Arr
  (
   const uint8_t code,
   uint64_t * wCodesArray,
   const uint64_t idxW //!< "(idxW + 1) * 4 - 1"-th bit of wCodesArray should not be out-of-bound.
   ) noexcept {
    bits::writeWBits_S(code, wCodesArray, WCBITS * idxW, ctcbits::UINTW_MAX(WCBITS));
  }


  /*!
   * @brief Calculate the beginning bit-pos of "idxW"-th value in bv.
   */
  static uint64_t calcBvPos_Arr
  (
   const uint64_t * wCodesArray,
   const uint64_t idxW //!< "(idxW + 1) * 4 - 1"-th bit of wCodesArray should not be out-of-bound.
   ) noexcept {
    auto sum = idxW;
    for (uint64_t i = 0; i < idxW / 16; ++i) {
      sum += sumWCodes(wCodesArray[i]);
    }
    uint8_t rem = idxW % 16;
    if (rem) {
      sum += sumWCodes(wCodesArray[idxW / 16] & bits::UINTW_MAX(rem * WCBITS));
    }
    return sum * STEP;
  }


  static uint64_t sumW_Arr
  (
   const uint64_t * wCodesArray,
   const uint64_t beg, //!< beg <= end.
   const uint64_t end //!< (exclusive) "(end + 1) * 4 - 1"-th bit of wCodesArray should not be out-of-bound.
   ) noexcept {
    assert(beg <= end);

    uint64_t sum = 0;
    for (uint64_t i = beg; i < end; ++i) {
      sum += readW_Arr(wCodesArray, i);
    }
    return sum;
  }


  static uint64_t sumWCodes
  (
   uint64_t codes //!< wCode.
   ) noexcept {
    codes = ((codes & 0xf0f0f0f0f0f0f0f0ULL)>>4) + (codes & 0x0f0f0f0f0f0f0f0fULL);
    codes += (codes>>8);
    codes += (codes>>16);
    codes += (codes>>32);
    return codes & 0xff;
  }


private:
  uint64_t wCodes_[MAXNUM / WCNUM]; //!< Store wCodes in 4 bits each.
  uint64_t * array_; //!< Array to store bits.
  SizeT bvCapacity_; //!< Current capacity (must be in [0, 2^58)).
  SizeT bvSize_; //!< Current size (must be in [0, capacity_]).
  SizeT size_; //!< Current size (number of elements).


public:
  StepCode
  (
   size_t initialBvCapacity = 0 //!< Initial capacity of bv_.
   ) : array_(nullptr), bvCapacity_(0), bvSize_(0), size_(0) {
    assert(initialBvCapacity <= ctcbits::UINTW_MAX(sizeof(SizeT) * 8));
    assert(initialBvCapacity <= ctcbits::UINTW_MAX(58));

    changeBvCapacity(initialBvCapacity);
  }


  //// Copy
  /*!
   * @brief Copy constructor.
   * @attention
   *   Since the contents of "other" are copied, it may take time when other.size_ is large.
   */
  StepCode
  (
   const StepCode & other
   ) : array_(nullptr), bvCapacity_(0), bvSize_(other.bvSize_), size_(other.size_) {
    if (size_ > 0) {
      changeBvCapacity(other.bvSize_); // Lazy reservation: If size_ == 0, we do not reserve anything.
      bits::cpBytes(other.array_, array_, (bvSize_ + 7) / 8);
      bits::cpBytes(other.wCodes_, wCodes_, (size_ * WCBITS + 7) / 8);
    }
  }


  /*!
   * @brief Assignment operator.
   * @attention
   *   - The contents of 'other' are copied.
   *   - It may take time when other.size_ is large.
   *   - The original contents are discarded.
   *   - Lazy allocation: Changing capacity is done only if it is needed.
   */
  StepCode & operator=
  (
   const StepCode & other
   ) {
    if (this != &other) {
      size_ = other.size_;
      if (bvCapacity_ < other.bvSize_) {
        changeBvCapacity(0);
        changeBvCapacity(other.bvSize_);
      }
      bvSize_ = other.bvSize_;
      if (size_ > 0) {
        bits::cpBytes(other.array_, array_, (bvSize_ + 7) / 8);
        bits::cpBytes(other.wCodes_, wCodes_, (size_ * WCBITS + 7) / 8);
      }
    }
    return *this;
  }


  //// Move
  /*!
   * @brief Move constructor.
   * @attention
   *   "other" is initialized to an object with capacity = 0.
   */
  StepCode
  (
   StepCode && other
   ) : array_(other.array_), bvCapacity_(other.bvCapacity_), bvSize_(other.bvSize_), size_(other.size_) {
    other.array_ = nullptr;
    other.bvCapacity_ = other.bvSize_ = other.size_ = 0;
    if (size_) {
      bits::cpBytes(other.wCodes_, wCodes_, (size_ * WCBITS + 7) / 8);
    }
  }


  /*!
   * @brief Move operator.
   * @attention
   *   - 'other' is initialized to an object with capacity = 0.
   *   - The original contents are discarded.
   */
  StepCode & operator=
  (
   StepCode && other
   ) {
    if (this != &other) {
      clear(); changeBvCapacity(); // clear() & changeBvCapacity() free array_
      array_ = other.array_;
      bvCapacity_ = other.bvCapacity_;
      bvSize_ = other.bvSize_;
      size_ = other.size_;
      if (size_) {
        bits::cpBytes(other.wCodes_, wCodes_, (size_ * WCBITS + 7) / 8);
      }
      other.array_ = nullptr;
      other.bvCapacity_ = other.bvSize_ = other.size_ = 0;
    }
    return *this;
  }


  ~StepCode()
  {
    free(array_);
  }


  /*!
   * @brief Get read-only array pointer.
   */
  const uint64_t * getConstArrayPtr() const noexcept
  {
    return array_;
  }


  /*!
   * @brief Get read-only wCodes_ array pointer.
   */
  const uint64_t * getConstWCodesArrayPtr() const noexcept
  {
    return wCodes_;
  }


  uint64_t sumW
  (
   const uint64_t beg, //!< in [0, bv_.size()).
   const uint64_t end //!< in [beg, bv_.size()).
   ) const noexcept {
    assert(beg <= end);
    assert(end <= size_);

    return sumW_Arr(wCodes_, beg, end);
  }


  /*!
   * @brief Read bit-width of "idxW"-th (0base) values.
   */
  uint8_t readW
  (
   const uint64_t idxW //!< in [0, size_).
   ) const noexcept {
    assert(idxW < size_);

    return readW_Arr(wCodes_, idxW);
  }


  /*!
   * @brief Calculate the beginning bit-pos of "idxW"-th value in bv.
   */
  uint64_t calcBvPos
  (
   const uint64_t idxW //!< "(idxW + 1) * 4 - 1"-th bit of wCodesArray should not be out-of-bound.
   ) const noexcept {
    assert(idxW < size_);

    return calcBvPos_Arr(wCodes_, idxW);
  }


  /*!
   * @brief Read "w"-bits written in the bit-region beginning at array_[[bitPos..]] in bv_.
   * @return Value represented by array_[[bitPos..bitPos+w)).
   * @pre The bit-region must not be out of bounds.
   */
  uint64_t readWBits
  (
   const uint64_t bitPos, //!< Bit-pos specifying the beginning position of the bit-region
   const uint8_t w, //!< Bit-width in [0, 64].
   const uint64_t mask //!< UINTW_MAX(w).
   ) const noexcept {
    assert(bitPos < bvCapacity_);
    assert(bitPos + w <= bvCapacity_);
    assert(w <= 64);

    return bits::readWBits(array_, bitPos, w, mask);
  }


  /*!
   * @brief Write "w"-bit value "val" to the bit-region beginning at array_[[bitPos..]].
   * @pre The bit-region must not be out of bounds.
   */
  void writeWBits
  (
   const uint64_t val, //!< in [0, 2^w).
   const uint64_t bitPos, //!< Bit-pos.
   const uint8_t w, //!< Bit-width in [0, 64].
   const uint64_t mask //!< UINTW_MAX(w).
   ) {
    assert(bitPos < bvCapacity_);
    assert(bitPos + w <= bvCapacity_);
    assert(w <= 64);
    assert(val == 0 || bits::bitSize(val) <= w);

    bits::writeWBits(val, array_, bitPos, w, mask);
  }


  /*!
   * @brief Read "idx"-th (0base) value.
   * @note It may be inefficient for sequencial access. Use readW and readWBits in that case.
   */
  uint64_t read
  (
   uint64_t idx //!< in [0, size_).
   ) const noexcept {
    assert(idx < size_);

    const auto bitPos = calcBvPos_Arr(wCodes_, idx);
    const auto w = readW(idx);
    return readWBits(bitPos, w, bits::UINTW_MAX(w));
  }


  /*!
   * @brief Read "idx"-th (0base) value.
   * @note It may be inefficient for sequencial access. Use readW and readWBits in that case.
   */
  uint64_t read_naive
  (
   uint64_t idx //!< in [0, size_).
   ) const noexcept {
    assert(idx < size_);

    uint64_t bitPos = 0;
    for (uint64_t i = 0; i < idx; ++i) {
      auto bitPos = readW(i);
    }
    const auto w = readW(idx);
    return readWBits(bitPos, w, bits::UINTW_MAX(w));
  }


  /*!
   * @brief Write uint "val" at the end.
   * @pre The resulting size_ and bvSize_ should be within capacity.
   */
  void append
  (
   const uint64_t val, //!< Uint val.
   const uint8_t stepW //!<  bit-width in {4, 8, 12, ..., 64} into which val fits.
   ) {
    assert(size_ < MAXNUM);
    assert(bvSize_ + stepW <= bvCapacity_);
    assert(stepW <= 64 && stepW % 4 == 0);
    assert(bits::bitSize(val) <= stepW);

    const uint8_t wCode = stepW / STEP - 1;
    const auto begPos = bvSize_;
    bvSize_ += stepW;
    writeWBits(val, bvSize_, stepW, bits::UINTW_MAX(stepW));
    bits::writeWBits_S(wCode, wCodes_, WCBITS * size_, ctcbits::UINTW_MAX(WCBITS));
    ++size_;
  }


  /*!
   * @brief Write uint "val" at the end.
   * @pre The resulting size_ and bvSize_ should be within capacity.
   */
  void append
  (
   const uint64_t val //!< Uint val.
   ) {
    assert(size_ < MAXNUM);
    assert(bvSize_ + (bits::bitSize(val) + 3) / STEP * STEP <= bvCapacity_);

    const auto wCode = calcWCode(val);
    const auto w = (wCode + 1) * STEP;
    const auto begPos = bvSize_;
    bvSize_ += w;
    writeWBits(val, begPos, w, bits::UINTW_MAX(w));
    bits::writeWBits_S(wCode, wCodes_, WCBITS * size_, ctcbits::UINTW_MAX(WCBITS));
    ++size_;
  }


  /*!
   * @brief Rewrite value at "idx" to "val" without changing bit-width.
   */
  void rewriteVal
  (
   const uint64_t val, //!< should fit in readW(idxW) bits.
   const uint64_t idxW, //!< in [0, size_).
   const uint64_t bitPos //!< should be bit-position of "idxW"-th (0base) values in bv_.
   ) {
    assert(idxW < size_);
    assert(bits::bitSize(val) <= UINTW_MAX(readW(idxW)));

    const auto w = readW(idxW);
    writeWBits(val, bitPos, w, bits::UINTW_MAX(w));
  }


  /*!
   * @brief Get current capacity.
   * @pre The resulting size_ and bv_.size_ should be within capacity.
   */
  void changeWCodes
  (
   const uint64_t * src,
   const uint64_t srcIdxBeg, //!< Beginning idx of src.
   const uint64_t srcLen, //!< Length of wCodes of src to insert.
   const uint64_t tgtIdxBeg, //!< Beginning idx of tgt.
   const uint64_t tgtLen, //!< Length of wCodes of tgt to delete.
   const uint64_t bitPos, //!< bitPos of bv_.
   const uint64_t insBitLen, //!< Bit-length to insert in bv_
   const uint64_t delBitLen //!< Bit-length to delete in bv_
   ) {
    assert(tgtIdxBeg + tgtLen <= size_);
    assert(size_ - tgtLen + srcLen < MAXNUM);
    assert(size_ + srcLen >= tgtLen);

    const auto tailNum = size_ - (tgtIdxBeg + tgtLen); // at least 0 by assumption.
    { // Update wCodes_.
      if (tailNum && srcLen != tgtLen) { // Need to shift elements in wCodes_.
        const auto mvSrcPos = (tgtIdxBeg + tgtLen) * WCBITS;
        const auto mvTgtPos = (tgtIdxBeg + srcLen) * WCBITS;
        bits::mvBits(wCodes_ + (mvSrcPos / 64), mvSrcPos % 64, wCodes_ + (mvTgtPos / 64), mvTgtPos % 64, tailNum * WCBITS);
      }
      if (srcLen) {
        const auto mvSrcPos = srcIdxBeg * WCBITS;
        const auto mvTgtPos = tgtIdxBeg * WCBITS;
        bits::mvBits(src + (mvSrcPos / 64), mvSrcPos % 64, wCodes_ + (mvTgtPos / 64), mvTgtPos % 64, srcLen * WCBITS);
      }
      size_ += srcLen - tgtLen;
    }

    { // Update bv_.
      if (tailNum && insBitLen != delBitLen) { // Need to shift values.
        const uint64_t srcPos = bitPos + delBitLen;
        const uint64_t tgtPos = bitPos + insBitLen;
        bits::mvBits(array_ + (srcPos / 64), srcPos % 64, array_ + (tgtPos / 64), tgtPos % 64, bvSize_ - srcPos);
      }
      bvSize_ += insBitLen - delBitLen;
    }
  }

  
  /*!
   * @brief Get current capacity of bv_.
   */
  size_t bvCapacity() const noexcept {
    return bvCapacity_;
  }


  /*!
   * @brief Get current size of bv_.
   */
  size_t bvSize() const noexcept {
    return bvSize_;
  }


  /*!
   * @brief Get current capacity.
   */
  size_t capacity() const noexcept {
    return MAXNUM;
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
    return sizeof(*this) + sizeof(uint64_t) * (bvCapacity_ / 64);
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
   * @brief Change capacity to max of givenCapacity and current size.
   * @node If givenCapacity is not given, it works (with default parameter 0) as shrink_to_fit.
   */
  void changeBvCapacity
  (
   const size_t givenCapacity = 0
   ) {
    assert(givenCapacity <= ctcbits::UINTW_MAX(sizeof(SizeT) * 8));
    assert(givenCapacity <= ctcbits::UINTW_MAX(58));

    if (bvCapacity_ != givenCapacity) {
      const size_t newLen = (std::max(static_cast<size_t>(bvSize_), givenCapacity) + 63) / 64; // +63 for roundup
      if (newLen > 0) {
        memutil::realloc_AbortOnFail<uint64_t>(array_, newLen);
        bvCapacity_ = newLen * 64;
      } else {
        memutil::safefree(array_);
        bvCapacity_ = 0;
      }
    }
  }


  void printStatistics
  (
   const bool verbose = false
   ) const noexcept {
    std::cout << "StepCode object (" << this << ") " << __func__ << "(" << verbose << ") BEGIN" << std::endl;
    std::cout << "size = " << this->size() << ", capacity = " << this->capacity() << std::endl;
    std::cout << this->calcMemBytes() << " bytes (dynamic array = " << sizeof(uint64_t) * (bvCapacity_ / 64) << ")" << std::endl;
    if (verbose) {
      {
        const auto size = this->size();
        std::cout << "dump bit witdth stored in wCodes (" << wCodes_ << ")" << std::endl;
        for (uint64_t i = 0; i < size_; ++i) {
          std::cout << (uint64_t)(readW(i)) << " ";
        }
        std::cout << std::endl;
      }
      {
        const auto size = this->size();
        std::cout << "dump values" << std::endl;
        for (uint64_t i = 0; i < size_; ++i) {
          std::cout << read(i) << " ";
        }
        std::cout << std::endl;
      }
      {
        const auto size = this->bvSize();
        std::cout << "bvSize = " << size << ", bvCapacity = " << this->bvCapacity() << std::endl;
        std::cout << "dump bits in array_ (" << array_ << ")" << std::endl;
        for (uint64_t i = 0; i < (size + 63) / 64; ++i) {
          std::cout << "(" << i << ")";
          for (uint64_t j = 0; j < 64; ++j) {
            std::cout << bits::readWBits_S(array_, 64 * i + 63 - j, ctcbits::UINTW_MAX(1));
          }
          std::cout << " ";
        }
        std::cout << std::endl;
      }
    }
    std::cout << "StepCode object (" << this << ") " << __func__ << "(" << verbose << ") END" << std::endl;
  }
};

#endif

/*!
 * Copyright (c) 2017 Tomohiro I
 *
 * This program is released under the MIT License.
 * http://opensource.org/licenses/mit-license.php
 */
/*!
 * @file BasicSearch.hpp
 * @brief Useful functions to work on bits.
 * @author Tomohiro I
 * @date 2017-08-20
 */
#ifndef INCLUDE_GUARD_BasicSearch
#define INCLUDE_GUARD_BasicSearch

#include <stdint.h> // include uint64_t etc.
#include <assert.h>


/*!
 * @namespace basic_search
 * @brief Basic search.
 */
namespace basic_search
{
  /*! zzz
   * @brief My implimentation of lower_bound on a simple array.
   * @tparam ArrayType: typename supporting access with [] operator.
   * @tparam LS: Thereshold to switch to liniear search.
   * @attention The range must be sorted in increasing order. The answer must exist in the range.
   */
  template<uint64_t LS = 8, class Predicate>
    uint64_t lb
  (
   uint64_t lb, //!< Initial lower bound (inclusive).
   uint64_t ub, //!< Initial upper bound (exclusive).
   Predicate func
   ) noexcept {
    assert(lb < ub);
    assert(ub <= UINT64_MAX - LS);

    if (LS != UINT64_MAX) { // If LS == UINT64_MAX, it is just a linear search.
      while (lb + LS + 1 < ub) { // invariant: the answer is in [lb..ub).
        auto mid = (lb + ub) / 2;
        if (!func(mid - 1)) {
          lb = mid;
        } else {
          ub = mid;
        }
      }
    }
    if (LS) {
      while (!func(lb)) {
        ++lb;
      }
    }

    return lb;
  }
}

#if 0
  template<uint64_t STEP, typename ArrayType>
  struct ComplArray
  {
    const ArrayType & array_;

    ComplArray(const ArrayType & array) : array_(array)
    {
    }

    uint64_t operator[](size_t i) const
    {
      return STEP * (i+1) - array_[i];
    }
  };


  /*!
   * @brief Helper function to make ComplArray.
   */
  template<uint64_t STEP, typename ArrayType>
  auto makeComplArray(const ArrayType & array) {
    return ComplArray<STEP, decltype(array)>(array);
  }


  /*!
   * @brief My implimentation of lower_bound on a simple array.
   * @tparam ArrayType: typename supporting access with [] operator.
   * @tparam LS: Thereshold to switch to liniear search.
   * @attention The range must be sorted in increasing order. The answer must exist in the range.
   */
  template<uint64_t LS = 16, typename ArrayType>
    uint64_t lb_inc
  (
   const ArrayType & array,
   const uint64_t val, //!< key value to search lower bound.
   uint64_t lb, //!< Initial lower bound (inclusive).
   uint64_t ub //!< Initial upper bound (exclusive).
   ) noexcept {
    assert(array[ub - 1] >= val);
    assert(lb < ub);
    assert(ub <= UINT64_MAX - LS);

    if (LS != UINT64_MAX) { // If LS == UINT64_MAX, it is just a linear search.
      while (lb + LS + 1 < ub) { // invariant: the answer is in [lb..ub).
        auto mid = (lb + ub) / 2;
        if (array[mid - 1] < val) {
          lb = mid;
        } else {
          ub = mid;
        }
      }
    }
    if (LS) {
      while (array[lb] < val) {
        ++lb;
      }
    }

    return lb;
  }


  /*!
   * @brief My implimentation of lower_bound on a simple array.
   * @tparam ArrayType: typename supporting access with [] operator.
   * @tparam LS: Thereshold to switch to liniear search.
   * @attention The range must be sorted in decreasing order. The answer must exist in the range.
   */
  template<typename ArrayType, uint64_t LS = 16>
    uint64_t lb_dec
  (
   const ArrayType & array,
   const uint64_t val, //!< key value to search lower bound.
   uint64_t lb, //!< Initial lower bound (inclusive).
   uint64_t ub //!< Initial upper bound (exclusive).
   ) {
    assert(array[ub - 1] <= val);
    assert(lb < ub);

    while (lb + LS + 1 < ub) { // invariant: the answer is in [lb..ub)
      auto mid = (lb + ub) / 2;
      if (array[mid - 1] > val) {
        lb = mid;
      } else {
        ub = mid;
      }
    }
    if (LS) {
      while (lb + 1 < ub && array[lb] > val) {
        ++lb;
      }
    }

    return lb;
  }
}
#endif

#endif

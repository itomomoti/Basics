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
  /*!
   * @brief My implimentation of lower_bound on a simple array.
   * @tparam ArrayType: typename supporting access with [] operator.
   * @tparam LS: Thereshold to switch to liniear search.
   * @attention The range must be sorted in increasing order. The answer must exist in the range.
   */
  template<typename ArrayType, uint64_t LS = 16>
    uint64_t lb
  (
   const ArrayType & array,
   const uint64_t val, //!< key value to search lower bound.
   uint64_t lb, //!< Initial lower bound.
   uint64_t ub //!< Initial upper bound.
   ) {
    assert(array[ub - 1] >= val);

    while (lb + LS + 1 < ub) { // invariant: the answer is in [lb..ub)
      auto mid = (lb + ub) / 2;
      if (array[mid - 1] < val) {
        lb = mid;
      } else {
        ub = mid;
      }
    }
    while (array[lb] < val) {
      ++lb;
    }

    return lb;
  }
}


#endif

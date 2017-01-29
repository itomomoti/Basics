#ifndef INCLUDE_GUARD_MemUtil
#define INCLUDE_GUARD_MemUtil

#include <stdint.h> // include uint64_t etc.
#include <assert.h>
#include <iostream>

namespace memutil
{
  template <typename T>
  T * mymalloc(size_t n) {
    assert(n > 0);

    T * ptr = static_cast<T *>(malloc(n * sizeof(T)));
    if (ptr == NULL) {
      std::cout << "not enough memory at line " << __LINE__ << std::endl;
      exit(EXIT_FAILURE);
    }
    return ptr;
  }


  template <typename T>
  void myrealloc(T *& ptr, size_t n) {
    assert(n > 0);

    ptr = static_cast<T *>(realloc(ptr, n * sizeof(T)));
    if (ptr == NULL) {
      std::cout << "not enough memory at line " << __LINE__ << std::endl;
      exit(EXIT_FAILURE);
    }
  }


  template <typename T>
  void myfree(T *& ptr) {
    free(ptr);
    ptr = NULL;
  }


  template <typename T>
  void mydelete(T *& ptr) {
    delete ptr;
    ptr = NULL;
  }
}

#endif

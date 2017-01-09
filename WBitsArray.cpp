#include "WBitsArray.hpp"


#define TEST_Basics_
#ifdef TEST_Basics_
#include <time.h>
#include <iostream>
#include <iomanip>
#include "cmdline.h"
#endif




#ifdef TEST_Basics_ // PERFORMANCE_TEST
int main(int argc, char *argv[])
{
  cmdline::parser parser;
  parser.add<uint64_t>("num",'n', "num of unsigned integers", true);
  // parser.add<uint8_t>("width", 'w', "bit width for packed vector", false, 8, cmdline::range(1, 64));
  parser.add<uint64_t>("jump", 'j', "jump for random access", false, 38201); // 38201 is a prime number
  parser.add<uint64_t>("val", 'v', "value to write (should fit in w bits)", false, 1);
  parser.add<uint8_t>("dummy", 0, "dummy argument (do not input this)", false, 0);
  parser.add("help", 0, "pring help");

  parser.parse_check(argc, argv);
  const uint64_t num = parser.get<uint64_t>("num");
  // const uint8_t w = parser.get<uint8_t>("width");
  const uint64_t jump = parser.get<uint64_t>("jump");
  uint64_t val = parser.get<uint64_t>("val");
  const uint8_t dummy = parser.get<uint8_t>("dummy");

  const uint64_t idxMask = bits::getMaxW(bits::bitSize(num));
  // std::cout << "jump: " << jump << std::endl;

  assert(num > 2);

  WBitsArray wbArray(num, 64);
  wbArray.resize(num);

  clock_t start, end;
  double sec;  

  std::cout << __FILE__ << " sequential read (itr): " << std::endl;
  for (uint8_t w = 6; w <= 58; w += 7) {
    w += dummy;
    wbArray.setW(w);
    const uint64_t valMask = (1ULL << w) - 1;
    uint64_t checksum0 = 0;
    for (uint64_t i = 0; i < num; ++i) {
      checksum0 += i & valMask;
    }
    for (uint64_t i = 0; i < num; ++i) { // write some values for checksum
      wbArray.write(i & valMask, i);
    }
    uint64_t checksum = 0;
    start = clock();
    auto itr = wbArray.begin();
    for (uint64_t i = 0; i < num; ++i, ++itr) {
      checksum += itr.read();
    }
    end = clock();
    sec = (double)(end - start)/CLOCKS_PER_SEC;
    std::cout << "[" << static_cast<uint64_t>(w) << "] " << std::setw(8) << sec << "; ";
    if (checksum0 != checksum) {
      std::cout << std::endl << "error: checksum = " << checksum << " should be " << checksum0 << std::endl;
    }
  }
  std::cout << std::endl;

  std::cout << __FILE__ << " sequential write (itr): " << std::endl;
  for (uint8_t w = 6; w <= 58; w += 7) {
    w += dummy;
    wbArray.setW(w);
    start = clock();
    auto itr = wbArray.begin();
    for (uint64_t i = 0; i < num; ++i, ++itr) {
      itr.write(val);
    }
    end = clock();
    sec = (double)(end - start)/CLOCKS_PER_SEC;
    std::cout << "[" << static_cast<uint64_t>(w) << "] " << std::setw(8) << sec << "; ";
  }
  std::cout << std::endl;


  std::cout << __FILE__ << " sequential read: " << std::endl;
  for (uint8_t w = 6; w <= 58; w += 7) {
    w += dummy;
    wbArray.setW(w);
    const uint64_t valMask = (1ULL << w) - 1;
    uint64_t checksum0 = 0;
    for (uint64_t i = 0; i < num; ++i) {
      checksum0 += i & valMask;
    }
    for (uint64_t i = 0; i < num; ++i) { // write some values for checksum
      wbArray.write(i & valMask, i);
    }
    uint64_t checksum = 0;
    start = clock();
    for (uint64_t i = 0; i < num; ++i) {
      checksum += wbArray.read(i);
    }
    end = clock();
    sec = (double)(end - start)/CLOCKS_PER_SEC;
    std::cout << "[" << static_cast<uint64_t>(w) << "] " << std::setw(8) << sec << "; ";
    if (checksum0 != checksum) {
      std::cout << std::endl << "error: checksum = " << checksum << " should be " << checksum0 << std::endl;
    }
  }
  std::cout << std::endl;

  std::cout << __FILE__ << " sequential write: " << std::endl;
  for (uint8_t w = 6; w <= 58; w += 7) {
    w += dummy;
    wbArray.setW(w);
    start = clock();
    for (uint64_t i = 0; i < num; ++i) {
      wbArray.write(val, i);
    }
    end = clock();
    sec = (double)(end - start)/CLOCKS_PER_SEC;
    std::cout << "[" << static_cast<uint64_t>(w) << "] " << std::setw(8) << sec << "; ";
  }
  std::cout << std::endl;



  std::cout << __FILE__ << " random read: " << std::endl;
  for (uint8_t w = 6; w <= 58; w += 7) {
    w += dummy;
    wbArray.setW(w);
    const uint64_t valMask = (1ULL << w) - 1;
    uint64_t checksum0 = 0;
    for (uint64_t i = 0; i < num; ++i) { // write some values for checksum
      wbArray.write(i & valMask, i);
    }
    for (uint64_t i = 0, pos = jump & idxMask; i < num; ++i, pos = (pos + jump) & idxMask) {
      checksum0 += wbArray.read(pos);
    }
    for (uint64_t i = 0; i < num; ++i) { // write some values for checksum
      wbArray.write(i & valMask, i);
    }
    uint64_t checksum = 0;
    start = clock();
    for (uint64_t i = 0, pos = jump & idxMask; i < num; ++i, pos = (pos + jump) & idxMask) {
      checksum += wbArray.read(pos);
    }
    end = clock();
    sec = (double)(end - start)/CLOCKS_PER_SEC;
    std::cout << "[" << (uint64_t)w << "] " << std::setw(8) << sec << "; ";
    if (checksum0 != checksum) {
      std::cout << std::endl << "error: checksum = " << checksum << " should be " << checksum0 << std::endl;
    }
  }
  std::cout << std::endl;

  std::cout << __FILE__ << " random write: " << std::endl;
  for (uint8_t w = 6; w <= 58; w += 7) {
    w += dummy;
    wbArray.setW(w);
    start = clock();
    for (uint64_t i = 0, pos = jump & idxMask; i < num; ++i, pos = (pos + jump) & idxMask) {
      wbArray.write(val, pos);
    }
    end = clock();
    sec = (double)(end - start)/CLOCKS_PER_SEC;
    std::cout << "[" << (uint64_t)w << "] " << std::setw(8) << sec << "; ";
  }
  std::cout << std::endl;

}
#endif


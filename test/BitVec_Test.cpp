#include "BitVec.hpp"
#include <gtest/gtest.h>
#include <stdint.h>

namespace {
  class BitVec_Test : public ::testing::Test
  {
  };


  TEST_F(BitVec_Test, Constructor)
  {
    size_t num = 8200;

    BitVec bv1(num);
    bv1.resize(num);
    for (uint64_t j = 0; j < num; ++j) {
      bv1.writeBit((UINT64_C(1) << (j % 64)) & bits::UINTW_MAX(1), j);
    }
    {
      BitVec bv2(bv1);
      ASSERT_EQ(bv1.size(), bv2.size());
      for (uint64_t j = 0; j < num; ++j) {
        ASSERT_EQ(bv1.readBit(j), bv2.readBit(j));
      }
    }
    {
      BitVec bv2(num/2);
      bv2 = bv1;
      ASSERT_EQ(bv1.size(), bv2.size());
      for (uint64_t j = 0; j < num; ++j) {
        ASSERT_EQ(bv1.readBit(j), bv2.readBit(j));
      }
    }
    {
      BitVec bv_copy(bv1);
      BitVec bv2(std::move(bv_copy));
      ASSERT_EQ(0, bv_copy.size());
      ASSERT_EQ(0, bv_copy.capacity());
      ASSERT_EQ(bv1.size(), bv2.size());
      for (uint64_t j = 0; j < num; ++j) {
        ASSERT_EQ(bv1.readBit(j), bv2.readBit(j));
      }
    }
    {
      BitVec bv_copy(bv1);
      BitVec bv2(num/2);
      bv2 = std::move(bv_copy);
      ASSERT_EQ(0, bv_copy.size());
      ASSERT_EQ(0, bv_copy.capacity());
      ASSERT_EQ(bv1.size(), bv2.size());
      for (uint64_t j = 0; j < num; ++j) {
        ASSERT_EQ(bv1.readBit(j), bv2.readBit(j));
      }
    }
  }


  TEST_F(BitVec_Test, ReadWrite)
  {
    size_t num = 8200;

    BitVec bv(num);
    bv.resize(num);
    for (uint64_t j = 0; j < num; ++j) {
      bv.writeBit((UINT64_C(1) << (j % 64)) & bits::UINTW_MAX(1), j);
    }
    for (uint64_t j = 0; j < num; ++j) {
      ASSERT_EQ((UINT64_C(1) << (j % 64)) & bits::UINTW_MAX(1), bv.readBit(j));
    }
  }


} // namespace




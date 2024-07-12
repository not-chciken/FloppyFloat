/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 ******************************************************************************/

#include <gtest/gtest.h>

#include <array>

#include "utils.h"

std::array<f32, 12> values = {1.f,
                              -1.f,
                              +0.f,
                              -0.f,
                              nl<f32>::signaling_NaN(),
                              nl<f32>::quiet_NaN(),
                              nl<f32>::infinity(),
                              -nl<f32>::infinity(),
                              nl<f32>::denorm_min(),
                              -nl<f32>::denorm_min(),
                              std::bit_cast<f32>(0x7fe00000u),   // NaN with positive sign.
                              std::bit_cast<f32>(0xffe00000u)};  // NaN with negative sign.

TEST(UtilTests, IsInf) {
  ASSERT_EQ(IsInf(values[0]), false);
  ASSERT_EQ(IsInf(values[1]), false);
  ASSERT_EQ(IsInf(values[2]), false);
  ASSERT_EQ(IsInf(values[3]), false);
  ASSERT_EQ(IsInf(values[4]), false);
  ASSERT_EQ(IsInf(values[5]), false);
  ASSERT_EQ(IsInf(values[6]), true);
  ASSERT_EQ(IsInf(values[7]), true);
  ASSERT_EQ(IsInf(values[8]), false);
  ASSERT_EQ(IsInf(values[9]), false);
  ASSERT_EQ(IsInf(values[10]), false);
  ASSERT_EQ(IsInf(values[11]), false);
}

TEST(UtilTests, IsInfOrNan) {
  ASSERT_EQ(IsInfOrNan(values[0]), false);
  ASSERT_EQ(IsInfOrNan(values[1]), false);
  ASSERT_EQ(IsInfOrNan(values[2]), false);
  ASSERT_EQ(IsInfOrNan(values[3]), false);
  ASSERT_EQ(IsInfOrNan(values[4]), true);
  ASSERT_EQ(IsInfOrNan(values[5]), true);
  ASSERT_EQ(IsInfOrNan(values[6]), true);
  ASSERT_EQ(IsInfOrNan(values[7]), true);
  ASSERT_EQ(IsInfOrNan(values[8]), false);
  ASSERT_EQ(IsInfOrNan(values[9]), false);
  ASSERT_EQ(IsInfOrNan(values[10]), true);
  ASSERT_EQ(IsInfOrNan(values[11]), true);
}

TEST(UtilTests, IsNan) {
  ASSERT_EQ(IsNan(values[0]), false);
  ASSERT_EQ(IsNan(values[1]), false);
  ASSERT_EQ(IsNan(values[2]), false);
  ASSERT_EQ(IsNan(values[3]), false);
  ASSERT_EQ(IsNan(values[4]), true);
  ASSERT_EQ(IsNan(values[5]), true);
  ASSERT_EQ(IsNan(values[6]), false);
  ASSERT_EQ(IsNan(values[7]), false);
  ASSERT_EQ(IsNan(values[8]), false);
  ASSERT_EQ(IsNan(values[9]), false);
  ASSERT_EQ(IsNan(values[10]), true);
  ASSERT_EQ(IsNan(values[11]), true);
}

TEST(UtilTests, IsNegInf) {
  ASSERT_EQ(IsNegInf(values[0]), false);
  ASSERT_EQ(IsNegInf(values[1]), false);
  ASSERT_EQ(IsNegInf(values[2]), false);
  ASSERT_EQ(IsNegInf(values[3]), false);
  ASSERT_EQ(IsNegInf(values[4]), false);
  ASSERT_EQ(IsNegInf(values[5]), false);
  ASSERT_EQ(IsNegInf(values[6]), false);
  ASSERT_EQ(IsNegInf(values[7]), true);
  ASSERT_EQ(IsNegInf(values[8]), false);
  ASSERT_EQ(IsNegInf(values[9]), false);
  ASSERT_EQ(IsNegInf(values[10]), false);
  ASSERT_EQ(IsNegInf(values[11]), false);
}

TEST(UtilTests, IsPosInf) {
  ASSERT_EQ(IsPosInf(values[0]), false);
  ASSERT_EQ(IsPosInf(values[1]), false);
  ASSERT_EQ(IsPosInf(values[2]), false);
  ASSERT_EQ(IsPosInf(values[3]), false);
  ASSERT_EQ(IsPosInf(values[4]), false);
  ASSERT_EQ(IsPosInf(values[5]), false);
  ASSERT_EQ(IsPosInf(values[6]), true);
  ASSERT_EQ(IsPosInf(values[7]), false);
  ASSERT_EQ(IsPosInf(values[8]), false);
  ASSERT_EQ(IsPosInf(values[9]), false);
  ASSERT_EQ(IsPosInf(values[10]), false);
  ASSERT_EQ(IsPosInf(values[11]), false);
}

TEST(UtilTests, IsSnan) {
  ASSERT_EQ(IsSnan(values[0]), false);
  ASSERT_EQ(IsSnan(values[1]), false);
  ASSERT_EQ(IsSnan(values[2]), false);
  ASSERT_EQ(IsSnan(values[3]), false);
  ASSERT_EQ(IsSnan(values[4]), true);
  ASSERT_EQ(IsSnan(values[5]), false);
  ASSERT_EQ(IsSnan(values[6]), false);
  ASSERT_EQ(IsSnan(values[7]), false);
  ASSERT_EQ(IsSnan(values[8]), false);
  ASSERT_EQ(IsSnan(values[9]), false);
  ASSERT_EQ(IsSnan(values[10]), false);
  ASSERT_EQ(IsSnan(values[11]), false);
}

TEST(UtilTests, IsZero) {
  ASSERT_EQ(IsZero(values[0]), false);
  ASSERT_EQ(IsZero(values[1]), false);
  ASSERT_EQ(IsZero(values[2]), true);
  ASSERT_EQ(IsZero(values[3]), true);
  ASSERT_EQ(IsZero(values[4]), false);
  ASSERT_EQ(IsZero(values[5]), false);
  ASSERT_EQ(IsZero(values[6]), false);
  ASSERT_EQ(IsZero(values[7]), false);
  ASSERT_EQ(IsZero(values[8]), false);
  ASSERT_EQ(IsZero(values[9]), false);
  ASSERT_EQ(IsZero(values[10]), false);
  ASSERT_EQ(IsZero(values[11]), false);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
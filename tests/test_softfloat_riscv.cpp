/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 ******************************************************************************/

#include <gtest/gtest.h>

#include <bit>
#include <cmath>
#include <functional>
#include <iostream>
#include <type_traits>

#include "float_rng.h"
#include "floppy_float.h"

extern "C" {
#include "softfloat/softfloat.h"
}

using namespace std::placeholders;
using namespace FfUtils;

constexpr i32 kNumIterations = 50000;
constexpr i32 kRngSeed = 42;

FloppyFloat ff;

template <typename T>
struct FFloatToSFloat;

template <>
struct FFloatToSFloat<f16> {
  using type = float16_t;
};
template <>
struct FFloatToSFloat<f32> {
  using type = float32_t;
};
template <>
struct FFloatToSFloat<f64> {
  using type = float64_t;
};

template <typename T>
auto ToComparableType(T a) {
  if constexpr (std::is_floating_point<decltype(a)>::value) {
    return std::bit_cast<typename FloatToUint<T>::type>(a);
  } else if constexpr (std::is_same_v<decltype(a), float16_t>) {
    return a.v;
  } else if constexpr (std::is_same_v<decltype(a), float32_t>) {
    return a.v;
  } else if constexpr (std::is_same_v<decltype(a), float64_t>) {
    return a.v;
  } else {
    return a;
  }
}

template <typename T1, typename T2>
void CheckResult(T1 ff_result_u, T2 sf_result_u, size_t i) {
  ASSERT_EQ(ff_result_u, sf_result_u) << "Iteration: " << i;
  ASSERT_EQ(ff.invalid, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_invalid)) << "Iteration: " << i;
  ASSERT_EQ(ff.division_by_zero, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_infinite)) << "Iteration: " << i;
  ASSERT_EQ(ff.overflow, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_overflow)) << "Iteration: " << i;
  ASSERT_EQ(ff.underflow, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_underflow)) << "Iteration: " << i;
  ASSERT_EQ(ff.inexact, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_inexact)) << "Iteration: " << i;
}

template <typename FT, typename FFFUNC, typename SFFUNC, int num_args>
void DoTest(FFFUNC ff_func, SFFUNC sf_func) {
  ff.SetupToRiscv();

  ::softfloat_exceptionFlags = 0;
  ff.ClearFlags();

  FloatRng<FT> float_rng(kRngSeed);
  FT valuefa{float_rng.Gen()};
  typename FFloatToSFloat<FT>::type valuesfa{std::bit_cast<typename FloatToUint<FT>::type>(valuefa)};
  FT valuefb{float_rng.Gen()};
  typename FFloatToSFloat<FT>::type valuesfb{std::bit_cast<typename FloatToUint<FT>::type>(valuefb)};
  FT valuefc{float_rng.Gen()};
  typename FFloatToSFloat<FT>::type valuesfc{std::bit_cast<typename FloatToUint<FT>::type>(valuefc)};

  for (i32 i = 0; i < kNumIterations; ++i) {
    if constexpr (num_args == 1) {
      auto ff_result = ff_func(valuefa);
      auto sf_result = sf_func(valuesfa);
      auto ff_result_u = ToComparableType(ff_result);
      auto sf_result_u = ToComparableType(sf_result);
      CheckResult(ff_result_u, sf_result_u, i);
    }
    if constexpr (num_args == 2) {
      auto ff_result = ff_func(valuefa, valuefb);
      auto sf_result = sf_func(valuesfa, valuesfb);
      auto ff_result_u = ToComparableType(ff_result);
      auto sf_result_u = ToComparableType(sf_result);
      CheckResult(ff_result_u, sf_result_u, i);
    }
    if constexpr (num_args == 3) {
      auto ff_result = ff_func(valuefa, valuefb, valuefc);
      auto sf_result = sf_func(valuesfa, valuesfb, valuesfc);
      auto ff_result_u = ToComparableType(ff_result);
      auto sf_result_u = ToComparableType(sf_result);
      CheckResult(ff_result_u, sf_result_u, i);
    }

    ::softfloat_exceptionFlags = 0;
    ff.ClearFlags();

    valuesfc = valuesfb;
    valuefc = valuefb;
    valuesfb = valuesfa;
    valuefb = valuefa;
    valuefa = float_rng.Gen();
    valuesfa.v = std::bit_cast<typename FloatToUint<FT>::type>(valuefa);
  }
}

TEST(SoftFloatRiscvTests, Addf16RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  ff.rounding_mode = FloppyFloat::kRoundTiesToEven;
  auto ff_func = std::bind(&FloppyFloat::Add<f16>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_add, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf16RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  ff.rounding_mode = FloppyFloat::kRoundTowardPositive;
  auto ff_func = std::bind(&FloppyFloat::Add<f16>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_add, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf16RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Add<f16, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_add, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf16RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Add<f16, FloppyFloat::kRoundTowardZero>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_add, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf16RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::Add<f16, FloppyFloat::kRoundTiesToAway>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_add, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf16RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Sub<f16, FloppyFloat::kRoundTiesToEven>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_sub, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf16RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Sub<f16, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_sub, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf16RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Sub<f16, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_sub, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf16RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Sub<f16, FloppyFloat::kRoundTowardZero>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_sub, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf16RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::Sub<f16, FloppyFloat::kRoundTiesToAway>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_sub, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf16RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Mul<f16, FloppyFloat::kRoundTiesToEven>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_mul, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf16RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Mul<f16, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_mul, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf16RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Mul<f16, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_mul, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf16RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Mul<f16, FloppyFloat::kRoundTowardZero>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_mul, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf16RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Div<f16, FloppyFloat::kRoundTiesToEven>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_div, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf16RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Div<f16, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_div, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf16RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Div<f16, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_div, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf16RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Div<f16, FloppyFloat::kRoundTowardZero>, &ff, _1, _2);
  auto sf_func = std::bind(&::f16_div, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}


TEST(SoftFloatRiscvTests, Sqrtf16RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Sqrt<f16, FloppyFloat::kRoundTiesToEven>, &ff, _1);
  auto sf_func = std::bind(&::f16_sqrt, _1);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf16RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Sqrt<f16, FloppyFloat::kRoundTowardPositive>, &ff, _1);
  auto sf_func = std::bind(&::f16_sqrt, _1);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf16RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Sqrt<f16, FloppyFloat::kRoundTowardNegative>, &ff, _1);
  auto sf_func = std::bind(&::f16_sqrt, _1);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf16RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Sqrt<f16, FloppyFloat::kRoundTowardZero>, &ff, _1);
  auto sf_func = std::bind(&::f16_sqrt, _1);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf16RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Fma<f16, FloppyFloat::kRoundTiesToEven>, &ff, _1, _2, _3);
  auto sf_func = std::bind(&::f16_mulAdd, _1, _2, _3);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf16RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Fma<f16, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2, _3);
  auto sf_func = std::bind(&::f16_mulAdd, _1, _2, _3);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf16RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Fma<f16, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2, _3);
  auto sf_func = std::bind(&::f16_mulAdd, _1, _2, _3);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf16RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Fma<f16, FloppyFloat::kRoundTowardZero>, &ff, _1, _2, _3);
  auto sf_func = std::bind(&::f16_mulAdd, _1, _2, _3);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

// F32

TEST(SoftFloatRiscvTests, Addf32RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  ff.rounding_mode = FloppyFloat::kRoundTiesToEven;
  auto ff_func = std::bind(&FloppyFloat::Add<f32>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_add, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf32RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Add<f32, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_add, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf32RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Add<f32, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_add, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf32RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Add<f32, FloppyFloat::kRoundTowardZero>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_add, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf32RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::Add<f32, FloppyFloat::kRoundTiesToAway>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_add, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf32RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Sub<f32, FloppyFloat::kRoundTiesToEven>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_sub, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf32RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Sub<f32, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_sub, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf32RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Sub<f32, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_sub, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf32RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Sub<f32, FloppyFloat::kRoundTowardZero>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_sub, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf32RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::Sub<f32, FloppyFloat::kRoundTiesToAway>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_sub, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf32RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Mul<f32, FloppyFloat::kRoundTiesToEven>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_mul, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf32RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Mul<f32, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_mul, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf32RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Mul<f32, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_mul, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf32RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Mul<f32, FloppyFloat::kRoundTowardZero>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_mul, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf32RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Div<f32, FloppyFloat::kRoundTiesToEven>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_div, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf32RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Div<f32, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_div, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf32RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Div<f32, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_div, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf32RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Div<f32, FloppyFloat::kRoundTowardZero>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_div, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf32RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Sqrt<f32, FloppyFloat::kRoundTiesToEven>, &ff, _1);
  auto sf_func = std::bind(&::f32_sqrt, _1);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf32RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Sqrt<f32, FloppyFloat::kRoundTowardPositive>, &ff, _1);
  auto sf_func = std::bind(&::f32_sqrt, _1);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf32RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Sqrt<f32, FloppyFloat::kRoundTowardNegative>, &ff, _1);
  auto sf_func = std::bind(&::f32_sqrt, _1);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf32RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Sqrt<f32, FloppyFloat::kRoundTowardZero>, &ff, _1);
  auto sf_func = std::bind(&::f32_sqrt, _1);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf32RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Fma<f32, FloppyFloat::kRoundTiesToEven>, &ff, _1, _2, _3);
  auto sf_func = std::bind(&::f32_mulAdd, _1, _2, _3);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf32RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Fma<f32, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2, _3);
  auto sf_func = std::bind(&::f32_mulAdd, _1, _2, _3);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf32RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Fma<f32, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2, _3);
  auto sf_func = std::bind(&::f32_mulAdd, _1, _2, _3);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf32RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Fma<f32, FloppyFloat::kRoundTowardZero>, &ff, _1, _2, _3);
  auto sf_func = std::bind(&::f32_mulAdd, _1, _2, _3);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToI32RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::F32ToI32<FloppyFloat::kRoundTiesToEven>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_i32, _1, ::softfloat_round_near_even, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToI32RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::F32ToI32<FloppyFloat::kRoundTowardPositive>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_i32, _1, ::softfloat_round_max, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToI32RoundMin) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::F32ToI32<FloppyFloat::kRoundTowardNegative>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_i32, _1, ::softfloat_round_min, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToI32RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::F32ToI32<FloppyFloat::kRoundTiesToAway>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_i32, _1, ::softfloat_round_near_maxMag, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToI32RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::F32ToI32<FloppyFloat::kRoundTowardZero>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_i32, _1, ::softfloat_round_minMag, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToU32RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::F32ToU32<FloppyFloat::kRoundTiesToEven>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_ui32, _1, ::softfloat_round_near_even, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToU32RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::F32ToU32<FloppyFloat::kRoundTowardPositive>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_ui32, _1, ::softfloat_round_max, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToU32RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::F32ToU32<FloppyFloat::kRoundTowardNegative>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_ui32, _1, ::softfloat_round_min, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToU32RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::F32ToU32<FloppyFloat::kRoundTowardZero>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_ui32, _1, ::softfloat_round_minMag, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToU32RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::F32ToU32<FloppyFloat::kRoundTiesToAway>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_ui32, _1, ::softfloat_round_near_maxMag, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToI64RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::F32ToI64<FloppyFloat::kRoundTiesToEven>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_i64, _1, ::softfloat_round_near_even, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToI64RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::F32ToI64<FloppyFloat::kRoundTowardPositive>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_i64, _1, ::softfloat_round_max, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToI64RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::F32ToI64<FloppyFloat::kRoundTowardNegative>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_i64, _1, ::softfloat_round_min, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToI64RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::F32ToI64<FloppyFloat::kRoundTowardZero>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_i64, _1, ::softfloat_round_minMag, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToI64RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::F32ToI64<FloppyFloat::kRoundTiesToAway>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_i64, _1, ::softfloat_round_near_maxMag, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToU64RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::F32ToU64<FloppyFloat::kRoundTiesToEven>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_ui64, _1, ::softfloat_round_near_even, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToU64RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::F32ToU64<FloppyFloat::kRoundTowardPositive>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_ui64, _1, ::softfloat_round_max, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToU64RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::F32ToU64<FloppyFloat::kRoundTowardNegative>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_ui64, _1, ::softfloat_round_min, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToU64RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::F32ToU64<FloppyFloat::kRoundTowardZero>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_ui64, _1, ::softfloat_round_minMag, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToU64RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::F32ToU64<FloppyFloat::kRoundTiesToAway>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_ui64, _1, ::softfloat_round_near_maxMag, true);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToF16RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::F32ToF16<FloppyFloat::kRoundTiesToEven>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_f16, _1);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToF16RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::F32ToF16<FloppyFloat::kRoundTowardPositive>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_f16, _1);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToF16RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::F32ToF16<FloppyFloat::kRoundTowardNegative>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_f16, _1);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToF16RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::F32ToF16<FloppyFloat::kRoundTowardZero>, &ff, _1);
  auto sf_func = std::bind(&::f32_to_f16, _1);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

// TEST(SoftFloatRiscvTests, F32ToF16RoundMag) {
//   ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
//   auto ff_func = std::bind(&FloppyFloat::F32ToF16<FloppyFloat::kRoundTiesToAway>, &ff, _1);
//   auto sf_func = std::bind(&::f32_to_f16, _1);
//   DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
// }

TEST(SoftFloatRiscvTests, F32ToF64) {
  auto ff_func = std::bind(&FloppyFloat::F32ToF64, &ff, _1);
  auto sf_func = std::bind(&::f32_to_f64, _1);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Eqf32) {
  auto ff_func = std::bind(&FloppyFloat::Eq<f32, true>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_eq, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Lef32) {
  auto ff_func = std::bind(&FloppyFloat::Le<f32, false>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_le, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Lt32) {
  auto ff_func = std::bind(&FloppyFloat::Lt<f32, false>, &ff, _1, _2);
  auto sf_func = std::bind(&::f32_lt, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

// F64

TEST(SoftFloatRiscvTests, Addf64RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  ff.rounding_mode = FloppyFloat::kRoundTiesToEven;
  auto ff_func = std::bind(&FloppyFloat::Add<f64>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_add, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf64RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Add<f64, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_add, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf64RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Add<f64, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_add, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf64RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Add<f64, FloppyFloat::kRoundTowardZero>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_add, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf64RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::Add<f64, FloppyFloat::kRoundTiesToAway>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_add, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf64RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Sub<f64,FloppyFloat::kRoundTiesToEven>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_sub, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf64RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Sub<f64, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_sub, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf64RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Sub<f64, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_sub, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf64RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Sub<f64, FloppyFloat::kRoundTowardZero>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_sub, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf64RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::Sub<f64, FloppyFloat::kRoundTiesToAway>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_sub, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf64RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Mul<f64, FloppyFloat::kRoundTiesToEven>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_mul, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf64RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Mul<f64, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_mul, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf64RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Mul<f64, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_mul, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf64RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Mul<f64, FloppyFloat::kRoundTowardZero>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_mul, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf64RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Div<f64, FloppyFloat::kRoundTiesToEven>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_div, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf64RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Div<f64, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_div, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf64RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Div<f64, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_div, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf64RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Div<f64, FloppyFloat::kRoundTowardZero>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_div, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf64RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Sqrt<f64, FloppyFloat::kRoundTiesToEven>, &ff, _1);
  auto sf_func = std::bind(&::f64_sqrt, _1);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf64RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Sqrt<f64, FloppyFloat::kRoundTowardPositive>, &ff, _1);
  auto sf_func = std::bind(&::f64_sqrt, _1);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf64RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Sqrt<f64, FloppyFloat::kRoundTowardNegative>, &ff, _1);
  auto sf_func = std::bind(&::f64_sqrt, _1);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf64RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Sqrt<f64, FloppyFloat::kRoundTowardZero>, &ff, _1);
  auto sf_func = std::bind(&::f64_sqrt, _1);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf64RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::Fma<f64, FloppyFloat::kRoundTiesToEven>, &ff, _1, _2, _3);
  auto sf_func = std::bind(&::f64_mulAdd, _1, _2, _3);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf64RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::Fma<f64, FloppyFloat::kRoundTowardPositive>, &ff, _1, _2, _3);
  auto sf_func = std::bind(&::f64_mulAdd, _1, _2, _3);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf64RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::Fma<f64, FloppyFloat::kRoundTowardNegative>, &ff, _1, _2, _3);
  auto sf_func = std::bind(&::f64_mulAdd, _1, _2, _3);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf64RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::Fma<f64, FloppyFloat::kRoundTowardZero>, &ff, _1, _2, _3);
  auto sf_func = std::bind(&::f64_mulAdd, _1, _2, _3);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, f64ToI32RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::F64ToI32<FloppyFloat::kRoundTiesToEven>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_i32, _1, ::softfloat_round_near_even, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, f64ToI32RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::F64ToI32<FloppyFloat::kRoundTowardPositive>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_i32, _1, ::softfloat_round_max, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, f64ToI32RoundMin) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::F64ToI32<FloppyFloat::kRoundTowardNegative>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_i32, _1, ::softfloat_round_min, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToI32RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::F64ToI32<FloppyFloat::kRoundTiesToAway>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_i32, _1, ::softfloat_round_near_maxMag, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToI64RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::F64ToI64<FloppyFloat::kRoundTowardZero>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_i64, _1, ::softfloat_round_minMag, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToI64RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::F64ToI64<FloppyFloat::kRoundTiesToEven>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_i64, _1, ::softfloat_round_near_even, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToI64RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::F64ToI64<FloppyFloat::kRoundTowardPositive>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_i64, _1, ::softfloat_round_max, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToI64RoundMin) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::F64ToI64<FloppyFloat::kRoundTowardNegative>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_i64, _1, ::softfloat_round_min, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToI64RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::F64ToI64<FloppyFloat::kRoundTiesToAway>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_i64, _1, ::softfloat_round_near_maxMag, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToU32RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::F64ToU32<FloppyFloat::kRoundTiesToEven>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_ui32, _1, ::softfloat_round_near_even, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToU32RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::F64ToU32<FloppyFloat::kRoundTowardPositive>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_ui32, _1, ::softfloat_round_max, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToU32RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::F64ToU32<FloppyFloat::kRoundTowardNegative>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_ui32, _1, ::softfloat_round_min, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToU32RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::F64ToU32<FloppyFloat::kRoundTowardZero>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_ui32, _1, ::softfloat_round_minMag, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToU32RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::F64ToU32<FloppyFloat::kRoundTiesToAway>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_ui32, _1, ::softfloat_round_near_maxMag, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToU64RoundNear) {
  ::softfloat_roundingMode = ::softfloat_round_near_even;
  auto ff_func = std::bind(&FloppyFloat::F64ToU64<FloppyFloat::kRoundTiesToEven>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_ui64, _1, ::softfloat_round_near_even, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToU64RoundPos) {
  ::softfloat_roundingMode = ::softfloat_round_max;
  auto ff_func = std::bind(&FloppyFloat::F64ToU64<FloppyFloat::kRoundTowardPositive>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_ui64, _1, ::softfloat_round_max, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToU64RoundNeg) {
  ::softfloat_roundingMode = ::softfloat_round_min;
  auto ff_func = std::bind(&FloppyFloat::F64ToU64<FloppyFloat::kRoundTowardNegative>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_ui64, _1, ::softfloat_round_min, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToU64RoundZero) {
  ::softfloat_roundingMode = ::softfloat_round_minMag;
  auto ff_func = std::bind(&FloppyFloat::F64ToU64<FloppyFloat::kRoundTowardZero>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_ui64, _1, ::softfloat_round_minMag, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToU64RoundMag) {
  ::softfloat_roundingMode = ::softfloat_round_near_maxMag;
  auto ff_func = std::bind(&FloppyFloat::F64ToU64<FloppyFloat::kRoundTiesToAway>, &ff, _1);
  auto sf_func = std::bind(&::f64_to_ui64, _1, ::softfloat_round_near_maxMag, true);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Eqf64) {
  auto ff_func = std::bind(&FloppyFloat::Eq<f64, true>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_eq, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Lef64) {
  auto ff_func = std::bind(&FloppyFloat::Le<f64, false>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_le, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Ltf64) {
  auto ff_func = std::bind(&FloppyFloat::Lt<f64, false>, &ff, _1, _2);
  auto sf_func = std::bind(&::f64_lt, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, I32ToF64) {
  ::softfloat_exceptionFlags = 0;
  ff.ClearFlags();
  CheckResult(ToComparableType(ff.I32ToF64(0)), ToComparableType(::i32_to_f64(0)), 0);
  CheckResult(ToComparableType(ff.I32ToF64(5)), ToComparableType(::i32_to_f64(5)), 1);
  CheckResult(ToComparableType(ff.I32ToF64(-5)), ToComparableType(::i32_to_f64(-5)), 2);
  CheckResult(ToComparableType(ff.I32ToF64(1024)), ToComparableType(::i32_to_f64(1024)), 3);
  CheckResult(ToComparableType(ff.I32ToF64(-1024)), ToComparableType(::i32_to_f64(-1024)), 4);
  CheckResult(ToComparableType(ff.I32ToF64(2147483646)), ToComparableType(::i32_to_f64(2147483646)), 5);
  CheckResult(ToComparableType(ff.I32ToF64(-2147483646)), ToComparableType(::i32_to_f64(-2147483646)), 6);
  CheckResult(ToComparableType(ff.I32ToF64(2147483647)), ToComparableType(::i32_to_f64(2147483647)), 7);
  CheckResult(ToComparableType(ff.I32ToF64(-2147483647)), ToComparableType(::i32_to_f64(-2147483647)), 8);
}

TEST(SoftFloatRiscvTests, I32ToF32) {
  ::softfloat_exceptionFlags = 0;
  ff.ClearFlags();
  for (auto rm : rms) {
    ff.rounding_mode = rm.first;
    ::softfloat_roundingMode = rm.second;
    CheckResult(ToComparableType(ff.I32ToF32(0)), ToComparableType(::i32_to_f32(0)), 0);
    CheckResult(ToComparableType(ff.I32ToF32(1)), ToComparableType(::i32_to_f32(1)), 1);
    CheckResult(ToComparableType(ff.I32ToF32(-1)), ToComparableType(::i32_to_f32(-1)), 2);
    CheckResult(ToComparableType(ff.I32ToF32(5)), ToComparableType(::i32_to_f32(5)), 3);
    CheckResult(ToComparableType(ff.I32ToF32(-5)), ToComparableType(::i32_to_f32(-5)), 4);
    CheckResult(ToComparableType(ff.I32ToF32(1024)), ToComparableType(::i32_to_f32(1024)), 5);
    CheckResult(ToComparableType(ff.I32ToF32(-1024)), ToComparableType(::i32_to_f32(-1024)), 6);
    CheckResult(ToComparableType(ff.I32ToF32(33554432)), ToComparableType(::i32_to_f32(33554432)), 7);
    CheckResult(ToComparableType(ff.I32ToF32(33554433)), ToComparableType(::i32_to_f32(33554433)), 8);
    CheckResult(ToComparableType(ff.I32ToF32(33554434)), ToComparableType(::i32_to_f32(33554434)), 9);
    CheckResult(ToComparableType(ff.I32ToF32(33554435)), ToComparableType(::i32_to_f32(33554435)), 10);
    CheckResult(ToComparableType(ff.I32ToF32(33554436)), ToComparableType(::i32_to_f32(33554436)), 11);
    CheckResult(ToComparableType(ff.I32ToF32(2147483646)), ToComparableType(::i32_to_f32(2147483646)), 12);
    CheckResult(ToComparableType(ff.I32ToF32(-2147483646)), ToComparableType(::i32_to_f32(-2147483646)), 13);
    CheckResult(ToComparableType(ff.I32ToF32(2147483647)), ToComparableType(::i32_to_f32(2147483647)), 14);
    CheckResult(ToComparableType(ff.I32ToF32(-2147483647)), ToComparableType(::i32_to_f32(-2147483647)), 15);
  }
}

TEST(SoftFloatRiscvTests, U32ToF32) {
  ::softfloat_exceptionFlags = 0;
  ff.ClearFlags();
  for (auto rm : rms) {
    ff.rounding_mode = rm.first;
    ::softfloat_roundingMode = rm.second;
    CheckResult(ToComparableType(ff.U32ToF32(0u)), ToComparableType(::ui32_to_f32(0u)), 0);
    CheckResult(ToComparableType(ff.U32ToF32(5u)), ToComparableType(::ui32_to_f32(5u)), 1);
    CheckResult(ToComparableType(ff.U32ToF32(1024u)), ToComparableType(::ui32_to_f32(1024u)), 2);
    CheckResult(ToComparableType(ff.U32ToF32(33554432u)), ToComparableType(::ui32_to_f32(33554432u)), 3);
    CheckResult(ToComparableType(ff.U32ToF32(33554433u)), ToComparableType(::ui32_to_f32(33554433u)), 4);
    CheckResult(ToComparableType(ff.U32ToF32(33554434u)), ToComparableType(::ui32_to_f32(33554434u)), 5);
    CheckResult(ToComparableType(ff.U32ToF32(33554435u)), ToComparableType(::ui32_to_f32(33554435u)), 6);
    CheckResult(ToComparableType(ff.U32ToF32(33554436u)), ToComparableType(::ui32_to_f32(33554436u)), 7);
    CheckResult(ToComparableType(ff.U32ToF32(2147483646u)), ToComparableType(::ui32_to_f32(2147483646u)), 8);
    CheckResult(ToComparableType(ff.U32ToF32(2147483647u)), ToComparableType(::ui32_to_f32(2147483647u)), 9);
    CheckResult(ToComparableType(ff.U32ToF32(4294967294u)), ToComparableType(::ui32_to_f32(4294967294u)), 10);
    CheckResult(ToComparableType(ff.U32ToF32(4294967295u)), ToComparableType(::ui32_to_f32(4294967295u)), 11);
  }
}

TEST(SoftFloatRiscvTests, U32ToF64) {
  ::softfloat_exceptionFlags = 0;
  ff.ClearFlags();
  CheckResult(ToComparableType(ff.U32ToF64(0)), ToComparableType(::ui32_to_f64(0)), 0);
  CheckResult(ToComparableType(ff.U32ToF64(5)), ToComparableType(::ui32_to_f64(5)), 1);
  CheckResult(ToComparableType(ff.U32ToF64(1024)), ToComparableType(::ui32_to_f64(1024)), 2);
  CheckResult(ToComparableType(ff.U32ToF64(2147483646)), ToComparableType(::ui32_to_f64(2147483646)), 3);
  CheckResult(ToComparableType(ff.U32ToF64(2147483647)), ToComparableType(::ui32_to_f64(2147483647)), 4);
  CheckResult(ToComparableType(ff.U32ToF64(4294967294)), ToComparableType(::ui32_to_f64(4294967294)), 5);
  CheckResult(ToComparableType(ff.U32ToF64(4294967295)), ToComparableType(::ui32_to_f64(4294967295)), 6);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
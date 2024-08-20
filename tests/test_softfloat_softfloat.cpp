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
#include <utility>

#include "float_rng.h"
#include "soft_float.h"

extern "C" {
#include "softfloat/softfloat.h"
}

using namespace std::placeholders;
using namespace FfUtils;

constexpr i32 kNumIterations = 2000;
constexpr i32 kRngSeed = 42;

SoftFloat sf;

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
  ASSERT_EQ(sf.invalid, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_invalid)) << "Iteration: " << i;
  ASSERT_EQ(sf.division_by_zero, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_infinite))
      << "Iteration: " << i;
  ASSERT_EQ(sf.overflow, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_overflow))
      << "Iteration: " << i;
  ASSERT_EQ(sf.underflow, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_underflow))
      << "Iteration: " << i;
  ASSERT_EQ(sf.inexact, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_inexact)) << "Iteration: " << i;
}

std::array<std::pair<uint_fast8_t, SoftFloat::RoundingMode>, 5> rounding_modes{
    {{::softfloat_round_near_even, SoftFloat::RoundingMode::kRoundTiesToEven},
     {::softfloat_round_near_maxMag, SoftFloat::RoundingMode::kRoundTiesToAway},
     {::softfloat_round_min, SoftFloat::RoundingMode::kRoundTowardNegative},
     {::softfloat_round_max, SoftFloat::RoundingMode::kRoundTowardPositive},
     {::softfloat_round_minMag, SoftFloat::RoundingMode::kRoundTowardZero}}};

template <typename FT, typename FFFUNC, typename SFFUNC, int num_args>
void DoTest(FFFUNC ff_func, SFFUNC sf_func) {
  sf.SetupToRiscv();

  ::softfloat_exceptionFlags = 0;
  sf.ClearFlags();

  FloatRng<FT> float_rng(kRngSeed);
  FT valuefa{float_rng.Gen()};
  typename FFloatToSFloat<FT>::type valuesfa{std::bit_cast<typename FloatToUint<FT>::type>(valuefa)};
  FT valuefb{float_rng.Gen()};
  typename FFloatToSFloat<FT>::type valuesfb{std::bit_cast<typename FloatToUint<FT>::type>(valuefb)};
  FT valuefc{float_rng.Gen()};
  typename FFloatToSFloat<FT>::type valuesfc{std::bit_cast<typename FloatToUint<FT>::type>(valuefc)};

  for (auto rm : rounding_modes) {
    ::softfloat_roundingMode = rm.first;
    sf.rounding_mode = rm.second;
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
      sf.ClearFlags();

      valuesfc = valuesfb;
      valuefc = valuefb;
      valuesfb = valuesfa;
      valuefb = valuefa;
      valuefa = float_rng.Gen();
      valuesfa.v = std::bit_cast<typename FloatToUint<FT>::type>(valuefa);
    }
  }
}

TEST(SoftFloatRiscvTests, Addf16) {
  auto ff_func = std::bind(&SoftFloat::Add<f16>, &sf, _1, _2);
  auto sf_func = std::bind(&::f16_add, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf32) {
  auto ff_func = std::bind(&SoftFloat::Add<f32>, &sf, _1, _2);
  auto sf_func = std::bind(&::f32_add, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Addf64) {
  auto ff_func = std::bind(&SoftFloat::Add<f64>, &sf, _1, _2);
  auto sf_func = std::bind(&::f64_add, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf16) {
  auto ff_func = std::bind(&SoftFloat::Sub<f16>, &sf, _1, _2);
  auto sf_func = std::bind(&::f16_sub, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf32) {
  auto ff_func = std::bind(&SoftFloat::Sub<f32>, &sf, _1, _2);
  auto sf_func = std::bind(&::f32_sub, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Subf64) {
  auto ff_func = std::bind(&SoftFloat::Sub<f64>, &sf, _1, _2);
  auto sf_func = std::bind(&::f64_sub, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf16) {
  auto ff_func = std::bind(&SoftFloat::Mul<f16>, &sf, _1, _2);
  auto sf_func = std::bind(&::f16_mul, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf32) {
  auto ff_func = std::bind(&SoftFloat::Mul<f32>, &sf, _1, _2);
  auto sf_func = std::bind(&::f32_mul, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Mulf64) {
  auto ff_func = std::bind(&SoftFloat::Mul<f64>, &sf, _1, _2);
  auto sf_func = std::bind(&::f64_mul, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf16) {
  auto ff_func = std::bind(&SoftFloat::Div<f16>, &sf, _1, _2);
  auto sf_func = std::bind(&::f16_div, _1, _2);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf32) {
  auto ff_func = std::bind(&SoftFloat::Div<f32>, &sf, _1, _2);
  auto sf_func = std::bind(&::f32_div, _1, _2);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Divf64) {
  auto ff_func = std::bind(&SoftFloat::Div<f64>, &sf, _1, _2);
  auto sf_func = std::bind(&::f64_div, _1, _2);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 2>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf16) {
  auto ff_func = std::bind(&SoftFloat::Sqrt<f16>, &sf, _1);
  auto sf_func = std::bind(&::f16_sqrt, _1);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf32) {
  auto ff_func = std::bind(&SoftFloat::Sqrt<f32>, &sf, _1);
  auto sf_func = std::bind(&::f32_sqrt, _1);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Sqrtf64) {
  auto ff_func = std::bind(&SoftFloat::Sqrt<f64>, &sf, _1);
  auto sf_func = std::bind(&::f64_sqrt, _1);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf16) {
  auto ff_func = std::bind(&SoftFloat::Fma<f16>, &sf, _1, _2, _3);
  auto sf_func = std::bind(&::f16_mulAdd, _1, _2, _3);
  DoTest<f16, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf32) {
  auto ff_func = std::bind(&SoftFloat::Fma<f32>, &sf, _1, _2, _3);
  auto sf_func = std::bind(&::f32_mulAdd, _1, _2, _3);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, Fmaf64) {
  auto ff_func = std::bind(&SoftFloat::Fma<f64>, &sf, _1, _2, _3);
  auto sf_func = std::bind(&::f64_mulAdd, _1, _2, _3);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 3>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F32ToF16) {
  auto ff_func = std::bind(&SoftFloat::F32ToF16, &sf, _1);
  auto sf_func = std::bind(&::f32_to_f16, _1);
  DoTest<f32, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64Tof16) {
  auto ff_func = std::bind(&SoftFloat::F64ToF16, &sf, _1);
  auto sf_func = std::bind(&::f64_to_f16, _1);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

TEST(SoftFloatRiscvTests, F64ToF32) {
  auto ff_func = std::bind(&SoftFloat::F64ToF32, &sf, _1);
  auto sf_func = std::bind(&::f64_to_f32, _1);
  DoTest<f64, decltype(ff_func), decltype(sf_func), 1>(ff_func, sf_func);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
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

//constexpr i32 kNumIterations = 200000;
constexpr i32 kNumIterations = 10000;
constexpr i32 kRngSeed = 42;

FloppyFloat ff;

std::array<std::pair<uint_fast8_t, SoftFloat::RoundingMode>, 5> rounding_modes{
    {{::softfloat_round_near_even, SoftFloat::RoundingMode::kRoundTiesToEven},
     {::softfloat_round_near_maxMag, SoftFloat::RoundingMode::kRoundTiesToAway},
     {::softfloat_round_max, SoftFloat::RoundingMode::kRoundTowardPositive},
     {::softfloat_round_min, SoftFloat::RoundingMode::kRoundTowardNegative},
     {::softfloat_round_minMag, SoftFloat::RoundingMode::kRoundTowardZero}}};

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
  ASSERT_EQ(ff_result_u, sf_result_u)
    << "Iteration: " << i << ", FF result:" << ff_result_u << ", SF result:" << sf_result_u;
  ASSERT_EQ(ff.invalid, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_invalid))
    << "Iteration: " << i << ", FF result:" << ff_result_u << ", SF result:" << sf_result_u;
  ASSERT_EQ(ff.division_by_zero, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_infinite))
    << "Iteration: " << i << ", FF result:" << ff_result_u << ", SF result:" << sf_result_u;
  ASSERT_EQ(ff.overflow, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_overflow))
    << "Iteration: " << i << ", FF result:" << ff_result_u << ", SF result:" << sf_result_u;
  ASSERT_EQ(ff.underflow, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_underflow))
    << "Iteration: " << i << ", FF result:" << ff_result_u << ", SF result:" << sf_result_u;
  ASSERT_EQ(ff.inexact, static_cast<bool>(::softfloat_exceptionFlags & ::softfloat_flag_inexact))
    << "Iteration: " << i << ", FF result:" << ff_result_u << ", SF result:" << sf_result_u;
}

template <typename FT, typename FFFUNC, typename SFFUNC, int num_args>
void DoTest(FFFUNC ff_func, SFFUNC sf_func) {

#if defined(ARCH_RISCV)
  ff.SetupToRiscv();
#elif defined(ARCH_X86)
  ff.SetupToX86();
#elif defined(ARCH_ARM)
  ff.SetupToArm();
#endif

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

#if defined(ARCH_RISCV)
  #define TEST_SUITE_NAME SoftFloatFloppyFloatRiscvTests
#elif defined(ARCH_X86)
  #define TEST_SUITE_NAME SoftFloatFloppyFloatX86Tests
#elif defined(ARCH_ARM)
  #define TEST_SUITE_NAME SoftFloatFloppyFloatArmTests
#else
  static_assert(false, "Unknown architecture");
#endif

#define TEST_MACRO_BASE(name, ff_op, sf_op, type, rm, rm_name, nargs, ...)       \
  TEST(TEST_SUITE_NAME, name##rm_name) {                                         \
    ::softfloat_roundingMode = rounding_modes[rm].first;                         \
    ff.rounding_mode = rounding_modes[rm].second;                                \
    auto ff_func = std::bind(ff_op, &ff, __VA_ARGS__);                           \
    auto sf_func = std::bind(&::sf_op, __VA_ARGS__);                             \
    DoTest<type, decltype(ff_func), decltype(sf_func), nargs>(ff_func, sf_func); \
  }

#define TEST_MACRO_BASE_FTOI(name, ff_op, sf_op, type, rm, rm_name, nargs, ...)      \
  TEST(TEST_SUITE_NAME, name##rm_name) {                                             \
    ::softfloat_roundingMode = rounding_modes[rm].first;                             \
    ff.rounding_mode = rounding_modes[rm].second;                                    \
    auto ff_func = std::bind(ff_op, &ff, __VA_ARGS__);                               \
    auto sf_func = std::bind(&::sf_op, __VA_ARGS__, ::softfloat_roundingMode, true); \
    DoTest<type, decltype(ff_func), decltype(sf_func), nargs>(ff_func, sf_func);     \
  }

#define TEST_MACRO_1(name, ff_op, sf_op, type, rm, rm_name) TEST_MACRO_BASE(name, ff_op, sf_op, type, rm, rm_name, 1, _1)
#define TEST_MACRO_2(name, ff_op, sf_op, type, rm, rm_name) TEST_MACRO_BASE(name, ff_op, sf_op, type, rm, rm_name, 2, _1, _2)
#define TEST_MACRO_3(name, ff_op, sf_op, type, rm, rm_name) TEST_MACRO_BASE(name, ff_op, sf_op, type, rm, rm_name, 3, _1, _2, _3)
#define TEST_MACRO_FTOI(name, ff_op, sf_op, type, rm, rm_name) TEST_MACRO_BASE_FTOI(name, ff_op, sf_op, type, rm, rm_name, 1, _1)

#define TEST_MACRO_ITOF(name, ff_op, sf_op, type, rm, rm_name)                                                        \
  TEST(TEST_SUITE_NAME, name##rm_name) {                                                                              \
    ::softfloat_exceptionFlags = 0;                                                                                   \
    ff.ClearFlags();                                                                                                  \
    ::softfloat_roundingMode = rounding_modes[rm].first;                                                              \
    ff.rounding_mode = rounding_modes[rm].second;                                                                     \
    CheckResult(ToComparableType(ff.ff_op((type)0)), ToComparableType(::sf_op((type)0)), 0);                          \
    CheckResult(ToComparableType(ff.ff_op((type)1)), ToComparableType(::sf_op((type)1)), 1);                          \
    CheckResult(ToComparableType(ff.ff_op((type) - 1)), ToComparableType(::sf_op((type)-1)), 2);                      \
    CheckResult(ToComparableType(ff.ff_op((type)5)), ToComparableType(::sf_op((type)5)), 3);                          \
    CheckResult(ToComparableType(ff.ff_op((type) - 5)), ToComparableType(::sf_op((type)-5)), 4);                      \
    CheckResult(ToComparableType(ff.ff_op((type)1024)), ToComparableType(::sf_op((type)1024)), 5);                    \
    CheckResult(ToComparableType(ff.ff_op((type) - 1024)), ToComparableType(::sf_op(-(type)1024)), 6);                \
    CheckResult(ToComparableType(ff.ff_op((type)33554432)), ToComparableType(::sf_op((type)33554432)), 7);            \
    CheckResult(ToComparableType(ff.ff_op((type)33554433)), ToComparableType(::sf_op((type)33554433)), 8);            \
    CheckResult(ToComparableType(ff.ff_op((type)33554434)), ToComparableType(::sf_op((type)33554434)), 9);            \
    CheckResult(ToComparableType(ff.ff_op((type)33554435)), ToComparableType(::sf_op((type)33554435)), 10);           \
    CheckResult(ToComparableType(ff.ff_op((type)33554436)), ToComparableType(::sf_op((type)33554436)), 11);           \
    CheckResult(ToComparableType(ff.ff_op((type)2147483646)), ToComparableType(::sf_op((type)2147483646)), 12);       \
    CheckResult(ToComparableType(ff.ff_op((type) - 2147483646)), ToComparableType(::sf_op((type) - 2147483646)), 13); \
    CheckResult(ToComparableType(ff.ff_op((type)2147483647)), ToComparableType(::sf_op((type)2147483647)), 14);       \
    CheckResult(ToComparableType(ff.ff_op((type) - 2147483647)), ToComparableType(::sf_op((type) - 2147483647)), 15); \
    CheckResult(ToComparableType(ff.ff_op((type)4294967294u)), ToComparableType(::sf_op((type)4294967294u)), 16);     \
    CheckResult(ToComparableType(ff.ff_op((type)4294967295u)), ToComparableType(::sf_op((type) 4294967295u)), 17);    \
    CheckResult(ToComparableType(ff.ff_op((type)4294967294u)), ToComparableType(::sf_op((type)4294967294u)), 18);     \
  }

TEST_MACRO_2(Addf16, &FloppyFloat::Add<f16>, f16_add, f16, 0, RoundTiesToEven)
TEST_MACRO_2(Addf16, &FloppyFloat::Add<f16>, f16_add, f16, 1, RoundTiesToAway)
TEST_MACRO_2(Addf16, &FloppyFloat::Add<f16>, f16_add, f16, 2, RoundTowardPositive)
TEST_MACRO_2(Addf16, &FloppyFloat::Add<f16>, f16_add, f16, 3, RoundTowardNegative)
TEST_MACRO_2(Addf16, &FloppyFloat::Add<f16>, f16_add, f16, 4, RoundTowardZero)
TEST_MACRO_2(Addf32, &FloppyFloat::Add<f32>, f32_add, f32, 0, RoundTiesToEven)
TEST_MACRO_2(Addf32, &FloppyFloat::Add<f32>, f32_add, f32, 1, RoundTiesToAway)
TEST_MACRO_2(Addf32, &FloppyFloat::Add<f32>, f32_add, f32, 2, RoundTowardPositive)
TEST_MACRO_2(Addf32, &FloppyFloat::Add<f32>, f32_add, f32, 3, RoundTowardNegative)
TEST_MACRO_2(Addf32, &FloppyFloat::Add<f32>, f32_add, f32, 4, RoundTowardZero)
TEST_MACRO_2(Addf64, &FloppyFloat::Add<f64>, f64_add, f64, 0, RoundTiesToEven)
TEST_MACRO_2(Addf64, &FloppyFloat::Add<f64>, f64_add, f64, 1, RoundTiesToAway)
TEST_MACRO_2(Addf64, &FloppyFloat::Add<f64>, f64_add, f64, 2, RoundTowardPositive)
TEST_MACRO_2(Addf64, &FloppyFloat::Add<f64>, f64_add, f64, 3, RoundTowardNegative)
TEST_MACRO_2(Addf64, &FloppyFloat::Add<f64>, f64_add, f64, 4, RoundTowardZero)

TEST_MACRO_2(Subf16, &FloppyFloat::Sub<f16>, f16_sub, f16, 0, RoundTiesToEven)
TEST_MACRO_2(Subf16, &FloppyFloat::Sub<f16>, f16_sub, f16, 1, RoundTiesToAway)
TEST_MACRO_2(Subf16, &FloppyFloat::Sub<f16>, f16_sub, f16, 2, RoundTowardPositive)
TEST_MACRO_2(Subf16, &FloppyFloat::Sub<f16>, f16_sub, f16, 3, RoundTowardNegative)
TEST_MACRO_2(Subf16, &FloppyFloat::Sub<f16>, f16_sub, f16, 4, RoundTowardZero)
TEST_MACRO_2(Subf32, &FloppyFloat::Sub<f32>, f32_sub, f32, 0, RoundTiesToEven)
TEST_MACRO_2(Subf32, &FloppyFloat::Sub<f32>, f32_sub, f32, 1, RoundTiesToAway)
TEST_MACRO_2(Subf32, &FloppyFloat::Sub<f32>, f32_sub, f32, 2, RoundTowardPositive)
TEST_MACRO_2(Subf32, &FloppyFloat::Sub<f32>, f32_sub, f32, 3, RoundTowardNegative)
TEST_MACRO_2(Subf32, &FloppyFloat::Sub<f32>, f32_sub, f32, 4, RoundTowardZero)
TEST_MACRO_2(Subf64, &FloppyFloat::Sub<f64>, f64_sub, f64, 0, RoundTiesToEven)
TEST_MACRO_2(Subf64, &FloppyFloat::Sub<f64>, f64_sub, f64, 1, RoundTiesToAway)
TEST_MACRO_2(Subf64, &FloppyFloat::Sub<f64>, f64_sub, f64, 2, RoundTowardPositive)
TEST_MACRO_2(Subf64, &FloppyFloat::Sub<f64>, f64_sub, f64, 3, RoundTowardNegative)
TEST_MACRO_2(Subf64, &FloppyFloat::Sub<f64>, f64_sub, f64, 4, RoundTowardZero)

TEST_MACRO_2(Mulf16, &FloppyFloat::Mul<f16>, f16_mul, f16, 0, RoundTiesToEven)
TEST_MACRO_2(Mulf16, &FloppyFloat::Mul<f16>, f16_mul, f16, 1, RoundTiesToAway)
TEST_MACRO_2(Mulf16, &FloppyFloat::Mul<f16>, f16_mul, f16, 2, RoundTowardPositive)
TEST_MACRO_2(Mulf16, &FloppyFloat::Mul<f16>, f16_mul, f16, 3, RoundTowardNegative)
TEST_MACRO_2(Mulf16, &FloppyFloat::Mul<f16>, f16_mul, f16, 4, RoundTowardZero)
TEST_MACRO_2(Mulf32, &FloppyFloat::Mul<f32>, f32_mul, f32, 0, RoundTiesToEven)
TEST_MACRO_2(Mulf32, &FloppyFloat::Mul<f32>, f32_mul, f32, 1, RoundTiesToAway)
TEST_MACRO_2(Mulf32, &FloppyFloat::Mul<f32>, f32_mul, f32, 2, RoundTowardPositive)
TEST_MACRO_2(Mulf32, &FloppyFloat::Mul<f32>, f32_mul, f32, 3, RoundTowardNegative)
TEST_MACRO_2(Mulf32, &FloppyFloat::Mul<f32>, f32_mul, f32, 4, RoundTowardZero)
TEST_MACRO_2(Mulf64, &FloppyFloat::Mul<f64>, f64_mul, f64, 0, RoundTiesToEven)
TEST_MACRO_2(Mulf64, &FloppyFloat::Mul<f64>, f64_mul, f64, 1, RoundTiesToAway)
TEST_MACRO_2(Mulf64, &FloppyFloat::Mul<f64>, f64_mul, f64, 2, RoundTowardPositive)
TEST_MACRO_2(Mulf64, &FloppyFloat::Mul<f64>, f64_mul, f64, 3, RoundTowardNegative)
TEST_MACRO_2(Mulf64, &FloppyFloat::Mul<f64>, f64_mul, f64, 4, RoundTowardZero)

TEST_MACRO_2(Divf16, &FloppyFloat::Div<f16>, f16_div, f16, 0, RoundTiesToEven)
TEST_MACRO_2(Divf16, &FloppyFloat::Div<f16>, f16_div, f16, 1, RoundTiesToAway)
TEST_MACRO_2(Divf16, &FloppyFloat::Div<f16>, f16_div, f16, 2, RoundTowardPositive)
TEST_MACRO_2(Divf16, &FloppyFloat::Div<f16>, f16_div, f16, 3, RoundTowardNegative)
TEST_MACRO_2(Divf16, &FloppyFloat::Div<f16>, f16_div, f16, 4, RoundTowardZero)
TEST_MACRO_2(Divf32, &FloppyFloat::Div<f32>, f32_div, f32, 0, RoundTiesToEven)
TEST_MACRO_2(Divf32, &FloppyFloat::Div<f32>, f32_div, f32, 1, RoundTiesToAway)
TEST_MACRO_2(Divf32, &FloppyFloat::Div<f32>, f32_div, f32, 2, RoundTowardPositive)
TEST_MACRO_2(Divf32, &FloppyFloat::Div<f32>, f32_div, f32, 3, RoundTowardNegative)
TEST_MACRO_2(Divf32, &FloppyFloat::Div<f32>, f32_div, f32, 4, RoundTowardZero)
TEST_MACRO_2(Divf64, &FloppyFloat::Div<f64>, f64_div, f64, 0, RoundTiesToEven)
TEST_MACRO_2(Divf64, &FloppyFloat::Div<f64>, f64_div, f64, 1, RoundTiesToAway)
TEST_MACRO_2(Divf64, &FloppyFloat::Div<f64>, f64_div, f64, 2, RoundTowardPositive)
TEST_MACRO_2(Divf64, &FloppyFloat::Div<f64>, f64_div, f64, 3, RoundTowardNegative)
TEST_MACRO_2(Divf64, &FloppyFloat::Div<f64>, f64_div, f64, 4, RoundTowardZero)

TEST_MACRO_1(Sqrtf16, &FloppyFloat::Sqrt<f16>, f16_sqrt, f16, 0, RoundTiesToEven)
TEST_MACRO_1(Sqrtf16, &FloppyFloat::Sqrt<f16>, f16_sqrt, f16, 1, RoundTiesToAway)
TEST_MACRO_1(Sqrtf16, &FloppyFloat::Sqrt<f16>, f16_sqrt, f16, 2, RoundTowardPositive)
TEST_MACRO_1(Sqrtf16, &FloppyFloat::Sqrt<f16>, f16_sqrt, f16, 3, RoundTowardNegative)
TEST_MACRO_1(Sqrtf16, &FloppyFloat::Sqrt<f16>, f16_sqrt, f16, 4, RoundTowardZero)
TEST_MACRO_1(Sqrtf32, &FloppyFloat::Sqrt<f32>, f32_sqrt, f32, 0, RoundTiesToEven)
TEST_MACRO_1(Sqrtf32, &FloppyFloat::Sqrt<f32>, f32_sqrt, f32, 1, RoundTiesToAway)
TEST_MACRO_1(Sqrtf32, &FloppyFloat::Sqrt<f32>, f32_sqrt, f32, 2, RoundTowardPositive)
TEST_MACRO_1(Sqrtf32, &FloppyFloat::Sqrt<f32>, f32_sqrt, f32, 3, RoundTowardNegative)
TEST_MACRO_1(Sqrtf32, &FloppyFloat::Sqrt<f32>, f32_sqrt, f32, 4, RoundTowardZero)
TEST_MACRO_1(Sqrtf64, &FloppyFloat::Sqrt<f64>, f64_sqrt, f64, 0, RoundTiesToEven)
TEST_MACRO_1(Sqrtf64, &FloppyFloat::Sqrt<f64>, f64_sqrt, f64, 1, RoundTiesToAway)
TEST_MACRO_1(Sqrtf64, &FloppyFloat::Sqrt<f64>, f64_sqrt, f64, 2, RoundTowardPositive)
TEST_MACRO_1(Sqrtf64, &FloppyFloat::Sqrt<f64>, f64_sqrt, f64, 3, RoundTowardNegative)
TEST_MACRO_1(Sqrtf64, &FloppyFloat::Sqrt<f64>, f64_sqrt, f64, 4, RoundTowardZero)

TEST_MACRO_3(Fmaf16, &FloppyFloat::Fma<f16>, f16_mulAdd, f16, 0, RoundTiesToEven)
TEST_MACRO_3(Fmaf16, &FloppyFloat::Fma<f16>, f16_mulAdd, f16, 1, RoundTiesToAway)
TEST_MACRO_3(Fmaf16, &FloppyFloat::Fma<f16>, f16_mulAdd, f16, 2, RoundTowardPositive)
TEST_MACRO_3(Fmaf16, &FloppyFloat::Fma<f16>, f16_mulAdd, f16, 3, RoundTowardNegative)
TEST_MACRO_3(Fmaf16, &FloppyFloat::Fma<f16>, f16_mulAdd, f16, 4, RoundTowardZero)
TEST_MACRO_3(Fmaf32, &FloppyFloat::Fma<f32>, f32_mulAdd, f32, 0, RoundTiesToEven)
TEST_MACRO_3(Fmaf32, &FloppyFloat::Fma<f32>, f32_mulAdd, f32, 1, RoundTiesToAway)
TEST_MACRO_3(Fmaf32, &FloppyFloat::Fma<f32>, f32_mulAdd, f32, 2, RoundTowardPositive)
TEST_MACRO_3(Fmaf32, &FloppyFloat::Fma<f32>, f32_mulAdd, f32, 3, RoundTowardNegative)
TEST_MACRO_3(Fmaf32, &FloppyFloat::Fma<f32>, f32_mulAdd, f32, 4, RoundTowardZero)
TEST_MACRO_3(Fmaf64, &FloppyFloat::Fma<f64>, f64_mulAdd, f64, 0, RoundTiesToEven)
TEST_MACRO_3(Fmaf64, &FloppyFloat::Fma<f64>, f64_mulAdd, f64, 1, RoundTiesToAway)
TEST_MACRO_3(Fmaf64, &FloppyFloat::Fma<f64>, f64_mulAdd, f64, 2, RoundTowardPositive)
TEST_MACRO_3(Fmaf64, &FloppyFloat::Fma<f64>, f64_mulAdd, f64, 3, RoundTowardNegative)
TEST_MACRO_3(Fmaf64, &FloppyFloat::Fma<f64>, f64_mulAdd, f64, 4, RoundTowardZero)

TEST_MACRO_1(F16ToF32, static_cast<f32 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToF32), f16_to_f32, f16, 0, )
TEST_MACRO_1(F16ToF64, static_cast<f64 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToF64), f16_to_f64, f16, 0, )
TEST_MACRO_1(F32ToF16, static_cast<f16 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToF16), f32_to_f16, f32, 0, RoundTiesToEven)
TEST_MACRO_1(F32ToF16, static_cast<f16 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToF16), f32_to_f16, f32, 1, RoundTiesToAway)
TEST_MACRO_1(F32ToF16, static_cast<f16 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToF16), f32_to_f16, f32, 2, RoundTowardPositive)
TEST_MACRO_1(F32ToF16, static_cast<f16 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToF16), f32_to_f16, f32, 3, RoundTowardNegative)
TEST_MACRO_1(F32ToF16, static_cast<f16 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToF16), f32_to_f16, f32, 4, RoundTowardZero)
TEST_MACRO_1(F32ToF64, static_cast<f64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToF64), f32_to_f64, f32, 0, )
TEST_MACRO_1(F64ToF16, static_cast<f16 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToF16), f64_to_f16, f64, 0, RoundTiesToEven)
TEST_MACRO_1(F64ToF16, static_cast<f16 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToF16), f64_to_f16, f64, 1, RoundTiesToAway)
TEST_MACRO_1(F64ToF16, static_cast<f16 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToF16), f64_to_f16, f64, 2, RoundTowardPositive)
TEST_MACRO_1(F64ToF16, static_cast<f16 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToF16), f64_to_f16, f64, 3, RoundTowardNegative)
TEST_MACRO_1(F64ToF16, static_cast<f16 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToF16), f64_to_f16, f64, 4, RoundTowardZero)
TEST_MACRO_1(F64ToF32, static_cast<f32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToF32), f64_to_f32, f64, 0, RoundTiesToEven)
TEST_MACRO_1(F64ToF32, static_cast<f32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToF32), f64_to_f32, f64, 1, RoundTiesToAway)
TEST_MACRO_1(F64ToF32, static_cast<f32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToF32), f64_to_f32, f64, 2, RoundTowardPositive)
TEST_MACRO_1(F64ToF32, static_cast<f32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToF32), f64_to_f32, f64, 3, RoundTowardNegative)
TEST_MACRO_1(F64ToF32, static_cast<f32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToF32), f64_to_f32, f64, 4, RoundTowardZero)

TEST_MACRO_FTOI(F16ToI32, static_cast<i32 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToI32), f16_to_i32, f16, 0, RoundTiesToEven)
TEST_MACRO_FTOI(F16ToI32, static_cast<i32 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToI32), f16_to_i32, f16, 1, RoundTiesToAway)
TEST_MACRO_FTOI(F16ToI32, static_cast<i32 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToI32), f16_to_i32, f16, 2, RoundTowardPositive)
TEST_MACRO_FTOI(F16ToI32, static_cast<i32 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToI32), f16_to_i32, f16, 3, RoundTowardNegative)
TEST_MACRO_FTOI(F16ToI32, static_cast<i32 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToI32), f16_to_i32, f16, 4, RoundTowardZero)
TEST_MACRO_FTOI(F16ToI64, static_cast<i64 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToI64), f16_to_i64, f16, 0, RoundTiesToEven)
TEST_MACRO_FTOI(F16ToI64, static_cast<i64 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToI64), f16_to_i64, f16, 1, RoundTiesToAway)
TEST_MACRO_FTOI(F16ToI64, static_cast<i64 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToI64), f16_to_i64, f16, 2, RoundTowardPositive)
TEST_MACRO_FTOI(F16ToI64, static_cast<i64 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToI64), f16_to_i64, f16, 3, RoundTowardNegative)
TEST_MACRO_FTOI(F16ToI64, static_cast<i64 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToI64), f16_to_i64, f16, 4, RoundTowardZero)
TEST_MACRO_FTOI(F16ToU32, static_cast<u32 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToU32), f16_to_ui32, f16, 0, RoundTiesToEven)
TEST_MACRO_FTOI(F16ToU32, static_cast<u32 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToU32), f16_to_ui32, f16, 1, RoundTiesToAway)
TEST_MACRO_FTOI(F16ToU32, static_cast<u32 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToU32), f16_to_ui32, f16, 2, RoundTowardPositive)
TEST_MACRO_FTOI(F16ToU32, static_cast<u32 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToU32), f16_to_ui32, f16, 3, RoundTowardNegative)
TEST_MACRO_FTOI(F16ToU32, static_cast<u32 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToU32), f16_to_ui32, f16, 4, RoundTowardZero)
TEST_MACRO_FTOI(F16ToU64, static_cast<u64 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToU64), f16_to_ui64, f16, 0, RoundTiesToEven)
TEST_MACRO_FTOI(F16ToU64, static_cast<u64 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToU64), f16_to_ui64, f16, 1, RoundTiesToAway)
TEST_MACRO_FTOI(F16ToU64, static_cast<u64 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToU64), f16_to_ui64, f16, 2, RoundTowardPositive)
TEST_MACRO_FTOI(F16ToU64, static_cast<u64 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToU64), f16_to_ui64, f16, 3, RoundTowardNegative)
TEST_MACRO_FTOI(F16ToU64, static_cast<u64 (FloppyFloat::*)(f16)>(&FloppyFloat::F16ToU64), f16_to_ui64, f16, 4, RoundTowardZero)

TEST_MACRO_FTOI(F32ToI32, static_cast<i32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI32), f32_to_i32, f32, 0, RoundTiesToEven)
TEST_MACRO_FTOI(F32ToI32, static_cast<i32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI32), f32_to_i32, f32, 1, RoundTiesToAway)
TEST_MACRO_FTOI(F32ToI32, static_cast<i32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI32), f32_to_i32, f32, 2, RoundTowardPositive)
TEST_MACRO_FTOI(F32ToI32, static_cast<i32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI32), f32_to_i32, f32, 3, RoundTowardNegative)
TEST_MACRO_FTOI(F32ToI32, static_cast<i32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI32), f32_to_i32, f32, 4, RoundTowardZero)
TEST_MACRO_FTOI(F32ToI64, static_cast<i64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI64), f32_to_i64, f32, 0, RoundTiesToEven)
TEST_MACRO_FTOI(F32ToI64, static_cast<i64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI64), f32_to_i64, f32, 1, RoundTiesToAway)
TEST_MACRO_FTOI(F32ToI64, static_cast<i64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI64), f32_to_i64, f32, 2, RoundTowardPositive)
TEST_MACRO_FTOI(F32ToI64, static_cast<i64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI64), f32_to_i64, f32, 3, RoundTowardNegative)
TEST_MACRO_FTOI(F32ToI64, static_cast<i64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI64), f32_to_i64, f32, 4, RoundTowardZero)
TEST_MACRO_FTOI(F32ToU32, static_cast<u32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU32), f32_to_ui32, f32, 0, RoundTiesToEven)
TEST_MACRO_FTOI(F32ToU32, static_cast<u32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU32), f32_to_ui32, f32, 1, RoundTiesToAway)
TEST_MACRO_FTOI(F32ToU32, static_cast<u32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU32), f32_to_ui32, f32, 2, RoundTowardPositive)
TEST_MACRO_FTOI(F32ToU32, static_cast<u32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU32), f32_to_ui32, f32, 3, RoundTowardNegative)
TEST_MACRO_FTOI(F32ToU32, static_cast<u32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU32), f32_to_ui32, f32, 4, RoundTowardZero)
TEST_MACRO_FTOI(F32ToU64, static_cast<u64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU64), f32_to_ui64, f32, 0, RoundTiesToEven)
TEST_MACRO_FTOI(F32ToU64, static_cast<u64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU64), f32_to_ui64, f32, 1, RoundTiesToAway)
TEST_MACRO_FTOI(F32ToU64, static_cast<u64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU64), f32_to_ui64, f32, 2, RoundTowardPositive)
TEST_MACRO_FTOI(F32ToU64, static_cast<u64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU64), f32_to_ui64, f32, 3, RoundTowardNegative)
TEST_MACRO_FTOI(F32ToU64, static_cast<u64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU64), f32_to_ui64, f32, 4, RoundTowardZero)

TEST_MACRO_FTOI(F64ToI32, static_cast<i32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToI32), f64_to_i32, f64, 0, RoundTiesToEven)
TEST_MACRO_FTOI(F64ToI32, static_cast<i32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToI32), f64_to_i32, f64, 1, RoundTiesToAway)
TEST_MACRO_FTOI(F64ToI32, static_cast<i32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToI32), f64_to_i32, f64, 2, RoundTowardPositive)
TEST_MACRO_FTOI(F64ToI32, static_cast<i32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToI32), f64_to_i32, f64, 3, RoundTowardNegative)
TEST_MACRO_FTOI(F64ToI32, static_cast<i32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToI32), f64_to_i32, f64, 4, RoundTowardZero)
TEST_MACRO_FTOI(F64ToI64, static_cast<i64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToI64), f64_to_i64, f64, 0, RoundTiesToEven)
TEST_MACRO_FTOI(F64ToI64, static_cast<i64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToI64), f64_to_i64, f64, 1, RoundTiesToAway)
TEST_MACRO_FTOI(F64ToI64, static_cast<i64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToI64), f64_to_i64, f64, 2, RoundTowardPositive)
TEST_MACRO_FTOI(F64ToI64, static_cast<i64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToI64), f64_to_i64, f64, 3, RoundTowardNegative)
TEST_MACRO_FTOI(F64ToI64, static_cast<i64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToI64), f64_to_i64, f64, 4, RoundTowardZero)
TEST_MACRO_FTOI(F64ToU32, static_cast<u32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU32), f64_to_ui32, f64, 0, RoundTiesToEven)
TEST_MACRO_FTOI(F64ToU32, static_cast<u32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU32), f64_to_ui32, f64, 1, RoundTiesToAway)
TEST_MACRO_FTOI(F64ToU32, static_cast<u32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU32), f64_to_ui32, f64, 2, RoundTowardPositive)
TEST_MACRO_FTOI(F64ToU32, static_cast<u32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU32), f64_to_ui32, f64, 3, RoundTowardNegative)
TEST_MACRO_FTOI(F64ToU32, static_cast<u32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU32), f64_to_ui32, f64, 4, RoundTowardZero)
TEST_MACRO_FTOI(F64ToU64, static_cast<u64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU64), f64_to_ui64, f64, 0, RoundTiesToEven)
TEST_MACRO_FTOI(F64ToU64, static_cast<u64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU64), f64_to_ui64, f64, 1, RoundTiesToAway)
TEST_MACRO_FTOI(F64ToU64, static_cast<u64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU64), f64_to_ui64, f64, 2, RoundTowardPositive)
TEST_MACRO_FTOI(F64ToU64, static_cast<u64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU64), f64_to_ui64, f64, 3, RoundTowardNegative)
TEST_MACRO_FTOI(F64ToU64, static_cast<u64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU64), f64_to_ui64, f64, 4, RoundTowardZero)

TEST_MACRO_2(EqQuietf16, &FloppyFloat::EqQuiet<f16>, f16_eq, f16, 0, )
TEST_MACRO_2(EqQuietf32, &FloppyFloat::EqQuiet<f32>, f32_eq, f32, 0, )
TEST_MACRO_2(EqQuietf64, &FloppyFloat::EqQuiet<f64>, f64_eq, f64, 0, )
TEST_MACRO_2(EqSignalingf16, &FloppyFloat::EqSignaling<f16>, f16_eq_signaling, f16, 0, )
TEST_MACRO_2(EqSignalingf32, &FloppyFloat::EqSignaling<f32>, f32_eq_signaling, f32, 0, )
TEST_MACRO_2(EqSignalingf64, &FloppyFloat::EqSignaling<f64>, f64_eq_signaling, f64, 0, )

TEST_MACRO_2(LtQuietf16, &FloppyFloat::LtQuiet<f16>, f16_lt_quiet, f16, 0, )
TEST_MACRO_2(LtQuietf32, &FloppyFloat::LtQuiet<f32>, f32_lt_quiet, f32, 0, )
TEST_MACRO_2(LtQuietf64, &FloppyFloat::LtQuiet<f64>, f64_lt_quiet, f64, 0, )
TEST_MACRO_2(LtSignalingf16, &FloppyFloat::LtSignaling<f16>, f16_lt, f16, 0, )
TEST_MACRO_2(LtSignalingf32, &FloppyFloat::LtSignaling<f32>, f32_lt, f32, 0, )
TEST_MACRO_2(LtSignalingf64, &FloppyFloat::LtSignaling<f64>, f64_lt, f64, 0, )

TEST_MACRO_2(LeQuietf16, &FloppyFloat::LeQuiet<f16>, f16_le_quiet, f16, 0, )
TEST_MACRO_2(LeQuietf32, &FloppyFloat::LeQuiet<f32>, f32_le_quiet, f32, 0, )
TEST_MACRO_2(LeQuietf64, &FloppyFloat::LeQuiet<f64>, f64_le_quiet, f64, 0, )
TEST_MACRO_2(LeSignalingf16, &FloppyFloat::LeSignaling<f16>, f16_le, f16, 0, )
TEST_MACRO_2(LeSignalingf32, &FloppyFloat::LeSignaling<f32>, f32_le, f32, 0, )
TEST_MACRO_2(LeSignalingf64, &FloppyFloat::LeSignaling<f64>, f64_le, f64, 0, )

TEST_MACRO_ITOF(I32ToF16, I32ToF16, i32_to_f16, i32, 0, RoundTiesToEven)
TEST_MACRO_ITOF(I32ToF16, I32ToF16, i32_to_f16, i32, 1, RoundTiesToAway)
TEST_MACRO_ITOF(I32ToF16, I32ToF16, i32_to_f16, i32, 2, RoundTowardPositive)
TEST_MACRO_ITOF(I32ToF16, I32ToF16, i32_to_f16, i32, 3, RoundTowardNegative)
TEST_MACRO_ITOF(I32ToF16, I32ToF16, i32_to_f16, i32, 4, RoundTowardZero)
TEST_MACRO_ITOF(I32ToF32, I32ToF32, i32_to_f32, i32, 0, RoundTiesToEven)
TEST_MACRO_ITOF(I32ToF32, I32ToF32, i32_to_f32, i32, 1, RoundTiesToAway)
TEST_MACRO_ITOF(I32ToF32, I32ToF32, i32_to_f32, i32, 2, RoundTowardPositive)
TEST_MACRO_ITOF(I32ToF32, I32ToF32, i32_to_f32, i32, 3, RoundTowardNegative)
TEST_MACRO_ITOF(I32ToF32, I32ToF32, i32_to_f32, i32, 4, RoundTowardZero)
TEST_MACRO_ITOF(I32ToF64, I32ToF64, i32_to_f64, i32, 0, RoundTiesToEven)
TEST_MACRO_ITOF(I32ToF64, I32ToF64, i32_to_f64, i32, 1, RoundTiesToAway)
TEST_MACRO_ITOF(I32ToF64, I32ToF64, i32_to_f64, i32, 2, RoundTowardPositive)
TEST_MACRO_ITOF(I32ToF64, I32ToF64, i32_to_f64, i32, 3, RoundTowardNegative)
TEST_MACRO_ITOF(I32ToF64, I32ToF64, i32_to_f64, i32, 4, RoundTowardZero)

TEST_MACRO_ITOF(U32ToF16, U32ToF16, ui32_to_f16, u32, 0, RoundTiesToEven)
TEST_MACRO_ITOF(U32ToF16, U32ToF16, ui32_to_f16, u32, 1, RoundTiesToAway)
TEST_MACRO_ITOF(U32ToF16, U32ToF16, ui32_to_f16, u32, 2, RoundTowardPositive)
TEST_MACRO_ITOF(U32ToF16, U32ToF16, ui32_to_f16, u32, 3, RoundTowardNegative)
TEST_MACRO_ITOF(U32ToF16, U32ToF16, ui32_to_f16, u32, 4, RoundTowardZero)
TEST_MACRO_ITOF(U32ToF32, U32ToF32, ui32_to_f32, u32, 0, RoundTiesToEven)
TEST_MACRO_ITOF(U32ToF32, U32ToF32, ui32_to_f32, u32, 1, RoundTiesToAway)
TEST_MACRO_ITOF(U32ToF32, U32ToF32, ui32_to_f32, u32, 2, RoundTowardPositive)
TEST_MACRO_ITOF(U32ToF32, U32ToF32, ui32_to_f32, u32, 3, RoundTowardNegative)
TEST_MACRO_ITOF(U32ToF32, U32ToF32, ui32_to_f32, u32, 4, RoundTowardZero)
TEST_MACRO_ITOF(U32ToF64, U32ToF64, ui32_to_f64, u32, 0, RoundTiesToEven)
TEST_MACRO_ITOF(U32ToF64, U32ToF64, ui32_to_f64, u32, 1, RoundTiesToAway)
TEST_MACRO_ITOF(U32ToF64, U32ToF64, ui32_to_f64, u32, 2, RoundTowardPositive)
TEST_MACRO_ITOF(U32ToF64, U32ToF64, ui32_to_f64, u32, 3, RoundTowardNegative)
TEST_MACRO_ITOF(U32ToF64, U32ToF64, ui32_to_f64, u32, 4, RoundTowardZero)

TEST_MACRO_ITOF(I64ToF16, I64ToF16, i64_to_f16, i64, 0, RoundTiesToEven)
TEST_MACRO_ITOF(I64ToF16, I64ToF16, i64_to_f16, i64, 1, RoundTiesToAway)
TEST_MACRO_ITOF(I64ToF16, I64ToF16, i64_to_f16, i64, 2, RoundTowardPositive)
TEST_MACRO_ITOF(I64ToF16, I64ToF16, i64_to_f16, i64, 3, RoundTowardNegative)
TEST_MACRO_ITOF(I64ToF16, I64ToF16, i64_to_f16, i64, 4, RoundTowardZero)
TEST_MACRO_ITOF(I64ToF32, I64ToF32, i64_to_f32, i64, 0, RoundTiesToEven)
TEST_MACRO_ITOF(I64ToF32, I64ToF32, i64_to_f32, i64, 1, RoundTiesToAway)
TEST_MACRO_ITOF(I64ToF32, I64ToF32, i64_to_f32, i64, 2, RoundTowardPositive)
TEST_MACRO_ITOF(I64ToF32, I64ToF32, i64_to_f32, i64, 3, RoundTowardNegative)
TEST_MACRO_ITOF(I64ToF32, I64ToF32, i64_to_f32, i64, 4, RoundTowardZero)
TEST_MACRO_ITOF(I64ToF64, I64ToF64, i64_to_f64, i64, 0, RoundTiesToEven)
TEST_MACRO_ITOF(I64ToF64, I64ToF64, i64_to_f64, i64, 1, RoundTiesToAway)
TEST_MACRO_ITOF(I64ToF64, I64ToF64, i64_to_f64, i64, 2, RoundTowardPositive)
TEST_MACRO_ITOF(I64ToF64, I64ToF64, i64_to_f64, i64, 3, RoundTowardNegative)
TEST_MACRO_ITOF(I64ToF64, I64ToF64, i64_to_f64, i64, 4, RoundTowardZero)

TEST_MACRO_ITOF(U64ToF16, U64ToF16, ui64_to_f16, u64, 0, RoundTiesToEven)
TEST_MACRO_ITOF(U64ToF16, U64ToF16, ui64_to_f16, u64, 1, RoundTiesToAway)
TEST_MACRO_ITOF(U64ToF16, U64ToF16, ui64_to_f16, u64, 2, RoundTowardPositive)
TEST_MACRO_ITOF(U64ToF16, U64ToF16, ui64_to_f16, u64, 3, RoundTowardNegative)
TEST_MACRO_ITOF(U64ToF16, U64ToF16, ui64_to_f16, u64, 4, RoundTowardZero)
TEST_MACRO_ITOF(U64ToF32, U64ToF32, ui64_to_f32, u64, 0, RoundTiesToEven)
TEST_MACRO_ITOF(U64ToF32, U64ToF32, ui64_to_f32, u64, 1, RoundTiesToAway)
TEST_MACRO_ITOF(U64ToF32, U64ToF32, ui64_to_f32, u64, 2, RoundTowardPositive)
TEST_MACRO_ITOF(U64ToF32, U64ToF32, ui64_to_f32, u64, 3, RoundTowardNegative)
TEST_MACRO_ITOF(U64ToF32, U64ToF32, ui64_to_f32, u64, 4, RoundTowardZero)
TEST_MACRO_ITOF(U64ToF64, U64ToF64, ui64_to_f64, u64, 0, RoundTiesToEven)
TEST_MACRO_ITOF(U64ToF64, U64ToF64, ui64_to_f64, u64, 1, RoundTiesToAway)
TEST_MACRO_ITOF(U64ToF64, U64ToF64, ui64_to_f64, u64, 2, RoundTowardPositive)
TEST_MACRO_ITOF(U64ToF64, U64ToF64, ui64_to_f64, u64, 3, RoundTowardNegative)
TEST_MACRO_ITOF(U64ToF64, U64ToF64, ui64_to_f64, u64, 4, RoundTowardZero)

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
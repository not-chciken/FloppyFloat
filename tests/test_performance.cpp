/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 * Compile with: g++ -g -O3 -std=c++23 test_perf2.cpp -lsoftfloat -lFloppyFloat
 ******************************************************************************/

#include <algorithm>
#include <bit>
#include <cfenv>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <vector>

#include "floppy_float.h"
#include "utils.h"

extern "C" {
#include "softfloat.h"
}

using namespace std::placeholders;
using namespace FfUtils;

constexpr i32 kNumIterations = 30000000;
constexpr i32 kRngSeed = 42;

template <typename FT>
class FloatRng {
 public:
  FloatRng(int seed) : index_(0), engine_(seed), dist_(0, 1024), values_() {
    for (size_t i = 0; i < size_; ++i) {
      FT sign = 1;  //(dist_(engine_) & 1) ? (FT)-1. : (FT)1.;
      values_.push_back(((FT)dist_(engine_)) / (FT)100 * sign);
    }
  }

  constexpr FT Gen() { return values_[index_++ % size_]; }

  void Reset() {}

 private:
  size_t index_;
  static constexpr size_t size_ = 1024;
  std::mt19937 engine_;
  std::uniform_int_distribution<int> dist_;
  std::vector<FT> values_;
};

std::vector<std::tuple<std::string, f64>> result_vec;

#define PERF_TEST_FF_0(func, ftype, ...)                                                      \
  {                                                                                           \
    FloatRng<ftype> float_rng(kRngSeed);                                                      \
    [[maybe_unused]] ftype a, b, c;                                                           \
    a = float_rng.Gen();                                                                      \
    b = float_rng.Gen();                                                                      \
    c = float_rng.Gen();                                                                      \
    begin = std::chrono::steady_clock::now();                                                 \
    for (size_t i = 0; i < kNumIterations; ++i) {                                             \
      [[maybe_unused]] auto result = func(__VA_ARGS__);                                       \
      c = b;                                                                                  \
      b = a;                                                                                  \
      a = float_rng.Gen();                                                                    \
    }                                                                                         \
    end = std::chrono::steady_clock::now();                                                   \
    ms_ff_float = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count(); \
    float_rng.Reset();                                                                        \
  }

#define PERF_TEST_FF_1(rm, func, ftype, ...)                                                  \
  {                                                                                           \
    ff.rounding_mode = rm;                                                                    \
    FloatRng<ftype> float_rng(kRngSeed);                                                      \
    [[maybe_unused]] ftype a, b, c;                                                           \
    a = float_rng.Gen();                                                                      \
    b = float_rng.Gen();                                                                      \
    c = float_rng.Gen();                                                                      \
    begin = std::chrono::steady_clock::now();                                                 \
    for (size_t i = 0; i < kNumIterations; ++i) {                                             \
      [[maybe_unused]] ftype result;                                                          \
      FLOPPY_FLOAT_FUNC_1(result, rm, func, __VA_ARGS__)                                      \
      c = b;                                                                                  \
      b = a;                                                                                  \
      a = float_rng.Gen();                                                                    \
    }                                                                                         \
    end = std::chrono::steady_clock::now();                                                   \
    ms_ff_float = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count(); \
    float_rng.Reset();                                                                        \
  }

#define PERF_TEST_FF_2(rm, func, ftype, ...)                                                  \
  {                                                                                           \
    ff.rounding_mode = rm;                                                                    \
    FloatRng<ftype> float_rng(kRngSeed);                                                      \
    [[maybe_unused]] ftype a, b, c;                                                           \
    a = float_rng.Gen();                                                                      \
    b = float_rng.Gen();                                                                      \
    c = float_rng.Gen();                                                                      \
    begin = std::chrono::steady_clock::now();                                                 \
    for (size_t i = 0; i < kNumIterations; ++i) {                                             \
      [[maybe_unused]] ftype result;                                                          \
      FLOPPY_FLOAT_FUNC_2(result, rm, func, ftype, __VA_ARGS__)                               \
      c = b;                                                                                  \
      b = a;                                                                                  \
      a = float_rng.Gen();                                                                    \
    }                                                                                         \
    end = std::chrono::steady_clock::now();                                                   \
    ms_ff_float = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count(); \
    float_rng.Reset();                                                                        \
  }

#define PERF_TEST_SF(rm, func, sftype, ftype, ...)                                            \
  {                                                                                           \
    ::softfloat_roundingMode = rm;                                                            \
    FloatRng<ftype> float_rng(kRngSeed);                                                      \
    [[maybe_unused]] sftype a, b, c;                                                          \
    a.v = std::bit_cast<typename FloatToUint<ftype>::type>(float_rng.Gen());                  \
    b.v = std::bit_cast<typename FloatToUint<ftype>::type>(float_rng.Gen());                  \
    c.v = std::bit_cast<typename FloatToUint<ftype>::type>(float_rng.Gen());                  \
    begin = std::chrono::steady_clock::now();                                                 \
    for (size_t i = 0; i < kNumIterations; ++i) {                                             \
      [[maybe_unused]] auto result = func(__VA_ARGS__);                                       \
      b.v = a.v;                                                                              \
      a.v = std::bit_cast<typename FloatToUint<ftype>::type>(float_rng.Gen());                \
    }                                                                                         \
    end = std::chrono::steady_clock::now();                                                   \
    ms_sf_float = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count(); \
    float_rng.Reset();                                                                        \
  }

int main() {
  FloppyFloat ff;
  ff.SetupToX86();

  ::softfloat_exceptionFlags = 0xff;
  ff.inexact = true;
  ff.underflow = true;
  ff.invalid = true;
  ff.overflow = true;
  ff.division_by_zero = true;

  std::chrono::steady_clock::time_point begin;
  std::chrono::steady_clock::time_point end;

  i64 ms_sf_float;
  i64 ms_ff_float;

  [[maybe_unused]] f64 result;

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToEven, ff.Add, f32, a, b)
  PERF_TEST_SF(::softfloat_round_near_even, f32_add, float32_t, f32, a, b)
  result_vec.push_back({"Addf32", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardPositive, ff.Add, f32, a, b)
  PERF_TEST_SF(::softfloat_round_max, f32_add, float32_t, f32, a, b)
  result_vec.push_back({"Addf32RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardNegative, ff.Add, f32, a, b)
  PERF_TEST_SF(::softfloat_round_min, f32_add, float32_t, f32, a, b)
  result_vec.push_back({"Addf32RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardZero, ff.Add, f32, a, b)
  PERF_TEST_SF(::softfloat_round_minMag, f32_add, float32_t, f32, a, b)
  result_vec.push_back({"Addf32RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToAway, ff.Add, f32, a, b)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f32_add, float32_t, f32, a, b)
  result_vec.push_back({"Addf32RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToEven, ff.Sub, f32, a, b)
  PERF_TEST_SF(::softfloat_round_near_even, f32_sub, float32_t, f32, a, b)
  result_vec.push_back({"Subf32", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardPositive, ff.Sub, f32, a, b)
  PERF_TEST_SF(::softfloat_round_max, f32_sub, float32_t, f32, a, b)
  result_vec.push_back({"Subf32RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardNegative, ff.Sub, f32, a, b)
  PERF_TEST_SF(::softfloat_round_min, f32_sub, float32_t, f32, a, b)
  result_vec.push_back({"Subf32RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardZero, ff.Sub, f32, a, b)
  PERF_TEST_SF(::softfloat_round_minMag, f32_sub, float32_t, f32, a, b)
  result_vec.push_back({"Subf32RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToAway, ff.Sub, f32, a, b)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f32_sub, float32_t, f32, a, b)
  result_vec.push_back({"Subf32RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToEven, ff.Mul, f32, a, b)
  PERF_TEST_SF(::softfloat_round_near_even, f32_mul, float32_t, f32, a, b)
  result_vec.push_back({"Mulf32", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardPositive, ff.Mul, f32, a, b)
  PERF_TEST_SF(::softfloat_round_max, f32_mul, float32_t, f32, a, b)
  result_vec.push_back({"Mulf32RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardNegative, ff.Mul, f32, a, b)
  PERF_TEST_SF(::softfloat_round_min, f32_mul, float32_t, f32, a, b)
  result_vec.push_back({"Mulf32RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardZero, ff.Mul, f32, a, b)
  PERF_TEST_SF(::softfloat_round_minMag, f32_mul, float32_t, f32, a, b)
  result_vec.push_back({"Mulf32RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToAway, ff.Mul, f32, a, b)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f32_mul, float32_t, f32, a, b)
  result_vec.push_back({"Mulf32RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToEven, ff.Div, f32, a, b)
  PERF_TEST_SF(::softfloat_round_near_even, f32_div, float32_t, f32, a, b)
  result_vec.push_back({"Divf32", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardPositive, ff.Div, f32, a, b)
  PERF_TEST_SF(::softfloat_round_max, f32_div, float32_t, f32, a, b)
  result_vec.push_back({"Divf32RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardNegative, ff.Div, f32, a, b)
  PERF_TEST_SF(::softfloat_round_min, f32_div, float32_t, f32, a, b)
  result_vec.push_back({"Divf32RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardZero, ff.Div, f32, a, b)
  PERF_TEST_SF(::softfloat_round_minMag, f32_div, float32_t, f32, a, b)
  result_vec.push_back({"Divf32RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToAway, ff.Div, f32, a, b)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f32_div, float32_t, f32, a, b)
  result_vec.push_back({"Divf32RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToEven, ff.Sqrt, f32, a)
  PERF_TEST_SF(::softfloat_round_near_even, f32_sqrt, float32_t, f32, a)
  result_vec.push_back({"Sqrtf32", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardPositive, ff.Sqrt, f32, a)
  PERF_TEST_SF(::softfloat_round_max, f32_sqrt, float32_t, f32, a)
  result_vec.push_back({"Sqrtf32RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardNegative, ff.Sqrt, f32, a)
  PERF_TEST_SF(::softfloat_round_min, f32_sqrt, float32_t, f32, a)
  result_vec.push_back({"Sqrtf32RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardZero, ff.Sqrt, f32, a)
  PERF_TEST_SF(::softfloat_round_minMag, f32_sqrt, float32_t, f32, a)
  result_vec.push_back({"Sqrtf32RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToAway, ff.Sqrt, f32, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f32_sqrt, float32_t, f32, a)
  result_vec.push_back({"Sqrtf32RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToEven, ff.Fma, f32, a, b, c)
  PERF_TEST_SF(::softfloat_round_near_even, f32_mulAdd, float32_t, f32, a, b, c)
  result_vec.push_back({"Fmaf32", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardPositive, ff.Fma, f32, a, b, c)
  PERF_TEST_SF(::softfloat_round_max, f32_mulAdd, float32_t, f32, a, b, c)
  result_vec.push_back({"Fmaf32RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardNegative, ff.Fma, f32, a, b, c)
  PERF_TEST_SF(::softfloat_round_min, f32_mulAdd, float32_t, f32, a, b, c)
  result_vec.push_back({"Fmaf32RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardZero, ff.Fma, f32, a, b, c)
  PERF_TEST_SF(::softfloat_round_minMag, f32_mulAdd, float32_t, f32, a, b, c)
  result_vec.push_back({"Fmaf32RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToAway, ff.Fma, f32, a, b, c)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f32_mulAdd, float32_t, f32, a, b, c)
  result_vec.push_back({"Fmaf32RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToEven, ff.F32ToF16, f32, a)
  PERF_TEST_SF(::softfloat_round_near_even, f32_to_f16, float32_t, f32, a)
  result_vec.push_back({"F32ToF16", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardPositive, ff.F32ToF16, f32, a)
  PERF_TEST_SF(::softfloat_round_max, f32_to_f16, float32_t, f32, a)
  result_vec.push_back({"F32ToF16RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardNegative, ff.F32ToF16, f32, a)
  PERF_TEST_SF(::softfloat_round_min, f32_to_f16, float32_t, f32, a)
  result_vec.push_back({"F32ToF16RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardZero, ff.F32ToF16, f32, a)
  PERF_TEST_SF(::softfloat_round_minMag, f32_to_f16, float32_t, f32, a)
  result_vec.push_back({"F32ToF16RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToAway, ff.F32ToF16, f32, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f32_to_f16, float32_t, f32, a)
  result_vec.push_back({"F32ToF16RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_0(ff.F32ToF64, f32, a)
  PERF_TEST_SF(::softfloat_round_near_even, f32_to_f64, float32_t, f32, a)
  result_vec.push_back({"F32ToF64", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToEven, ff.F32ToI32, f32, a)
  PERF_TEST_SF(::softfloat_round_near_even, f32_to_i32, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToI32", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardPositive, ff.F32ToI32, f32, a)
  PERF_TEST_SF(::softfloat_round_max, f32_to_i32, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToI32RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardNegative, ff.F32ToI32, f32, a)
  PERF_TEST_SF(::softfloat_round_min, f32_to_i32, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToI32RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardZero, ff.F32ToI32, f32, a)
  PERF_TEST_SF(::softfloat_round_minMag, f32_to_i32, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToI32RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToAway, ff.F32ToI32, f32, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f32_to_i32, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToFI32RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToEven, ff.F32ToU32, f32, a)
  PERF_TEST_SF(::softfloat_round_near_even, f32_to_ui32, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToU32", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardPositive, ff.F32ToU32, f32, a)
  PERF_TEST_SF(::softfloat_round_max, f32_to_ui32, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToU32RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardNegative, ff.F32ToU32, f32, a)
  PERF_TEST_SF(::softfloat_round_min, f32_to_ui32, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToU32RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardZero, ff.F32ToU32, f32, a)
  PERF_TEST_SF(::softfloat_round_minMag, f32_to_ui32, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToU32RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToAway, ff.F32ToU32, f32, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f32_to_ui32, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToU32RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToEven, ff.F32ToI64, f32, a)
  PERF_TEST_SF(::softfloat_round_near_even, f32_to_i64, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToI64", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardPositive, ff.F32ToI64, f32, a)
  PERF_TEST_SF(::softfloat_round_max, f32_to_i64, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToI64RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardNegative, ff.F32ToI64, f32, a)
  PERF_TEST_SF(::softfloat_round_min, f32_to_i64, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToI64RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardZero, ff.F32ToI64, f32, a)
  PERF_TEST_SF(::softfloat_round_minMag, f32_to_i64, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToI64RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToAway, ff.F32ToI64, f32, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f32_to_i64, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToI64RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToEven, ff.F32ToU64, f32, a)
  PERF_TEST_SF(::softfloat_round_near_even, f32_to_ui64, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToU64", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardPositive, ff.F32ToU64, f32, a)
  PERF_TEST_SF(::softfloat_round_max, f32_to_ui64, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToU64RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardNegative, ff.F32ToU64, f32, a)
  PERF_TEST_SF(::softfloat_round_min, f32_to_ui64, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToU64RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardZero, ff.F32ToU64, f32, a)
  PERF_TEST_SF(::softfloat_round_minMag, f32_to_ui64, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToU64RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToAway, ff.F32ToU64, f32, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f32_to_ui64, float32_t, f32, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F32ToU64RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToEven, ff.Add, f64, a, b)
  PERF_TEST_SF(::softfloat_round_near_even, f64_add, float64_t, f64, a, b)
  result_vec.push_back({"Addf64", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardPositive, ff.Add, f64, a, b)
  PERF_TEST_SF(::softfloat_round_max, f64_add, float64_t, f64, a, b)
  result_vec.push_back({"Addf64RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardNegative, ff.Add, f64, a, b)
  PERF_TEST_SF(::softfloat_round_min, f64_add, float64_t, f64, a, b)
  result_vec.push_back({"Addf64RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardZero, ff.Add, f64, a, b)
  PERF_TEST_SF(::softfloat_round_minMag, f64_add, float64_t, f64, a, b)
  result_vec.push_back({"Addf64RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToAway, ff.Add, f64, a, b)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f64_add, float64_t, f64, a, b)
  result_vec.push_back({"Addf64RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToEven, ff.Sub, f64, a, b)
  PERF_TEST_SF(::softfloat_round_near_even, f64_sub, float64_t, f64, a, b)
  result_vec.push_back({"Subf64", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardPositive, ff.Sub, f64, a, b)
  PERF_TEST_SF(::softfloat_round_max, f64_sub, float64_t, f64, a, b)
  result_vec.push_back({"Subf64RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardNegative, ff.Sub, f64, a, b)
  PERF_TEST_SF(::softfloat_round_min, f64_sub, float64_t, f64, a, b)
  result_vec.push_back({"Subf64RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardZero, ff.Sub, f64, a, b)
  PERF_TEST_SF(::softfloat_round_minMag, f64_sub, float64_t, f64, a, b)
  result_vec.push_back({"Subf64RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToAway, ff.Sub, f64, a, b)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f64_sub, float64_t, f64, a, b)
  result_vec.push_back({"Subf64RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToEven, ff.Mul, f64, a, b)
  PERF_TEST_SF(::softfloat_round_near_even, f64_mul, float64_t, f64, a, b)
  result_vec.push_back({"Mulf64", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardPositive, ff.Mul, f64, a, b)
  PERF_TEST_SF(::softfloat_round_max, f64_mul, float64_t, f64, a, b)
  result_vec.push_back({"Mulf64RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardNegative, ff.Mul, f64, a, b)
  PERF_TEST_SF(::softfloat_round_min, f64_mul, float64_t, f64, a, b)
  result_vec.push_back({"Mulf64RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardZero, ff.Mul, f64, a, b)
  PERF_TEST_SF(::softfloat_round_minMag, f64_mul, float64_t, f64, a, b)
  result_vec.push_back({"Mulf64RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToAway, ff.Mul, f64, a, b)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f64_mul, float64_t, f64, a, b)
  result_vec.push_back({"Mulf64RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToEven, ff.Div, f64, a, b)
  PERF_TEST_SF(::softfloat_round_near_even, f64_div, float64_t, f64, a, b)
  result_vec.push_back({"Divf64", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardPositive, ff.Div, f64, a, b)
  PERF_TEST_SF(::softfloat_round_max, f64_div, float64_t, f64, a, b)
  result_vec.push_back({"Divf64RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardNegative, ff.Div, f64, a, b)
  PERF_TEST_SF(::softfloat_round_min, f64_div, float64_t, f64, a, b)
  result_vec.push_back({"Divf64RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardZero, ff.Div, f64, a, b)
  PERF_TEST_SF(::softfloat_round_minMag, f64_div, float64_t, f64, a, b)
  result_vec.push_back({"Divf64RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToAway, ff.Div, f64, a, b)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f64_div, float64_t, f64, a, b)
  result_vec.push_back({"Divf64RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToEven, ff.Sqrt, f64, a)
  PERF_TEST_SF(::softfloat_round_near_even, f64_sqrt, float64_t, f64, a)
  result_vec.push_back({"Sqrtf64", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardPositive, ff.Sqrt, f64, a)
  PERF_TEST_SF(::softfloat_round_max, f64_sqrt, float64_t, f64, a)
  result_vec.push_back({"Sqrtf64RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardNegative, ff.Sqrt, f64, a)
  PERF_TEST_SF(::softfloat_round_min, f64_sqrt, float64_t, f64, a)
  result_vec.push_back({"Sqrtf64RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardZero, ff.Sqrt, f64, a)
  PERF_TEST_SF(::softfloat_round_minMag, f64_sqrt, float64_t, f64, a)
  result_vec.push_back({"Sqrtf64RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToAway, ff.Sqrt, f64, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f64_sqrt, float64_t, f64, a)
  result_vec.push_back({"Sqrtf64RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToEven, ff.Fma, f64, a, b, c)
  PERF_TEST_SF(::softfloat_round_near_even, f64_mulAdd, float64_t, f64, a, b, c)
  result_vec.push_back({"Fmaf64", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardPositive, ff.Fma, f64, a, b, c)
  PERF_TEST_SF(::softfloat_round_max, f64_mulAdd, float64_t, f64, a, b, c)
  result_vec.push_back({"Fmaf64RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardNegative, ff.Fma, f64, a, b, c)
  PERF_TEST_SF(::softfloat_round_min, f64_mulAdd, float64_t, f64, a, b, c)
  result_vec.push_back({"Fmaf64RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTowardZero, ff.Fma, f64, a, b, c)
  PERF_TEST_SF(::softfloat_round_minMag, f64_mulAdd, float64_t, f64, a, b, c)
  result_vec.push_back({"Fmaf64RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_2(Vfpu::RoundingMode::kRoundTiesToAway, ff.Fma, f64, a, b, c)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f64_mulAdd, float64_t, f64, a, b, c)
  result_vec.push_back({"Fmaf64RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToEven, ff.F64ToF16, f64, a)
  PERF_TEST_SF(::softfloat_round_near_even, f64_to_f16, float64_t, f64, a)
  result_vec.push_back({"F64ToF16", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardPositive, ff.F64ToF16, f64, a)
  PERF_TEST_SF(::softfloat_round_max, f64_to_f16, float64_t, f64, a)
  result_vec.push_back({"F64ToF16RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardNegative, ff.F64ToF16, f64, a)
  PERF_TEST_SF(::softfloat_round_min, f64_to_f16, float64_t, f64, a)
  result_vec.push_back({"F64ToF16RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardZero, ff.F64ToF16, f64, a)
  PERF_TEST_SF(::softfloat_round_minMag, f64_to_f16, float64_t, f64, a)
  result_vec.push_back({"F64ToF16RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToAway, ff.F64ToF16, f64, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f64_to_f16, float64_t, f64, a)
  result_vec.push_back({"F64ToF16RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToEven, ff.F64ToF32, f64, a)
  PERF_TEST_SF(::softfloat_round_near_even, f64_to_f32, float64_t, f64, a)
  result_vec.push_back({"F64ToF32", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardPositive, ff.F64ToF32, f64, a)
  PERF_TEST_SF(::softfloat_round_max, f64_to_f32, float64_t, f64, a)
  result_vec.push_back({"F64ToF32RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardNegative, ff.F64ToF32, f64, a)
  PERF_TEST_SF(::softfloat_round_min, f64_to_f32, float64_t, f64, a)
  result_vec.push_back({"F64ToF32RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardZero, ff.F64ToF32, f64, a)
  PERF_TEST_SF(::softfloat_round_minMag, f64_to_f32, float64_t, f64, a)
  result_vec.push_back({"F64ToF32RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToAway, ff.F64ToF32, f64, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f64_to_f32, float64_t, f64, a)
  result_vec.push_back({"F64ToF32RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToEven, ff.F64ToI32, f64, a)
  PERF_TEST_SF(::softfloat_round_near_even, f64_to_i32, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToI32", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardPositive, ff.F64ToI32, f64, a)
  PERF_TEST_SF(::softfloat_round_max, f64_to_i32, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToI32RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardNegative, ff.F64ToI32, f64, a)
  PERF_TEST_SF(::softfloat_round_min, f64_to_i32, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToI32RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardZero, ff.F64ToI32, f64, a)
  PERF_TEST_SF(::softfloat_round_minMag, f64_to_i32, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToI32RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToAway, ff.F64ToI32, f64, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f64_to_i32, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToFI32RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToEven, ff.F64ToU32, f64, a)
  PERF_TEST_SF(::softfloat_round_near_even, f64_to_ui32, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToU32", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardPositive, ff.F64ToU32, f64, a)
  PERF_TEST_SF(::softfloat_round_max, f64_to_ui32, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToU32RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardNegative, ff.F64ToU32, f64, a)
  PERF_TEST_SF(::softfloat_round_min, f64_to_ui32, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToU32RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardZero, ff.F64ToU32, f64, a)
  PERF_TEST_SF(::softfloat_round_minMag, f64_to_ui32, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToU32RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToAway, ff.F64ToU32, f64, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f64_to_ui32, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToU32RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToEven, ff.F64ToI64, f64, a)
  PERF_TEST_SF(::softfloat_round_near_even, f64_to_i64, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToI64", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardPositive, ff.F64ToI64, f64, a)
  PERF_TEST_SF(::softfloat_round_max, f64_to_i64, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToI64RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardNegative, ff.F64ToI64, f64, a)
  PERF_TEST_SF(::softfloat_round_min, f64_to_i64, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToI64RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardZero, ff.F64ToI64, f64, a)
  PERF_TEST_SF(::softfloat_round_minMag, f64_to_i64, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToI64RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToAway, ff.F64ToI64, f64, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f64_to_i64, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToI64RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToEven, ff.F64ToU64, f64, a)
  PERF_TEST_SF(::softfloat_round_near_even, f64_to_ui64, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToU64", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardPositive, ff.F64ToU64, f64, a)
  PERF_TEST_SF(::softfloat_round_max, f64_to_ui64, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToU64RoundTowardPositive", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardNegative, ff.F64ToU64, f64, a)
  PERF_TEST_SF(::softfloat_round_min, f64_to_ui64, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToU64RoundTowardNegative", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTowardZero, ff.F64ToU64, f64, a)
  PERF_TEST_SF(::softfloat_round_minMag, f64_to_ui64, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToU64RoundTowardZero", (f64)ms_sf_float / (f64)ms_ff_float});

  PERF_TEST_FF_1(Vfpu::RoundingMode::kRoundTiesToAway, ff.F64ToU64, f64, a)
  PERF_TEST_SF(::softfloat_round_near_maxMag, f64_to_ui64, float64_t, f64, a, ::softfloat_roundingMode, true)
  result_vec.push_back({"F64ToU64RoundTiesToAway", (f64)ms_sf_float / (f64)ms_ff_float});

  // std::reverse(result_vec.begin(), result_vec.end());
  for (auto t : result_vec) {
    std::cout << "(" << std::get<1>(t) << "," << std::get<0>(t) << ")" << std::endl;
  }

  return 0;
}

/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 * Compile with: g++ -g -O3 -std=c++23 test_perf.cpp -lsoftfloat -lFloppyFloat
 ******************************************************************************/

#include <gtest/gtest.h>

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
#include "softfloat/softfloat.h"
}

using namespace std::placeholders;
using namespace FfUtils;

constexpr i32 kNumIterations = 100000000;
constexpr i32 kRngSeed = 42;

template <typename FT>
class FloatRng {
 public:
  FloatRng(int seed) : index_(0), engine_(seed), dist_(0, 1048576), values_() {
    for (size_t i = 0; i < size_; ++i) {
      FT sign = 1;  // (dist_(engine_) & 1) ? (FT)-1. : (FT)1.;
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

template <typename FT, typename FTT, typename FFFUNC, typename SFFUNC, int args, FloppyFloat::RoundingMode rm,
          bool flags = false>
void DoTest(std::string name, FloppyFloat& ff, FFFUNC ff_func, SFFUNC sf_func) {
  switch (rm) {
  case FloppyFloat::kRoundTiesToEven:
    ::softfloat_roundingMode = ::softfloat_round_near_even;
    break;
  case FloppyFloat::kRoundTowardZero:
    ::softfloat_roundingMode = ::softfloat_round_minMag;
    break;
  case FloppyFloat::kRoundTowardNegative:
    ::softfloat_roundingMode = ::softfloat_round_min;
    break;
  case FloppyFloat::kRoundTowardPositive:
    ::softfloat_roundingMode = ::softfloat_round_max;
    break;
  default:
    break;
  }

  ff.rounding_mode = rm;

  ::softfloat_exceptionFlags = 0xff;
  ff.inexact = true;
  ff.underflow = true;
  ff.invalid = true;
  ff.overflow = true;
  ff.division_by_zero = true;

  FloatRng<FT> float_rng(kRngSeed);
  std::cout << name << std::endl;

  FT valuefa{float_rng.Gen()};
  FTT valuesfa{std::bit_cast<typename FloatToUint<FT>::type>(valuefa)};
  FT valuefb{float_rng.Gen()};
  FTT valuesfb{std::bit_cast<typename FloatToUint<FT>::type>(valuefb)};
  FT valuefc{float_rng.Gen()};
  FTT valuesfc{std::bit_cast<typename FloatToUint<FT>::type>(valuefc)};

  std::chrono::steady_clock::time_point begin;
  std::chrono::steady_clock::time_point end;

  begin = std::chrono::steady_clock::now();
  for (size_t i = 0; i < kNumIterations; ++i) {
    if constexpr (args == 1) [[maybe_unused]]
      auto sf_result = sf_func(valuesfa);
    if constexpr (args == 2) [[maybe_unused]]
      auto sf_result = sf_func(valuesfa, valuesfb);
    if constexpr (args == 3) [[maybe_unused]]
      auto sf_result = sf_func(valuesfa, valuesfb, valuesfc);
    if constexpr (flags)
      ::softfloat_exceptionFlags = 0;

    valuesfc.v = valuesfb.v;
    valuesfb.v = valuesfa.v;
    valuesfa.v = std::bit_cast<typename FloatToUint<FT>::type>(float_rng.Gen());
  }
  float_rng.Reset();
  end = std::chrono::steady_clock::now();
  auto ms_sf_float = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  std::cout << "Milliseconds SoftFloat: " << ms_sf_float << std::endl;

  begin = std::chrono::steady_clock::now();
  for (size_t i = 0; i < kNumIterations; ++i) {
    // if(rm == FloppyFloat::kRoundTiesToEven) {
    //   [[maybe_unused]] auto ff_result = ff.Add<FT, FloppyFloat::kRoundTiesToEven>(valuefa, valuefb);
    // } else  if(rm == FloppyFloat::kRoundTiesToAway) {
    //   [[maybe_unused]] auto ff_result = ff.Add<FT, FloppyFloat::kRoundTiesToAway>(valuefa, valuefb);
    // } else  if(rm == FloppyFloat::kRoundTowardPositive) {
    //   [[maybe_unused]] auto ff_result = ff.Add<FT, FloppyFloat::kRoundTowardPositive>(valuefa, valuefb);
    // }
    if constexpr (args == 1) [[maybe_unused]]
      auto ff_result = ff_func(valuefa);
    if constexpr (args == 2) [[maybe_unused]]
      auto ff_result = ff_func(valuefa, valuefb);
    if constexpr (args == 3) [[maybe_unused]]
      auto ff_result = ff_func(valuefa, valuefb, valuefc);

    if constexpr (flags)
      ff.ClearFlags();

    valuefc = valuefb;
    valuefb = valuefa;
    valuefa = float_rng.Gen();
  }
  float_rng.Reset();
  end = std::chrono::steady_clock::now();
  auto ms_ff_float = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
  std::cout << "Milliseconds FloppyFloat: " << ms_ff_float << std::endl;

  f64 speedup = (f64)ms_sf_float / (f64)ms_ff_float;
  std::cout << "Speedup: " << speedup << std::endl << std::endl;
  result_vec.push_back({name, speedup});
}

int main() {
  FloppyFloat ff;
  ff.SetupTox86();

  {
    auto ff_func = std::bind(&FloppyFloat::Add<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_add, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToEven>("Add32Template",
    ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Add<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_add, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToAway>("Add32RMM", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Add<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_add, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardPositive>(
        "Add32RUPTemplate", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Add<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_add, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardPositive>(
        "Add32RUPSwitch", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Add<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_add, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardPositive>(
        "Add32RUPAlt", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Add<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_add, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardNegative>(
        "Add32RDN", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Add<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_add, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardZero>("Add32RTZ", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Sub<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_sub, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToEven>("Sub32", ff,
    ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Sub<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_sub, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToAway>("Sub32RMM", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Sub<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_sub, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardPositive>(
        "Sub32RUP", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Sub<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_sub, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardNegative>(
        "Sub32RDN", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Sub<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_sub, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardZero>("Sub32RTZ", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Mul<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_mul, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardPositive>(
        "Mul32RUP", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Mul<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_mul, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToEven>("Mul32", ff,
    ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Mul<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_mul, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardNegative>(
        "Mul32RDN", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Mul<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_mul, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardZero>("Mul32RTZ", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Fma<f32>, ff, _1, _2, _3);
    auto sf_func = std::bind(&::f32_mulAdd, _1, _2, _3);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 3, FloppyFloat::kRoundTiesToEven>("Fma32", ff,
    ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Fma<f32>, ff, _1, _2, _3);
    auto sf_func = std::bind(&::f32_mulAdd, _1, _2, _3);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 3, FloppyFloat::kRoundTowardPositive>(
        "Fma32RUP", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Fma<f32>, ff, _1, _2, _3);
    auto sf_func = std::bind(&::f32_mulAdd, _1, _2, _3);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 3, FloppyFloat::kRoundTowardNegative>(
        "Fma32RDN", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Fma<f32>, ff, _1, _2, _3);
    auto sf_func = std::bind(&::f32_mulAdd, _1, _2, _3);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 3, FloppyFloat::kRoundTowardZero>("Fma32RTZ", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Div<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_div, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToEven>("Div32", ff,
    ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Div<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_div, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardPositive>(
        "Div32RUP", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Div<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_div, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardNegative>(
        "Div32RDN", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Div<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_div, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTowardZero>("Div32RTZ", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Sqrt<f32>, ff, _1);
    auto sf_func = std::bind(&::f32_sqrt, _1);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTiesToEven>("Sqrt32", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Sqrt<f32>, ff, _1);
    auto sf_func = std::bind(&::f32_sqrt, _1);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardPositive>(
        "Sqrt32RUP", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Sqrt<f32>, ff, _1);
    auto sf_func = std::bind(&::f32_sqrt, _1);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardNegative>(
        "Sqrt32RDN", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Sqrt<f32>, ff, _1);
    auto sf_func = std::bind(&::f32_sqrt, _1);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardZero>("Sqrt32RTZ", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Add<f64>, ff, _1, _2);
    auto sf_func = std::bind(&::f64_add, _1, _2);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToEven>("Add64", ff,
    ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Sub<f64>, ff, _1, _2);
    auto sf_func = std::bind(&::f64_sub, _1, _2);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToEven>("Sub64", ff,
    ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Mul<f64>, ff, _1, _2);
    auto sf_func = std::bind(&::f64_mul, _1, _2);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToEven>("Mul64", ff,
    ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Fma<f64>, ff, _1, _2, _3);
    auto sf_func = std::bind(&::f64_mulAdd, _1, _2, _3);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 3, FloppyFloat::kRoundTiesToEven>("Fma64", ff,
    ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Div<f64>, ff, _1, _2);
    auto sf_func = std::bind(&::f64_div, _1, _2);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToEven>("Div64", ff,
    ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::Sqrt<f64>, ff, _1);
    auto sf_func = std::bind(&::f64_sqrt, _1);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTiesToEven>("Sqrt64", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<i32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI32), ff, _1);
    auto sf_func = std::bind(&::f32_to_i32, _1, ::softfloat_round_minMag, true);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardZero>("F32ToI32", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<i32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI32), ff, _1);
    auto sf_func = std::bind(&::f32_to_i32, _1, ::softfloat_round_near_even, true);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTiesToEven>("F32ToI32RNE", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<i32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI32), ff, _1);
    auto sf_func = std::bind(&::f32_to_i32, _1, ::softfloat_round_max, true);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardPositive>(
        "F32ToI32RUP", ff, ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<i32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI32), ff, _1);
    auto sf_func = std::bind(&::f32_to_i32, _1, ::softfloat_round_near_maxMag, true);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTiesToAway>("F32ToI32RAW", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<i64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI64), ff, _1);
    auto sf_func = std::bind(&::f32_to_i64, _1, ::softfloat_round_near_even, true);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardZero>("F32ToI64RNE", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<i64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToI64), ff, _1);
    auto sf_func = std::bind(&::f32_to_i64, _1, ::softfloat_round_minMag, true);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardZero>("F32ToI64", ff,
                                                                                                   ff_func, sf_func);
  }

  {
    auto ff_func = std::bind(static_cast<u32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU32), ff, _1);
    auto sf_func = std::bind(&::f32_to_ui32, _1, ::softfloat_round_minMag, true);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardZero>("F32ToU32", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<u32 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU32), ff, _1);
    auto sf_func = std::bind(&::f32_to_ui64, _1, ::softfloat_round_minMag, true);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardZero>("F32ToU64", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::F32ToF64, ff, _1);
    auto sf_func = std::bind(&::f32_to_f64, _1);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTiesToEven>("F32ToF64", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<i32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToI32), ff, _1);
    auto sf_func = std::bind(&::f64_to_i32, _1, ::softfloat_round_minMag, true);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardZero>("F64ToI32", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<i64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToI64), ff, _1);
    auto sf_func = std::bind(&::f64_to_i64, _1, ::softfloat_round_minMag, true);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardZero>("F64ToI64", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<u32 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU32), ff, _1);
    auto sf_func = std::bind(&::f64_to_ui32, _1, ::softfloat_round_minMag, true);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardZero>("F64ToU32", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<u64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU64), ff, _1);
    auto sf_func = std::bind(&::f64_to_ui64, _1, ::softfloat_round_minMag, true);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardZero>("F64ToU64", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<u64 (FloppyFloat::*)(f32)>(&FloppyFloat::F32ToU64), ff, _1);
    auto sf_func = std::bind(&::f32_to_ui64, _1, ::softfloat_round_near_maxMag, true);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTiesToAway>("F32ToU64Away", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(static_cast<u64 (FloppyFloat::*)(f64)>(&FloppyFloat::F64ToU64), ff, _1);
    auto sf_func = std::bind(&::f32_to_ui64, _1, ::softfloat_round_near_even, true);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTiesToEven>("F32ToU64Near", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::F32ToF64, ff, _1);
    auto sf_func = std::bind(&::f32_to_f64, _1);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 1, FloppyFloat::kRoundTowardZero>("F32ToF64", ff,
                                                                                                   ff_func, sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::LtSignaling<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_lt, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToAway>("Lt32", ff, ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::LtSignaling<f64>, ff, _1, _2);
    auto sf_func = std::bind(&::f64_lt, _1, _2);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToAway>("Lt64", ff, ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::EqQuiet<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_eq, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToAway>("Eq32", ff, ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::EqQuiet<f64>, ff, _1, _2);
    auto sf_func = std::bind(&::f64_eq, _1, _2);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToAway>("Eq64", ff, ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::LeSignaling<f32>, ff, _1, _2);
    auto sf_func = std::bind(&::f32_le, _1, _2);
    DoTest<f32, float32_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToAway>("Le32", ff, ff_func,
                                                                                                   sf_func);
  }
  {
    auto ff_func = std::bind(&FloppyFloat::LeSignaling<f64>, ff, _1, _2);
    auto sf_func = std::bind(&::f64_le, _1, _2);
    DoTest<f64, float64_t, decltype(ff_func), decltype(sf_func), 2, FloppyFloat::kRoundTiesToAway>("Le64", ff, ff_func,
                                                                                                   sf_func);
  }

  std::reverse(result_vec.begin(), result_vec.end());
  for (auto t : result_vec) {
    std::cout << "(" << std::get<1>(t) << "," << std::get<0>(t) << ")" << std::endl;
  }

  return 0;
}

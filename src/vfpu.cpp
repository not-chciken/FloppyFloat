/**************************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 **************************************************************************************************/

#include "vfpu.h"

#include <bit>
#include <cmath>

using namespace FfUtils;

template <>
void Vfpu::SetQnan<f16>(u16 val) {
  qnan16_ = std::bit_cast<f16>(val);
}

template <>
void Vfpu::SetQnan<f32>(u32 val) {
  qnan32_ = std::bit_cast<f32>(val);
}

template <>
void Vfpu::SetQnan<f64>(u64 val) {
  qnan64_ = std::bit_cast<f64>(val);
}

template <>
f16 Vfpu::GetQnan<f16>() {
  return qnan16_;
}

template <>
f32 Vfpu::GetQnan<f32>() {
  return qnan32_;
}

template <>
f64 Vfpu::GetQnan<f64>() {
  return qnan64_;
}

template <typename T>
T Vfpu::MaxLimit() {
  if constexpr (std::is_same_v<T, i32>) {
    return max_limit_i32_;
  } else if constexpr (std::is_same_v<T, u32>) {
    return max_limit_u32_;
  } else if constexpr (std::is_same_v<T, i64>) {
    return max_limit_i64_;
  } else if constexpr (std::is_same_v<T, u64>) {
    return max_limit_u64_;
  } else {
    static_assert(false, "Wrong type type");
  }
}

template i32 Vfpu::MaxLimit<i32>();
template i64 Vfpu::MaxLimit<i64>();
template u32 Vfpu::MaxLimit<u32>();
template u64 Vfpu::MaxLimit<u64>();

template <typename T>
T Vfpu::MinLimit() {
  if constexpr (std::is_same_v<T, i32>) {
    return min_limit_i32_;
  } else if constexpr (std::is_same_v<T, u32>) {
    return min_limit_u32_;
  } else if constexpr (std::is_same_v<T, i64>) {
    return min_limit_i64_;
  } else if constexpr (std::is_same_v<T, u64>) {
    return min_limit_u64_;
  } else {
    static_assert(false, "Wrong type type");
  }
}

template i32 Vfpu::MinLimit<i32>();
template i64 Vfpu::MinLimit<i64>();
template u32 Vfpu::MinLimit<u32>();
template u64 Vfpu::MinLimit<u64>();

template <typename T>
T Vfpu::NanLimit() {
  if constexpr (std::is_same_v<T, i32>) {
    return nan_limit_i32_;
  } else if constexpr (std::is_same_v<T, u32>) {
    return nan_limit_u32_;
  } else if constexpr (std::is_same_v<T, i64>) {
    return nan_limit_i64_;
  } else if constexpr (std::is_same_v<T, u64>) {
    return nan_limit_u64_;
  } else {
    static_assert(false, "Wrong type type");
  }
}

template i32 Vfpu::NanLimit<i32>();
template i64 Vfpu::NanLimit<i64>();
template u32 Vfpu::NanLimit<u32>();
template u64 Vfpu::NanLimit<u64>();

Vfpu::Vfpu() {
  SetQnan<f16>(0x7e00u);
  SetQnan<f32>(0x7fc00000u);
  SetQnan<f64>(0x7ff8000000000000ull);
  ClearFlags();
  tininess_before_rounding = false;
}

void Vfpu::ClearFlags() {
  invalid = false;
  division_by_zero = false;
  overflow = false;
  underflow = false;
  inexact = false;
}

void Vfpu::SetupToArm() {
  SetQnan<f16>(0x7e00u);
  SetQnan<f32>(0x7fc00000u);
  SetQnan<f64>(0x7ff8000000000000ull);
  tininess_before_rounding = true;
  invalid_fma = true;
  nan_propagation_scheme = kNanPropArm64DefaultNan;  // Shares the same NaN propagation as ARM.

  nan_limit_i32_ = 0;
  max_limit_i32_ = std::numeric_limits<i32>::max();
  min_limit_i32_ = std::numeric_limits<i32>::min();

  nan_limit_u32_ = 0;
  max_limit_u32_ = std::numeric_limits<u32>::max();
  min_limit_u32_ = std::numeric_limits<u32>::min();

  nan_limit_i64_ = 0;
  max_limit_i64_ = std::numeric_limits<i64>::max();
  min_limit_i64_ = std::numeric_limits<i64>::min();

  nan_limit_u64_ = 0;
  max_limit_u64_ = std::numeric_limits<u64>::max();
  min_limit_u64_ = std::numeric_limits<u64>::min();
}

void Vfpu::SetupToRiscv() {
  SetQnan<f16>(0x7e00u);
  SetQnan<f32>(0x7fc00000u);
  SetQnan<f64>(0x7ff8000000000000ull);
  tininess_before_rounding = false;
  invalid_fma = true;
  nan_propagation_scheme = kNanPropRiscv;

  nan_limit_i32_ = std::numeric_limits<i32>::max();
  max_limit_i32_ = std::numeric_limits<i32>::max();
  min_limit_i32_ = std::numeric_limits<i32>::min();

  nan_limit_u32_ = std::numeric_limits<u32>::max();
  max_limit_u32_ = std::numeric_limits<u32>::max();
  min_limit_u32_ = std::numeric_limits<u32>::min();

  nan_limit_i64_ = std::numeric_limits<i64>::max();
  max_limit_i64_ = std::numeric_limits<i64>::max();
  min_limit_i64_ = std::numeric_limits<i64>::min();

  nan_limit_u64_ = std::numeric_limits<u64>::max();
  max_limit_u64_ = std::numeric_limits<u64>::max();
  min_limit_u64_ = std::numeric_limits<u64>::min();
}

void Vfpu::SetupToX86() {
  SetQnan<f16>(0xfe00u);
  SetQnan<f32>(0xffc00000u);
  SetQnan<f64>(0xfff8000000000000ull);
  tininess_before_rounding = false;
  invalid_fma = false;
  nan_propagation_scheme = kNanPropX86sse;

  nan_limit_i32_ = std::numeric_limits<i32>::min();
  max_limit_i32_ = std::numeric_limits<i32>::min();
  min_limit_i32_ = std::numeric_limits<i32>::min();

  nan_limit_u32_ = std::numeric_limits<u32>::max();
  max_limit_u32_ = std::numeric_limits<u32>::max();
  min_limit_u32_ = std::numeric_limits<u32>::max();

  nan_limit_i64_ = std::numeric_limits<i64>::min();
  max_limit_i64_ = std::numeric_limits<i64>::min();
  min_limit_i64_ = std::numeric_limits<i64>::min();

  nan_limit_u64_ = std::numeric_limits<u64>::max();
  max_limit_u64_ = std::numeric_limits<u64>::max();
  min_limit_u64_ = std::numeric_limits<u64>::max();
}

Vfpu::RmGuard::RmGuard(Vfpu* vfpu, RoundingMode rm) : vfpu(vfpu) {
  old_rm = vfpu->rounding_mode;
  vfpu->rounding_mode = rm;
}

Vfpu::RmGuard::~RmGuard() {
  vfpu->rounding_mode = old_rm;
}
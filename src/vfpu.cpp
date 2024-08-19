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

void Vfpu::SetupToArm64() {
  SetQnan<f16>(0x7e00u);
  SetQnan<f32>(0x7fc00000u);
  SetQnan<f64>(0x7ff8000000000000ull);
  tininess_before_rounding = true;
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

void Vfpu::SetupTox86() {
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

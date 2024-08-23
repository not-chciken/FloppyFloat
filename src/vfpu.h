#pragma once
/**************************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 **************************************************************************************************/

#include "utils.h"

class Vfpu {
  static_assert(std::numeric_limits<FfUtils::f16>::is_iec559);
  static_assert(std::numeric_limits<FfUtils::f32>::is_iec559);
  static_assert(std::numeric_limits<FfUtils::f64>::is_iec559);
  static_assert(std::numeric_limits<FfUtils::f128>::is_iec559);

 public:
  // See IEEE 754-2019: 4.3 Rounding-direction attributes
  enum RoundingMode {
    kRoundTiesToEven,
    kRoundTiesToAway,
    kRoundTowardPositive,
    kRoundTowardNegative,
    kRoundTowardZero
  } rounding_mode;

  // Floating Exception Flags.
  bool invalid;
  bool division_by_zero;
  bool overflow;
  bool underflow;
  bool inexact;

  // kNanPropArm64DefaultNan => FPCR.DN = 1
  // kNanPropArm64 => FPCR.DN = 0
  enum NanPropagationSchemes { kNanPropRiscv, kNanPropX86sse, kNanPropArm64DefaultNan, kNanPropArm64 } nan_propagation_scheme;
  bool tininess_before_rounding = false;
  bool invalid_fma = true;  // If true, FMA raises invalid for "∞ × 0 + qNaN". See IEE 754 ("7.2 Invalid operation").

  Vfpu();

  void ClearFlags();

  template <typename FT>
  void SetQnan(typename FfUtils::FloatToUint<FT>::type val);
  template <typename FT>
  FT GetQnan();

  void SetupToArm();
  void SetupToRiscv();
  void SetupToX86();

 protected:
  FfUtils::f16 qnan16_;
  FfUtils::f32 qnan32_;
  FfUtils::f64 qnan64_;

  template <typename T>
  T MaxLimit();
  template <typename T>
  T MinLimit();
  template <typename T>
  T NanLimit();

  FfUtils::i32 nan_limit_i32_;
  FfUtils::i32 max_limit_i32_;
  FfUtils::i32 min_limit_i32_;
  FfUtils::u32 nan_limit_u32_;
  FfUtils::u32 max_limit_u32_;
  FfUtils::u32 min_limit_u32_;
  FfUtils::i64 nan_limit_i64_;
  FfUtils::i64 max_limit_i64_;
  FfUtils::i64 min_limit_i64_;
  FfUtils::u64 nan_limit_u64_;
  FfUtils::u64 max_limit_u64_;
  FfUtils::u64 min_limit_u64_;

  struct RmGuard {
    RoundingMode old_rm;
    Vfpu* vfpu;
    RmGuard(Vfpu* vfpu, RoundingMode rm);
    ~RmGuard();
  };
};
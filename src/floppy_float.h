#pragma once
/**************************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 *
 * A faster than soft float library to simulate floating point instructions.
 * Based on: https://www.chciken.com/simulation/2023/11/12/fast-floating-point-simulation.html
 **************************************************************************************************/

#include "utils.h"

static_assert(std::numeric_limits<f16>::is_iec559);
static_assert(std::numeric_limits<f32>::is_iec559);
static_assert(std::numeric_limits<f64>::is_iec559);
static_assert(std::numeric_limits<f128>::is_iec559);

class FloppyFloat {
 public:
  // See IEEE 754-2019: 4.3 Rounding-direction attributes
  enum RoundingMode {
    kRoundTiesToEven,
    kRoundTiesToAway,
    kRoundTowardPositive,
    kRoundTowardNegative,
    kRoundTowardZero
  };

  // Floating Exception Flags.
  bool invalid;
  bool division_by_zero;
  bool overflow;
  bool underflow;
  bool inexact;

  enum NanPropagationSchemes { kNanPropRiscv, kNanPropX86sse } nan_propagation_scheme;
  enum ConversionLimits { kLimitsRiscv, kLimitsx86, kLimitsArm } conversion_limits;
  bool tininess_before_rounding = false;
  bool invalid_fma = true; // If true, FMA raises invalid for "∞ × 0 + qNaN". See IEE 754 ("7.2 Invalid operation").

  FloppyFloat();

  void ClearFlags();

  template <typename FT>
  void SetQnan(typename FloatToUint<FT>::type val);
  template <typename FT>
  constexpr FT GetQnan();

  template <typename FT, RoundingMode rm = kRoundTiesToEven>
  FT Add(FT a, FT b);
  template <typename FT, RoundingMode rm = kRoundTiesToEven>
  FT Sub(FT a, FT b);
  template <typename FT, RoundingMode rm = kRoundTiesToEven>
  FT Mul(FT a, FT b);
  template <typename FT, RoundingMode rm = kRoundTiesToEven>
  FT Div(FT a, FT b);
  template <typename FT, RoundingMode rm = kRoundTiesToEven>
  FT Sqrt(FT a);
  template <typename FT, RoundingMode rm = kRoundTiesToEven>
  FT Fma(FT a, FT b, FT c);

  template <typename FT, bool quiet>
  bool Eq(FT a, FT b);
  template <typename FT, bool quiet>
  bool Le(FT a, FT b);
  template <typename FT, bool quiet>
  bool Lt(FT a, FT b);

  template <typename FT>
  FT Maxx86(FT a, FT b);  // x86 legacy maximum (see "maxss/maxsd");
  template <typename FT>
  FT Minx86(FT a, FT b);  // x86 legacy minimum (see "minss/minsd");
  template <typename FT>
  FT MaximumNumber(FT a, FT b);
  template <typename FT>
  FT MinimumNumber(FT a, FT b);

  template <typename FT>
  u32 Class(FT a);

  template <RoundingMode rm = kRoundTiesToEven>
  i32 F32ToI32(f32 a);
  template <RoundingMode rm = kRoundTiesToEven>
  i64 F32ToI64(f32 a);
  template <RoundingMode rm = kRoundTiesToEven>
  u32 F32ToU32(f32 a);
  template <RoundingMode rm = kRoundTiesToEven>
  u64 F32ToU64(f32 a);
  template <RoundingMode rm = kRoundTiesToEven>
  f16 F32ToF16(f32 a);
  f64 F32ToF64(f32 a);

  template <RoundingMode rm = kRoundTiesToEven>
  i32 F64ToI32(f64 a);
  template <RoundingMode rm = kRoundTiesToEven>
  i64 F64ToI64(f64 a);
  template <RoundingMode rm = kRoundTiesToEven>
  u32 F64ToU32(f64 a); //TODO
  // template <RoundingMode rm = kRoundTiesToEven>
  // u64 F64ToU64(f64 a); TODO
  // f16 F64ToF16(f64 a); TODO
  // f64 F64ToF64(f64 a); TODO

  void SetupToArm();
  void SetupToRiscv();
  void SetupTox86();

 protected:
  template <typename FT, typename TFT, RoundingMode rm>
  constexpr FT RoundResult(TFT residual, FT result);
  template <typename FT>
  constexpr FT PropagateNan(FT a, FT b);
  constexpr f64 PropagateNan(f32 a);

  f16 qnan16_;
  f32 qnan32_;
  f64 qnan64_;
};
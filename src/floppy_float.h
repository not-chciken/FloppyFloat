#pragma once
/**************************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 *
 * A faster than soft float library to simulate floating point instructions.
 * Based on: https://www.chciken.com/simulation/2023/11/12/fast-floating-point-simulation.html
 **************************************************************************************************/

#include "utils.h"

class FloppyFloat {
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
  };

  // Floating Exception Flags.
  bool invalid;
  bool division_by_zero;
  bool overflow;
  bool underflow;
  bool inexact;

  // kNanPropArm64DefaultNan => FPCR.DN = 1
  // kNanPropArm64 => FPCR.DN = 0
  enum NanPropagationSchemes {
    kNanPropRiscv,
    kNanPropX86sse,
    kNanPropArm64DefaultNan,
    kNanPropArm64
  } nan_propagation_scheme;
  bool tininess_before_rounding = false;
  bool invalid_fma = true;  // If true, FMA raises invalid for "∞ × 0 + qNaN". See IEE 754 ("7.2 Invalid operation").

  FloppyFloat();

  void ClearFlags();

  template <typename FT>
  void SetQnan(typename FfUtils::FloatToUint<FT>::type val);
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
  FfUtils::u32 Class(FT a);

  template <RoundingMode rm = kRoundTiesToEven>
  FfUtils::i32 F32ToI32(FfUtils::f32 a);
  template <RoundingMode rm = kRoundTiesToEven>
  FfUtils::i64 F32ToI64(FfUtils::f32 a);
  template <RoundingMode rm = kRoundTiesToEven>
  FfUtils::u32 F32ToU32(FfUtils::f32 a);
  template <RoundingMode rm = kRoundTiesToEven>
  FfUtils::u64 F32ToU64(FfUtils::f32 a);
  template <RoundingMode rm = kRoundTiesToEven>
  FfUtils::f16 F32ToF16(FfUtils::f32 a);
  FfUtils::f64 F32ToF64(FfUtils::f32 a);

  template <RoundingMode rm = kRoundTiesToEven>
  FfUtils::i32 F64ToI32(FfUtils::f64 a);
  template <RoundingMode rm = kRoundTiesToEven>
  FfUtils::i64 F64ToI64(FfUtils::f64 a);
  template <RoundingMode rm = kRoundTiesToEven>
  FfUtils::u32 F64ToU32(FfUtils::f64 a);
  template <RoundingMode rm = kRoundTiesToEven>
  FfUtils::u64 F64ToU64(FfUtils::f64 a);
  // f16 F64ToF16(f64 a); TODO
  // f64 F64ToF64(f64 a); TODO

  // template <RoundingMode rm = kRoundTiesToEven>
  // FfUtils::f16 I32ToF16(FfUtils::i32 a); // TODO: implement
  template <RoundingMode rm = kRoundTiesToEven>
  FfUtils::f32 I32ToF32(FfUtils::i32 a); // TODO: implement
  FfUtils::f64 I32ToF64(FfUtils::i32 a); // TODO: implement

  // template <RoundingMode rm = kRoundTiesToEven>
  // FfUtils::f16 U32ToF16(FfUtils::u32 a); // TODO: implement
  // template <RoundingMode rm = kRoundTiesToEven>
  // FfUtils::f32 U32ToF32(FfUtils::u32 a); // TODO: implement
  // FfUtils::f64 U32ToF64(FfUtils::u32 a); // TODO: implement

  // template <RoundingMode rm = kRoundTiesToEven>
  // FfUtils::f16 I64ToF16(FfUtils::i64 a); // TODO: implement
  // template <RoundingMode rm = kRoundTiesToEven>
  // FfUtils::f32 I64ToF32(FfUtils::i64 a); // TODO: implement
  // FfUtils::f64 I64ToF64(FfUtils::i64 a); // TODO: implement

  // template <RoundingMode rm = kRoundTiesToEven>
  // FfUtils::f16 U64ToF16(FfUtils::u64 a); // TODO: implement
  // template <RoundingMode rm = kRoundTiesToEven>
  // FfUtils::f32 U64ToF32(FfUtils::u64 a); // TODO: implement
  // FfUtils::f64 U64ToF64(FfUtils::u64 a); // TODO: implement

  void SetupToArm64();
  void SetupToRiscv();
  void SetupTox86();

 protected:
  template <typename FT, typename TFT, RoundingMode rm>
  constexpr FT RoundResult(TFT residual, FT result);
  template <typename FT>
  constexpr FT PropagateNan(FT a, FT b);
  constexpr FfUtils::f64 PropagateNan(FfUtils::f32 a);

  FfUtils::f16 qnan16_;
  FfUtils::f32 qnan32_;
  FfUtils::f64 qnan64_;

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
};
#pragma once
/**************************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 *
 * A faster than soft float library to simulate floating point instructions.
 * Based on: https://www.chciken.com/simulation/2023/11/12/fast-floating-point-simulation.html
 **************************************************************************************************/

#include "soft_float.h"
#include "utils.h"
class FloppyFloat : public SoftFloat {
 public:
  FloppyFloat();

  template <typename FT>
  void SetQnan(typename FfUtils::FloatToUint<FT>::type val);
  template <typename FT>
  constexpr FT GetQnan();

  template <typename FT, RoundingMode rm>
  FT Add(FT a, FT b);
  template <typename FT>
  FT Add(FT a, FT b);

  template <typename FT, RoundingMode rm>
  FT Sub(FT a, FT b);
  template <typename FT>
  FT Sub(FT a, FT b);

  template <typename FT, RoundingMode rm>
  FT Mul(FT a, FT b);
  template <typename FT>
  FT Mul(FT a, FT b);

  template <typename FT, RoundingMode rm>
  FT Div(FT a, FT b);
  template <typename FT>
  FT Div(FT a, FT b);

  template <typename FT, RoundingMode rm>
  FT Sqrt(FT a);
  template <typename FT>
  FT Sqrt(FT a);

  template <typename FT, RoundingMode rm>
  FT Fma(FT a, FT b, FT c);
  template <typename FT>
  FT Fma(FT a, FT b, FT c);

  template <typename FT>
  bool EqQuiet(FT a, FT b);
  template <typename FT>
  bool LeQuiet(FT a, FT b);
  template <typename FT>
  bool LtQuiet(FT a, FT b);
  template <typename FT>
  bool EqSignaling(FT a, FT b);
  template <typename FT>
  bool LeSignaling(FT a, FT b);
  template <typename FT>
  bool LtSignaling(FT a, FT b);

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

  FfUtils::f32 F16ToF32(FfUtils::f16 a);
  FfUtils::f64 F16ToF64(FfUtils::f16 a);

  template <RoundingMode rm>
  FfUtils::i32 F32ToI32(FfUtils::f32 a);
  FfUtils::i32 F32ToI32(FfUtils::f32 a);

  template <RoundingMode rm>
  FfUtils::i64 F32ToI64(FfUtils::f32 a);
  FfUtils::i64 F32ToI64(FfUtils::f32 a);

  template <RoundingMode rm>
  FfUtils::u32 F32ToU32(FfUtils::f32 a);
  FfUtils::u32 F32ToU32(FfUtils::f32 a);

  template <RoundingMode rm>
  FfUtils::u64 F32ToU64(FfUtils::f32 a);
  FfUtils::u64 F32ToU64(FfUtils::f32 a);

  template <RoundingMode rm>
  FfUtils::f16 F32ToF16(FfUtils::f32 a);
  FfUtils::f16 F32ToF16(FfUtils::f32 a);

  FfUtils::f64 F32ToF64(FfUtils::f32 a);

  template <RoundingMode rm>
  FfUtils::f16 F64ToF16(FfUtils::f64 a);
  template <RoundingMode rm>
  FfUtils::f32 F64ToF32(FfUtils::f64 a);

  template <RoundingMode rm>
  FfUtils::i32 F64ToI32(FfUtils::f64 a);
  FfUtils::i32 F64ToI32(FfUtils::f64 a);

  template <RoundingMode rm>
  FfUtils::i64 F64ToI64(FfUtils::f64 a);
  FfUtils::i64 F64ToI64(FfUtils::f64 a);

  template <RoundingMode rm>
  FfUtils::u32 F64ToU32(FfUtils::f64 a);
  FfUtils::u32 F64ToU32(FfUtils::f64 a);

  template <RoundingMode rm>
  FfUtils::u64 F64ToU64(FfUtils::f64 a);
  FfUtils::u64 F64ToU64(FfUtils::f64 a);

  template <RoundingMode rm>
  FfUtils::f16 I32ToF16(FfUtils::i32 a);
  FfUtils::f16 I32ToF16(FfUtils::i32 a);
  template <RoundingMode rm>
  FfUtils::f32 I32ToF32(FfUtils::i32 a);
  FfUtils::f32 I32ToF32(FfUtils::i32 a);
  FfUtils::f64 I32ToF64(FfUtils::i32 a);

  template <RoundingMode rm>
  FfUtils::f32 U32ToF32(FfUtils::u32 a);
  FfUtils::f32 U32ToF32(FfUtils::u32 a);
  FfUtils::f64 U32ToF64(FfUtils::u32 a);

  template <RoundingMode rm>
  FfUtils::f32 U64ToF32(FfUtils::u64 a);
  FfUtils::f32 U64ToF32(FfUtils::u64 a);

 protected:
  template <typename FT, typename TFT, RoundingMode rm>
  constexpr FT RoundResult(TFT residual, FT result);

  template <typename TFROM, typename TTO>
  constexpr TTO PropagateNan(TFROM a);

  template <typename FT>
  constexpr FT PropagateNan(FT a, FT b);

  template <typename FT>
  constexpr FT PropagateNan(FT a, FT b, FT c);

  template <typename FT, FloppyFloat::RoundingMode rm>
  constexpr auto UpMul(FT a, FT b, FT& c);
  template <typename FT, FloppyFloat::RoundingMode rm>
  constexpr auto UpDiv(FT a, FT b, FT& c);
  template <typename FT, FloppyFloat::RoundingMode rm>
  constexpr auto UpSqrt(FT a, FT& b);
  template <typename FT, FloppyFloat::RoundingMode rm>
  constexpr auto UpFma(FT a, FT b, FT c, FT& d);

  //constexpr FfUtils::f64 PropagateNan(FfUtils::f32 a);
};
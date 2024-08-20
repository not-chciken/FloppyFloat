#pragma once
/**************************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 *
 * A faster than soft float library to simulate floating point instructions.
 * Based on: https://www.chciken.com/simulation/2023/11/12/fast-floating-point-simulation.html
 **************************************************************************************************/

#include "utils.h"
#include "soft_float.h"
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
  template <typename FT, RoundingMode rm>
  FT Mul(FT a, FT b);
  template <typename FT, RoundingMode rm>
  FT Div(FT a, FT b);
  template <typename FT, RoundingMode rm>
  FT Sqrt(FT a);
  template <typename FT, RoundingMode rm>
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

  template <RoundingMode rm>
  FfUtils::i32 F32ToI32(FfUtils::f32 a);
  template <RoundingMode rm>
  FfUtils::i64 F32ToI64(FfUtils::f32 a);
  template <RoundingMode rm>
  FfUtils::u32 F32ToU32(FfUtils::f32 a);
  template <RoundingMode rm>
  FfUtils::u64 F32ToU64(FfUtils::f32 a);
  template <RoundingMode rm>
  FfUtils::f16 F32ToF16(FfUtils::f32 a);
  FfUtils::f64 F32ToF64(FfUtils::f32 a);

  template <RoundingMode rm>
  FfUtils::i32 F64ToI32(FfUtils::f64 a);
  template <RoundingMode rm>
  FfUtils::i64 F64ToI64(FfUtils::f64 a);
  template <RoundingMode rm>
  FfUtils::u32 F64ToU32(FfUtils::f64 a);
  template <RoundingMode rm>
  FfUtils::u64 F64ToU64(FfUtils::f64 a);
  // f16 F64ToF16(f64 a); TODO
  // f64 F64ToF64(f64 a); TODO

  template <RoundingMode rm>
  FfUtils::f16 I32ToF16(FfUtils::i32 a);
  FfUtils::f16 I32ToF16(FfUtils::i32 a);
  template <RoundingMode rm>
  FfUtils::f32 I32ToF32(FfUtils::i32 a);
  FfUtils::f32 I32ToF32(FfUtils::i32 a);
  FfUtils::f64 I32ToF64(FfUtils::i32 a);

  // template <RoundingMode rm>
  // FfUtils::f16 U32ToF16(FfUtils::u32 a); // TODO: implement
  template <RoundingMode rm>
  FfUtils::f32 U32ToF32(FfUtils::u32 a); // TODO: implement
  FfUtils::f32 U32ToF32(FfUtils::u32 a);
  FfUtils::f64 U32ToF64(FfUtils::u32 a);

  // template <RoundingMode rm>
  // FfUtils::f16 I64ToF16(FfUtils::i64 a); // TODO: implement
  // template <RoundingMode rm>
  // FfUtils::f32 I64ToF32(FfUtils::i64 a); // TODO: implement
  // FfUtils::f64 I64ToF64(FfUtils::i64 a); // TODO: implement

  // template <RoundingMode rm>
  // FfUtils::f16 U64ToF16(FfUtils::u64 a); // TODO: implement
  template <RoundingMode rm>
  FfUtils::f32 U64ToF32(FfUtils::u64 a);
  FfUtils::f32 U64ToF32(FfUtils::u64 a);
  // FfUtils::f64 U64ToF64(FfUtils::u64 a); // TODO: implement

 protected:
  template <typename FT, typename TFT, RoundingMode rm>
  constexpr FT RoundResult(TFT residual, FT result);
  template <typename FT>
  constexpr FT PropagateNan(FT a, FT b);
  constexpr FfUtils::f64 PropagateNan(FfUtils::f32 a);
};
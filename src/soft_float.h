#pragma once
/**************************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 *
 * Soft float fall back inspired by F. Bellard's SoftFP.
 **************************************************************************************************/

#include "utils.h"
#include "vfpu.h"

class SoftFloat : public Vfpu {

 public:
  SoftFloat();

  template <typename FT>
  FT Add(FT a, FT b);
  template <typename FT>
  FT Sub(FT a, FT b);
  template <typename FT>
  FT Mul(FT a, FT b);
  template <typename FT>
  FT Div(FT a, FT b);
  template <typename FT>
  FT Sqrt(FT a);
  template <typename FT>
  FT Fma(FT a, FT b, FT c);

  FfUtils::i32 F16ToI32(FfUtils::f16 a);
  FfUtils::i64 F16ToI64(FfUtils::f16 a);
  FfUtils::u32 F16ToU32(FfUtils::f16 a);
  FfUtils::u64 F16ToU64(FfUtils::f16 a);

  FfUtils::i32 F32ToI32(FfUtils::f32 a);
  FfUtils::i64 F32ToI64(FfUtils::f32 a);
  FfUtils::u32 F32ToU32(FfUtils::f32 a);
  FfUtils::u64 F32ToU64(FfUtils::f32 a);

  FfUtils::i32 F64ToI32(FfUtils::f64 a);
  FfUtils::i64 F64ToI64(FfUtils::f64 a);
  FfUtils::u32 F64ToU32(FfUtils::f64 a);
  FfUtils::u64 F64ToU64(FfUtils::f64 a);

  FfUtils::f16 F32ToF16(FfUtils::f32 a);
  FfUtils::f16 F64ToF16(FfUtils::f64 a);
  FfUtils::f32 F64ToF32(FfUtils::f64 a);

  FfUtils::f16 I32ToF16(FfUtils::i32 a);
  FfUtils::f32 I32ToF32(FfUtils::i32 a);
  FfUtils::f64 I32ToF64(FfUtils::i32 a);

  FfUtils::f16 U32ToF16(FfUtils::u32 a);
  FfUtils::f32 U32ToF32(FfUtils::u32 a);
  FfUtils::f64 U32ToF64(FfUtils::u32 a);

  FfUtils::f16 I64ToF16(FfUtils::i64 a);
  FfUtils::f32 I64ToF32(FfUtils::i64 a);
  FfUtils::f64 I64ToF64(FfUtils::i64 a);

  FfUtils::f16 U64ToF16(FfUtils::u64 a);
  FfUtils::f32 U64ToF32(FfUtils::u64 a);
  FfUtils::f64 U64ToF64(FfUtils::u64 a);

  protected:
  template <typename FT, typename UT>
  FT RoundPack(bool a_sign, FfUtils::i32 a_exp, UT a_mant);

  template <typename FT, typename UT>
  constexpr UT NormalizeSubnormal(FfUtils::i32& exp, UT mant);

  template <typename FT, typename UT>
  constexpr FT Normalize(FfUtils::u32 a_sign, FfUtils::i32 a_exp, UT a_mant);
  template <typename FT, typename UT>
  constexpr FT Normalize(FfUtils::u32 a_sign, FfUtils::i32 a_exp, UT a_mant0, UT a_mant1);

  template<typename TFROM, typename TTO>
  TTO FToF(TFROM a);
  template<typename TFROM, typename TTO>
  TTO FToI(TFROM a);
  template<typename TFROM, typename TTO>
  TTO IToF(TFROM a);

  template <typename FT>
  constexpr FT PropagateNan(FT a, FT b);
  template <typename FT>
  constexpr FT PropagateNan(FT a, FT b, FT c);
};

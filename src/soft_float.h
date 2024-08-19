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

  FfUtils::f16 F32ToF16(FfUtils::f32 a);
  FfUtils::f16 F64ToF16(FfUtils::f64 a);
  FfUtils::f32 F64ToF32(FfUtils::f64 a);

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
};

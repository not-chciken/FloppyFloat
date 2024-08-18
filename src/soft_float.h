#pragma once
/**************************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 *
 * Soft float fall back inspired by F. Bellard's SoftFP.
 **************************************************************************************************/

#include "utils.h"

class SoftFloat {

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


  SoftFloat();

  template <typename FT>
  FT Mul(FT a, FT b);
};

template <typename FT>
inline FT SoftFloat::Mul(FT a, FT b) {
    using UT = FloatToUint<FT>::type;
    using IT = FloatToUint<FT>::type;
    bool a_sign = std::signbit(a);
    bool b_sign = std::signbit(b);
    bool r_sign = a_sign ^ b_sign;
    u32 a_exp = GetExponent<FT>(a);
    u32 b_exp = GetExponent<FT>(b);
    auto a_mant = GetSignificand<FT>(a);
    auto b_mant = GetSignificand<FT>(b);

    if (a_exp == MaxExponent<FT>() || b_exp == MaxExponent<FT>()) {
        if (IsNan(a) || IsNan(b)) {
            if (IsSnan(a) || IsSnan(b))
                invalid = true;
            return GetQnan<FT>(); // TODO nan prop
        } else {
            if ((a_exp == MaxExponent<FT>() &&
                 (b_exp == 0 && b_mant == 0)) ||
                (b_exp == MaxExponent<FT>() &&
                 (a_exp == 0 && a_mant == 0))) {
                invalid = true;
                return GetQnan<FT>(); // TODO nan prop
            } else {
                return FloatFrom3Tuple<FT>(r_sign, MaxExponent<FT>(), 0u);
            }
        }
    }

    if (a_exp == 0) {
        if (a_mant == 0)
            return FloatFrom3Tuple<F>(r_sign, 0, 0u);
        a_mant = normalize_subnormal32(a_exp, a_mant);
    } else {
        a_mant |= (u32)1 << NumMantissaBits<FT>();
    }

    if (b_exp == 0) {
        if (b_mant == 0)
            return FloatFrom3Tuple<FT>(r_sign, 0, (u32)0);
        b_mant = normalize_subnormal32(b_exp, b_mant);
    } else {
        b_mant |= (u32)1 << NumMantissaBits<FT>();
    }

    i32 r_exp = a_exp + b_exp - (1 << (NumExponentBits<FT>() - 1)) + 2;

    auto [lo, hi] = umul64(a_mant << float32::ROUND_BITS,
                           b_mant << (float32::ROUND_BITS + 1));

    u32 r_mant = hi | !!lo;
    return normalize32(r_sign, r_exp, r_mant);
}

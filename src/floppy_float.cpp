/**************************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 **************************************************************************************************/

#include "floppy_float.h"

#include <bit>

template <typename FT>
constexpr FT TwoSum(FT a, FT b, FT c) {
  FT ad = c - b;
  FT bd = c - a;
  FT da = a - ad;
  FT db = b - bd;
  FT r = da + db;
  return r;
}

template <typename FT>
constexpr TwiceWidthType<FT>::type UpMul(FT a, FT b, FT c) {
  auto da = static_cast<TwiceWidthType<FT>::type>(a);
  auto db = static_cast<TwiceWidthType<FT>::type>(b);
  auto dc = static_cast<TwiceWidthType<FT>::type>(c);
  auto r = da * db - dc;
  return r;
}

template <typename FT>
constexpr TwiceWidthType<FT> UpDiv(FT a, FT b, FT c) {
  auto da = static_cast<TwiceWidthType<FT>>(a);
  auto db = static_cast<TwiceWidthType<FT>>(b);
  auto dc = static_cast<TwiceWidthType<FT>>(c);
  return dc * db - da;
}

template <typename FT>
constexpr TwiceWidthType<FT> UpSqrt(FT a, FT b) {
  auto da = static_cast<TwiceWidthType<FT>>(a);
  auto db = static_cast<TwiceWidthType<FT>>(b);
  auto r = db * db - da;
  return r;
}

template <>
void FloppyFloat::SetQnan<f16>(u16 val) {
  qnan16_ = std::bit_cast<f16>(val);
}

template <>
void FloppyFloat::SetQnan<f32>(u32 val) {
  qnan32_ = std::bit_cast<f32>(val);
}

template <>
void FloppyFloat::SetQnan<f64>(u64 val) {
  qnan64_ = std::bit_cast<f64>(val);
}

template <>
constexpr f16 FloppyFloat::GetQnan<f16>() {
  return qnan16_;
}

template <>
constexpr f32 FloppyFloat::GetQnan<f32>() {
  return qnan32_;
}

template <>
constexpr f64 FloppyFloat::GetQnan<f64>() {
  return qnan64_;
}

FloppyFloat::FloppyFloat() {
  SetQnan<f16>(0x7e00u);
  SetQnan<f32>(0x7fc00000u);
  SetQnan<f64>(0x7ff8000000000000ull);
}

void FloppyFloat::ClearFlags() {
  invalid = false;
  division_by_zero = false;
  overflow = false;
  underflow = false;
  inexact = false;
}

template <typename FT, FloppyFloat::RoundingMode rm>
constexpr FT RoundInf(FT result) {
  if constexpr (rm == FloppyFloat::kRoundTiesToEven) {
    return result;
  }
  if constexpr (rm == FloppyFloat::kRoundTowardPositive) {
    return IsNegInf(result) ? nl<FT>::lowest() : result;
  }
  if constexpr (rm == FloppyFloat::kRoundTowardNegative) {
    return IsPosInf(result) ? nl<FT>::max() : result;
  }
  if constexpr (rm == FloppyFloat::kRoundTowardZero) {
    return IsNegInf(result) ? nl<FT>::lowest() : nl<FT>::max();
  }
}

template <typename FT, typename TFT, FloppyFloat::RoundingMode rm>
constexpr FT FloppyFloat::RoundResult([[maybe_unused]] TFT residual, FT result) {
  if constexpr (rm == kRoundTiesToEven) {
    // Nothing to do.
  }
  if constexpr (rm == kRoundTowardPositive) {
    if (residual > 0.) {
      auto uresult = std::bit_cast<typename FloatToUint<FT>::type>(result);
      uresult += result > 0. ? 1 : -1;  // Next up of result cannot be 0.
      result = std::bit_cast<FT>(uresult);
      overflow = (result == nl<FT>::infinity()) ? true : overflow;
    }
  }
  if constexpr (rm == kRoundTowardNegative) {
    if (residual < 0.) {
      auto uresult = std::bit_cast<typename FloatToUint<FT>::type>(result);
      uresult -= result > 0. ? 1 : -1;  // Nextdown of result cannot be 0.
      result = std::bit_cast<FT>(uresult);
      overflow = (result == -nl<FT>::infinity()) ? true : overflow;
    }
  }
  if constexpr (rm == kRoundTowardZero) {
    if (residual > 0. && result < 0.) {
      auto uresult = std::bit_cast<typename FloatToUint<FT>::type>(result);
      uresult += result > 0. ? 1 : -1;  // Nextup of result cannot be 0.
      result = std::bit_cast<FT>(uresult);
      overflow = (result == nl<FT>::infinity()) ? true : overflow;
    } else if (residual < 0 && result > 0) {  // Fix a round-up.
      auto uresult = std::bit_cast<typename FloatToUint<FT>::type>(result);
      uresult -= result > 0. ? 1 : -1;  // Nextdown of result cannot be 0.
      result = std::bit_cast<FT>(uresult);
      overflow = (result == -nl<FT>::infinity()) ? true : overflow;
    }
  }
  return result;
}

template <typename FT>
constexpr FT SetQuietBit(FT a) {
  auto au = std::bit_cast<typename FloatToUint<FT>::type>(a);
  return std::bit_cast<FT>((decltype(au))(QuietBit<FT>::u | au));
}

template <typename FT>
constexpr FT PropagateNan(FT a, FT b) {
  // TODO: This is x86 specific. Also test other!
  if (IsSnan(a) || IsSnan(b)) {
    if (IsSnan(a))
      return SetQuietBit(a);
  }
  return IsNan(a) ? SetQuietBit(a) : SetQuietBit(b);
}

template <typename FT, FloppyFloat::RoundingMode rm>
FT FloppyFloat::Add(FT a, FT b) {
  FT c = a + b;

  if (IsInfOrNan(c)) [[unlikely]] {
    if (IsInf(c)) {
      if (!IsInf(a) && !IsInf(b)) {
        overflow = true;
        inexact = true;
        c = RoundInf<FT, rm>(c);
      }
      return c;
    } else {  // NaN case.
      if (IsInf(a) && IsInf(b)) {
        invalid = true;
        return GetQnan<FT>();
      }
      if (IsSnan(a) || IsSnan(b))
        invalid = true;
      if (IsNan(a) || IsNan(b))
        return propagate_nan ? PropagateNan<FT>(a, b) : GetQnan<FT>();
    }
  }

  // See: IEEE 754-2019: 6.3 The sign bit
  if constexpr (rm == kRoundTowardNegative) {
    if (IsPosZero(c)) {
      if (IsNeg(a) || IsNeg(b))
        c = -c;
    }
  }

  if constexpr (rm == kRoundTiesToEven) {
    if (!inexact) [[unlikely]] {
      FT r = TwoSum<FT>(a, b, c);
      if (r != 0)
        inexact = true;
    }
  } else {
    FT r = TwoSum(a, b, c);
    if (r != 0) {
      inexact = true;
      RoundResult<FT, FT, rm>(r, c);
    }
  }

  return c;
}

template f16 FloppyFloat::Add<f16, FloppyFloat::kRoundTiesToEven>(f16 a, f16 b);
template f16 FloppyFloat::Add<f16, FloppyFloat::kRoundTowardPositive>(f16 a, f16 b);
template f16 FloppyFloat::Add<f16, FloppyFloat::kRoundTowardNegative>(f16 a, f16 b);
template f16 FloppyFloat::Add<f16, FloppyFloat::kRoundTowardZero>(f16 a, f16 b);

template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTiesToEven>(f32 a, f32 b);
template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTowardPositive>(f32 a, f32 b);
template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTowardNegative>(f32 a, f32 b);
template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTowardZero>(f32 a, f32 b);

template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTiesToEven>(f64 a, f64 b);
template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTowardPositive>(f64 a, f64 b);
template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTowardNegative>(f64 a, f64 b);
template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTowardZero>(f64 a, f64 b);

template <typename FT, FloppyFloat::RoundingMode rm>
FT FloppyFloat::Sub(FT a, FT b) {
  FT c = a - b;

  if (IsInfOrNan(c)) [[unlikely]] {
    if (IsInf(c)) {
      if (!IsInf(a) && !IsInf(b)) {
        overflow = true;
        inexact = true;
        c = RoundInf<FT, rm>(c);
      }
      return c;
    } else {  // NaN case.
      if (IsInf(a) && IsInf(b)) {
        invalid = true;
        return GetQnan<FT>();
      }
      if (IsSnan(a) || IsSnan(b))
        invalid = true;
      if (IsNan(a) || IsNan(b))
        return propagate_nan ? PropagateNan<FT>(a, b) : GetQnan<FT>();
    }
  }

  // See: IEEE 754-2019: 6.3 The sign bit
  if constexpr (rm == kRoundTowardNegative) {
    if (IsPosZero(c)) {
      if (IsNeg(a) || IsPos(b))
        c = -c;
    }
  }

  if constexpr (rm == kRoundTiesToEven) {
    if (!inexact) [[unlikely]] {
      FT r = TwoSum<FT>(a, -b, c);
      if (r != 0)
        inexact = true;
    }
  } else {
    FT r = TwoSum(a, b, c);
    if (r != 0) {
      inexact = true;
      RoundResult<FT, FT, rm>(r, c);
    }
  }

  return c;
}

template f16 FloppyFloat::Sub<f16, FloppyFloat::kRoundTiesToEven>(f16 a, f16 b);
template f16 FloppyFloat::Sub<f16, FloppyFloat::kRoundTowardPositive>(f16 a, f16 b);
template f16 FloppyFloat::Sub<f16, FloppyFloat::kRoundTowardNegative>(f16 a, f16 b);
template f16 FloppyFloat::Sub<f16, FloppyFloat::kRoundTowardZero>(f16 a, f16 b);

template f32 FloppyFloat::Sub<f32, FloppyFloat::kRoundTiesToEven>(f32 a, f32 b);
template f32 FloppyFloat::Sub<f32, FloppyFloat::kRoundTowardPositive>(f32 a, f32 b);
template f32 FloppyFloat::Sub<f32, FloppyFloat::kRoundTowardNegative>(f32 a, f32 b);
template f32 FloppyFloat::Sub<f32, FloppyFloat::kRoundTowardZero>(f32 a, f32 b);

template f64 FloppyFloat::Sub<f64, FloppyFloat::kRoundTiesToEven>(f64 a, f64 b);
template f64 FloppyFloat::Sub<f64, FloppyFloat::kRoundTowardPositive>(f64 a, f64 b);
template f64 FloppyFloat::Sub<f64, FloppyFloat::kRoundTowardNegative>(f64 a, f64 b);
template f64 FloppyFloat::Sub<f64, FloppyFloat::kRoundTowardZero>(f64 a, f64 b);

template <typename FT, FloppyFloat::RoundingMode rm>
FT FloppyFloat::Mul(FT a, FT b) {
  FT c = a * b;

  if (IsInfOrNan(c)) [[unlikely]] {
    if (IsInf(c)) {
      if (!IsInf(a) && !IsInf(b)) {
        overflow = true;
        inexact = true;
        c = RoundInf<FT, rm>(c);
      }
      return c;
    } else {  // NaN case.
      if (IsInf(a) && IsInf(b)) {
        invalid = true;
        return GetQnan<FT>();
      }
      if (IsSnan(a) || IsSnan(b))
        invalid = true;
      if (IsNan(a) || IsNan(b))
        return propagate_nan ? PropagateNan<FT>(a, b) : GetQnan<FT>();
    }
  }

  // TODO: Continue with underflow here!

  if constexpr (rm == kRoundTiesToEven) {
    if (!inexact) [[unlikely]] {
      auto r = UpMul(a, b, c);
      if (r != 0)
        inexact = true;
    }
  } else {
    auto r = UpMul(a, b, c);
    if (r != 0) {
      inexact = true;
      RoundResult<FT, typename TwiceWidthType<FT>::type, rm>(r, c);
      if ((std::abs(c) < nl<FT>::min())) [[unlikely]] {
        auto r = UpMul<FT>(a, b, c);
        if (r != 0)
          underflow = true;
      }
    }
  }

  return c;
}

template f16 FloppyFloat::Mul<f16, FloppyFloat::kRoundTiesToEven>(f16 a, f16 b);
template f16 FloppyFloat::Mul<f16, FloppyFloat::kRoundTowardPositive>(f16 a, f16 b);
template f16 FloppyFloat::Mul<f16, FloppyFloat::kRoundTowardNegative>(f16 a, f16 b);
template f16 FloppyFloat::Mul<f16, FloppyFloat::kRoundTowardZero>(f16 a, f16 b);

template f32 FloppyFloat::Mul<f32, FloppyFloat::kRoundTiesToEven>(f32 a, f32 b);
template f32 FloppyFloat::Mul<f32, FloppyFloat::kRoundTowardPositive>(f32 a, f32 b);
template f32 FloppyFloat::Mul<f32, FloppyFloat::kRoundTowardNegative>(f32 a, f32 b);
template f32 FloppyFloat::Mul<f32, FloppyFloat::kRoundTowardZero>(f32 a, f32 b);

template f64 FloppyFloat::Mul<f64, FloppyFloat::kRoundTiesToEven>(f64 a, f64 b);
template f64 FloppyFloat::Mul<f64, FloppyFloat::kRoundTowardPositive>(f64 a, f64 b);
template f64 FloppyFloat::Mul<f64, FloppyFloat::kRoundTowardNegative>(f64 a, f64 b);
template f64 FloppyFloat::Mul<f64, FloppyFloat::kRoundTowardZero>(f64 a, f64 b);
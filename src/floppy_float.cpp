/**************************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 **************************************************************************************************/

#include "floppy_float.h"

#include <bit>
#include <bitset>
#include <cmath>
#include <stdexcept>

using namespace FfUtils;

// 2Sum algorithm which determines the exact residual of an addition.
// May not work in cases that cause intermediate overflows (e.g., 65504.f16 + -48.f16).
// Prefer the Fast2Sum algorithm for these cases.
template <typename FT>
constexpr FT TwoSum(FT a, FT b, FT c) {
  FT ad = c - b;
  FT bd = c - ad;
  FT da = ad - a;
  FT db = bd - b;
  FT r = da + db;
  return r;
}

// 2Sum algorithm which determines the exact residual of an addition.
template <typename FT>
constexpr FT FastTwoSum(FT a, FT b, FT c) {
  const bool no_swap = std::fabs(a) > std::fabs(b);
  FT x = no_swap ? a : b;
  FT y = no_swap ? b : a;
  FT r = (c - x) - y;
  return r;
}

template <typename FT>
constexpr FT UpMulFma(FT a, FT b, FT c) {
  auto r = std::fma(-a, b, c);
  return r;
}

template <typename FT>
constexpr TwiceWidthType<FT>::type UpMul(FT a, FT b, FT c) {
  if constexpr (std::is_same_v<FT, f64>) {
    if (std::abs(c) > 4.008336720017946e-292) [[likely]]
      return UpMulFma<FT>(a, b, c);
  }

  auto da = static_cast<TwiceWidthType<FT>::type>(a);
  auto db = static_cast<TwiceWidthType<FT>::type>(b);
  auto dc = static_cast<TwiceWidthType<FT>::type>(c);
  auto r = dc - da * db;
  return r;
}

template <typename FT>
constexpr FT UpDivFma(FT a, FT b, FT c) {
  auto r = std::fma(c, b, -a);
  return std::signbit(b) ? -r : r;
}

template <typename FT>
constexpr TwiceWidthType<FT>::type UpDiv(FT a, FT b, FT c) {
  if constexpr (std::is_same_v<FT, f64>) {
    if (std::abs(a) > 4.008336720017946e-292) [[likely]]
      return UpDivFma<FT>(a, b, c);
  }

  auto da = static_cast<TwiceWidthType<FT>::type>(a);
  auto db = static_cast<TwiceWidthType<FT>::type>(b);
  auto dc = static_cast<TwiceWidthType<FT>::type>(c);
  auto r = dc * db - da;
  return std::signbit(b) ? -r : r;
}

template <typename FT>
constexpr FT UpSqrtFma(FT a, FT b) {
  auto r = std::fma(b, b, -a);
  return r;
}

template <typename FT>
constexpr TwiceWidthType<FT>::type UpSqrt(FT a, FT b) {
  if constexpr (std::is_same_v<FT, f64>) {
    if (std::abs(a) > 4.008336720017946e-292) [[likely]]
      return UpSqrtFma<FT>(a, b);
  }
  auto da = static_cast<TwiceWidthType<FT>::type>(a);
  auto db = static_cast<TwiceWidthType<FT>::type>(b);
  auto r = db * db - da;
  return r;
}

template <typename FT>
constexpr TwiceWidthType<FT>::type UpFma(FT a, FT b, FT c, FT d) {
  auto da = static_cast<TwiceWidthType<FT>::type>(a);
  auto db = static_cast<TwiceWidthType<FT>::type>(b);
  auto dc = static_cast<TwiceWidthType<FT>::type>(c);
  auto dd = static_cast<TwiceWidthType<FT>::type>(d);
  auto p = da * db;
  auto di = p + dc;
  auto r1 = TwoSum<typename TwiceWidthType<FT>::type>(p, dc, di);
  auto r2 = dd - di;
  return r1 + r2;
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

FloppyFloat::FloppyFloat() : SoftFloat() {
  SetQnan<f16>(0x7e00u);
  SetQnan<f32>(0x7fc00000u);
  SetQnan<f64>(0x7ff8000000000000ull);
  ClearFlags();
  tininess_before_rounding = false;
}

template <typename FT, FloppyFloat::RoundingMode rm>
constexpr FT RoundInf(FT result) {
  if constexpr (rm == FloppyFloat::kRoundTiesToEven) {
    return result;
  } else if constexpr (rm == FloppyFloat::kRoundTowardPositive) {
    return IsNegInf(result) ? nl<FT>::lowest() : result;
  } else if constexpr (rm == FloppyFloat::kRoundTowardNegative) {
    return IsPosInf(result) ? nl<FT>::max() : result;
  } else if constexpr (rm == FloppyFloat::kRoundTowardZero) {
    return IsNegInf(result) ? nl<FT>::lowest() : nl<FT>::max();
  } else if constexpr (rm == FloppyFloat::kRoundTiesToAway) {
    return result;
  } else {
    static_assert("Using unsupported rounding mode");
  }
}

template <typename FT, typename TFT, FloppyFloat::RoundingMode rm>
constexpr FT FloppyFloat::RoundResult([[maybe_unused]] TFT residual, FT result) {
  if constexpr (rm == kRoundTiesToEven) {
    // Nothing to do.
  } else if constexpr (rm == kRoundTowardPositive) {
    if (residual < static_cast<FT>(0.f)) {
      result = NextUpNoNegZero(result);
      overflow = IsPosInf(result) ? true : overflow;
    }
  } else if constexpr (rm == kRoundTowardNegative) {
    if (residual > static_cast<FT>(0.f)) {
      result = NextDownNoPosZero(result);
      overflow = IsNegInf(result) ? true : overflow;
    }
  } else if constexpr (rm == kRoundTowardZero) {
    if (residual < static_cast<FT>(0.f) && result < static_cast<FT>(0.f)) {  // Fix a round-down.
      result = NextUpNoNegZero(result);
      overflow = IsPosInf(result) ? true : overflow;
    } else if (residual > static_cast<FT>(0.f) && result > static_cast<FT>(0.f)) {  // Fix a round-up.
      result = NextDownNoPosZero(result);
      overflow = IsNegInf(result) ? true : overflow;
    }
  } else {
    static_assert("Using unsupported rounding mode");
  }
  return result;
}

template <typename FT>
constexpr FT SetQuietBit(FT a) {
  auto au = std::bit_cast<typename FloatToUint<FT>::type>(a);
  return std::bit_cast<FT>((decltype(au))(QuietBit<FT>::u | au));
}

template <typename FT>
constexpr FT FloppyFloat::PropagateNan(FT a, FT b) {
  if (nan_propagation_scheme == kNanPropX86sse) {
    if (IsSnan(a) || IsSnan(b)) {
      if (IsSnan(a))
        return SetQuietBit(a);
    }
    return IsNan(a) ? SetQuietBit(a) : SetQuietBit(b);
  } else if (nan_propagation_scheme == kNanPropRiscv) {
    return GetQnan<FT>();
  } else if (nan_propagation_scheme == kNanPropArm64DefaultNan) {
    return GetQnan<FT>();
  } else {
    throw std::runtime_error(std::string("Unknown NaN propagation scheme"));
  }
}

constexpr f64 FloppyFloat::PropagateNan(f32 a) {
  if (nan_propagation_scheme == kNanPropX86sse) {
    u64 payload = (u64)GetPayload(a) << 29;
    u64 result = (((u64)std::signbit(a)) << 63) | 0x7ff8000000000000ull | payload;
    return std::bit_cast<f64>(result);
  } else if (nan_propagation_scheme == kNanPropRiscv) {
    return GetQnan<f64>();
  } else if (nan_propagation_scheme == kNanPropArm64DefaultNan) {
    return GetQnan<f64>();
  } else {
    throw std::runtime_error(std::string("Unknown NaN propagation scheme"));
  }
}

template <typename FT>
FT GetRScaled(FT r) {
  FT r_scaled;
  if constexpr (std::is_same_v<FT, f16>) {
    r_scaled = r * 2048.0f16;  // = 2**11
  } else if constexpr (std::is_same_v<FT, f32>) {
    r_scaled = r * 16777216.0f32;  // 2**24
  } else if constexpr (std::is_same_v<FT, f64>) {
    r_scaled = r * 9007199254740992.0f64;  // 2**53
  } else {
    static_assert("Unsupported data type");
  }
  return r_scaled;
}

template <typename FT>
constexpr FT ResidualLimit() {
  if constexpr (std::is_same_v<FT, f16>) {
    return 32.f16;  // 2**5
  } else if constexpr (std::is_same_v<FT, f32>) {
    return 20282409603651670423947251286016.f32;  // 2**104
  } else if constexpr (std::is_same_v<FT, f64>) {
    return std::bit_cast<f64>(0x7de0000100000000ull);  // 2**991
  }
}

// TODO mathematical proof or change to *0.5 method.
template <typename FT, FloppyFloat::RoundingMode rm>
constexpr bool IsOverflow(FT a, FT b, FT c) {
  if (IsInf(c))
    return true;
  if constexpr (rm == FloppyFloat::kRoundTiesToEven) {
    return true;
  } else if constexpr (rm == FloppyFloat::kRoundTowardPositive) {
    FT r = FastTwoSum<FT>(a, b, c);
    return (r >= ResidualLimit<FT>());
  } else if constexpr (rm == FloppyFloat::kRoundTowardNegative) {
    FT r = FastTwoSum<FT>(a, b, c);
    return (r <= -ResidualLimit<FT>());
  } else if constexpr (rm == FloppyFloat::kRoundTowardZero) {
    FT r = FastTwoSum<FT>(a, b, c);
    if (std::signbit(c)) {
      return (r >= ResidualLimit<FT>());
    } else {
      return (r <= -ResidualLimit<FT>());
    }
  } else if constexpr (rm == FloppyFloat::kRoundTiesToAway) {
    return true;
  } else {
    static_assert("Using unsupported rounding mode");
  }
}

template <typename FT>
FT FloppyFloat::Add(FT a, FT b) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return Add<FT, kRoundTiesToEven>(a, b);
  case kRoundTiesToAway:
    return Add<FT, kRoundTiesToAway>(a, b);
  case kRoundTowardPositive:
    return Add<FT, kRoundTowardPositive>(a, b);
  case kRoundTowardNegative:
    return Add<FT, kRoundTowardNegative>(a, b);
  case kRoundTowardZero:
    return Add<FT, kRoundTowardZero>(a, b);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template f16 FloppyFloat::Add<f16>(f16 a, f16 b);
template f32 FloppyFloat::Add<f32>(f32 a, f32 b);
template f64 FloppyFloat::Add<f64>(f64 a, f64 b);

template <typename FT, FloppyFloat::RoundingMode rm>
constexpr FT FloppyFloat::Add(FT a, FT b) {
  FT c = a + b;

  if (IsInfOrNan(c)) [[unlikely]] {
    if (IsInf(c)) {
      if (!IsInf(a) && !IsInf(b)) {
        c = RoundInf<FT, rm>(c);
        overflow = IsOverflow<FT, rm>(a, b, c);
        inexact = true;
      }
      return c;
    }
    if (IsInf(a) && IsInf(b)) {
      invalid = true;
      return GetQnan<FT>();
    }
    if (IsSnan(a) || IsSnan(b))
      invalid = true;
    if (IsNan(a) || IsNan(b))
      return PropagateNan<FT>(a, b);
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
      FT r = FastTwoSum<FT>(a, b, c);
      if (!IsZero(r))
        inexact = true;
    }
  } else {
    FT r = FastTwoSum<FT>(a, b, c);
    if (!IsZero(r)) {
      inexact = true;
      if constexpr (rm == kRoundTiesToAway) {
        FT cc = ClearSignificand<FT>(c);
        FT r_scaled = GetRScaled<FT>(r);

        if (-cc == r_scaled) [[unlikely]] {
          if (r < 0. && c > 0.) {
            c = NextUpNoNegZero(c);
            overflow = IsInf(c) ? true : overflow;
          } else if (r > 0. && c < 0.) {
            c = NextDownNoPosZero(c);
            overflow = IsInf(c) ? true : overflow;
          }
        }
      } else {
        c = RoundResult<FT, FT, rm>(r, c);
      }
    }
  }

  return c;
}

template f16 FloppyFloat::Add<f16, FloppyFloat::kRoundTiesToEven>(f16 a, f16 b);
template f16 FloppyFloat::Add<f16, FloppyFloat::kRoundTowardPositive>(f16 a, f16 b);
template f16 FloppyFloat::Add<f16, FloppyFloat::kRoundTowardNegative>(f16 a, f16 b);
template f16 FloppyFloat::Add<f16, FloppyFloat::kRoundTowardZero>(f16 a, f16 b);
template f16 FloppyFloat::Add<f16, FloppyFloat::kRoundTiesToAway>(f16 a, f16 b);

template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTiesToEven>(f32 a, f32 b);
template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTowardPositive>(f32 a, f32 b);
template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTowardNegative>(f32 a, f32 b);
template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTowardZero>(f32 a, f32 b);
template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTiesToAway>(f32 a, f32 b);

template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTiesToEven>(f64 a, f64 b);
template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTowardPositive>(f64 a, f64 b);
template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTowardNegative>(f64 a, f64 b);
template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTowardZero>(f64 a, f64 b);
template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTiesToAway>(f64 a, f64 b);

template <typename FT>
FT FloppyFloat::Sub(FT a, FT b) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return Sub<FT, kRoundTiesToEven>(a, b);
  case kRoundTiesToAway:
    return Sub<FT, kRoundTiesToAway>(a, b);
  case kRoundTowardPositive:
    return Sub<FT, kRoundTowardPositive>(a, b);
  case kRoundTowardNegative:
    return Sub<FT, kRoundTowardNegative>(a, b);
  case kRoundTowardZero:
    return Sub<FT, kRoundTowardZero>(a, b);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template f16 FloppyFloat::Sub<f16>(f16 a, f16 b);
template f32 FloppyFloat::Sub<f32>(f32 a, f32 b);
template f64 FloppyFloat::Sub<f64>(f64 a, f64 b);

template <typename FT, FloppyFloat::RoundingMode rm>
constexpr FT FloppyFloat::Sub(FT a, FT b) {
  FT c = a - b;

  if (IsInfOrNan(c)) [[unlikely]] {
    if (IsInf(c)) {
      if (!IsInf(a) && !IsInf(b)) {
        c = RoundInf<FT, rm>(c);
        overflow = IsOverflow<FT, rm>(a, -b, c);
        inexact = true;
      }
      return c;
    }
    if (IsInf(a) && IsInf(b)) {
      invalid = true;
      return GetQnan<FT>();
    }
    if (IsSnan(a) || IsSnan(b))
      invalid = true;
    if (IsNan(a) || IsNan(b))
      return PropagateNan<FT>(a, b);
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
      FT r = FastTwoSum<FT>(a, -b, c);
      if (!IsZero(r))
        inexact = true;
    }
  } else {
    FT r = FastTwoSum(a, -b, c);
    if (!IsZero(r)) {
      inexact = true;
      if constexpr (rm == kRoundTiesToAway) {
        FT cc = ClearSignificand<FT>(c);
        FT r_scaled = GetRScaled<FT>(r);

        if (-cc == r_scaled) [[unlikely]] {
          if (r < 0. && c > 0.) {
            c = NextUpNoNegZero(c);
            overflow = IsInf(c) ? true : overflow;
          } else if (r > 0. && c < 0.) {
            c = NextDownNoPosZero(c);
            overflow = IsInf(c) ? true : overflow;
          }
        }
      } else {
        c = RoundResult<FT, FT, rm>(r, c);
      }
    }
  }

  return c;
}

template f16 FloppyFloat::Sub<f16, FloppyFloat::kRoundTiesToEven>(f16 a, f16 b);
template f16 FloppyFloat::Sub<f16, FloppyFloat::kRoundTowardPositive>(f16 a, f16 b);
template f16 FloppyFloat::Sub<f16, FloppyFloat::kRoundTowardNegative>(f16 a, f16 b);
template f16 FloppyFloat::Sub<f16, FloppyFloat::kRoundTowardZero>(f16 a, f16 b);
template f16 FloppyFloat::Sub<f16, FloppyFloat::kRoundTiesToAway>(f16 a, f16 b);

template f32 FloppyFloat::Sub<f32, FloppyFloat::kRoundTiesToEven>(f32 a, f32 b);
template f32 FloppyFloat::Sub<f32, FloppyFloat::kRoundTowardPositive>(f32 a, f32 b);
template f32 FloppyFloat::Sub<f32, FloppyFloat::kRoundTowardNegative>(f32 a, f32 b);
template f32 FloppyFloat::Sub<f32, FloppyFloat::kRoundTowardZero>(f32 a, f32 b);
template f32 FloppyFloat::Sub<f32, FloppyFloat::kRoundTiesToAway>(f32 a, f32 b);

template f64 FloppyFloat::Sub<f64, FloppyFloat::kRoundTiesToEven>(f64 a, f64 b);
template f64 FloppyFloat::Sub<f64, FloppyFloat::kRoundTowardPositive>(f64 a, f64 b);
template f64 FloppyFloat::Sub<f64, FloppyFloat::kRoundTowardNegative>(f64 a, f64 b);
template f64 FloppyFloat::Sub<f64, FloppyFloat::kRoundTowardZero>(f64 a, f64 b);
template f64 FloppyFloat::Sub<f64, FloppyFloat::kRoundTiesToAway>(f64 a, f64 b);

template <typename FT>
FT FloppyFloat::Mul(FT a, FT b) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return Mul<FT, kRoundTiesToEven>(a, b);
  case kRoundTiesToAway:
    return Mul<FT, kRoundTiesToAway>(a, b);
  case kRoundTowardPositive:
    return Mul<FT, kRoundTowardPositive>(a, b);
  case kRoundTowardNegative:
    return Mul<FT, kRoundTowardNegative>(a, b);
  case kRoundTowardZero:
    return Mul<FT, kRoundTowardZero>(a, b);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template f16 FloppyFloat::Mul<f16>(f16 a, f16 b);
template f32 FloppyFloat::Mul<f32>(f32 a, f32 b);
template f64 FloppyFloat::Mul<f64>(f64 a, f64 b);

template <typename FT, FloppyFloat::RoundingMode rm>
constexpr FT FloppyFloat::Mul(FT a, FT b) {
  if constexpr (rm == kRoundTiesToAway) {
    RoundingMode old_rm = rounding_mode;
    rounding_mode = rm;
    FT c = SoftFloat::Mul(a, b);
    rounding_mode = old_rm;
    return c;
  }

  FT c = a * b;

  if (IsInfOrNan(c)) [[unlikely]] {
    if (IsInf(c)) {
      if (!IsInf(a) && !IsInf(b)) {
        overflow = true;
        inexact = true;
        c = RoundInf<FT, rm>(c);
      }
      return c;
    }
    if (IsSnan(a) || IsSnan(b))
      invalid = true;
    if (IsNan(a) || IsNan(b))
      return PropagateNan<FT>(a, b);
    invalid = true;
    return GetQnan<FT>();
  }

  if constexpr (rm == kRoundTiesToEven) {
    if (!inexact) [[unlikely]] {
      auto r = UpMul<FT>(a, b, c);
      if (!IsZero(r))
        inexact = true;
    }
    if (!underflow) {
      if (IsTiny(c)) [[unlikely]] {
        auto r = UpMul<FT>(a, b, c);
        if (r != 0.)
          underflow = true;
      }
    }
  } else {
    auto r = UpMul(a, b, c);
    if (!IsZero(r)) {
      inexact = true;
      c = RoundResult<FT, typename TwiceWidthType<FT>::type, rm>(r, c);
      if (IsTiny(c)) [[unlikely]] {
        if (!IsZero(r))
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
template f16 FloppyFloat::Mul<f16, FloppyFloat::kRoundTiesToAway>(f16 a, f16 b);

template f32 FloppyFloat::Mul<f32, FloppyFloat::kRoundTiesToEven>(f32 a, f32 b);
template f32 FloppyFloat::Mul<f32, FloppyFloat::kRoundTowardPositive>(f32 a, f32 b);
template f32 FloppyFloat::Mul<f32, FloppyFloat::kRoundTowardNegative>(f32 a, f32 b);
template f32 FloppyFloat::Mul<f32, FloppyFloat::kRoundTowardZero>(f32 a, f32 b);
template f32 FloppyFloat::Mul<f32, FloppyFloat::kRoundTiesToAway>(f32 a, f32 b);

template f64 FloppyFloat::Mul<f64, FloppyFloat::kRoundTiesToEven>(f64 a, f64 b);
template f64 FloppyFloat::Mul<f64, FloppyFloat::kRoundTowardPositive>(f64 a, f64 b);
template f64 FloppyFloat::Mul<f64, FloppyFloat::kRoundTowardNegative>(f64 a, f64 b);
template f64 FloppyFloat::Mul<f64, FloppyFloat::kRoundTowardZero>(f64 a, f64 b);
template f64 FloppyFloat::Mul<f64, FloppyFloat::kRoundTiesToAway>(f64 a, f64 b);

template <typename FT>
FT FloppyFloat::Div(FT a, FT b) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return Div<FT, kRoundTiesToEven>(a, b);
  case kRoundTiesToAway:
    return Div<FT, kRoundTiesToAway>(a, b);
  case kRoundTowardPositive:
    return Div<FT, kRoundTowardPositive>(a, b);
  case kRoundTowardNegative:
    return Div<FT, kRoundTowardNegative>(a, b);
  case kRoundTowardZero:
    return Div<FT, kRoundTowardZero>(a, b);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template f16 FloppyFloat::Div<f16>(f16 a, f16 b);
template f32 FloppyFloat::Div<f32>(f32 a, f32 b);
template f64 FloppyFloat::Div<f64>(f64 a, f64 b);

template <typename FT, FloppyFloat::RoundingMode rm>
constexpr FT FloppyFloat::Div(FT a, FT b) {
  if constexpr (rm == kRoundTiesToAway) {
    RoundingMode old_rm = rounding_mode;
    rounding_mode = rm;
    FT d = SoftFloat::Div(a, b);
    rounding_mode = old_rm;
    return d;
  }

  FT c = a / b;

  if (IsInfOrNan(c)) [[unlikely]] {
    if (IsInf(c)) {
      if (!IsInf(a) && IsZero(b)) {
        division_by_zero = true;
        return c;
      }
      if (!IsInf(a) && !(IsInf(b))) {
        overflow = true;
        inexact = true;
        c = RoundInf<FT, rm>(c);
      }
      return c;
    }
    if (IsSnan(a) || IsSnan(b))
      invalid = true;
    if (IsNan(a) || IsNan(b))
      return PropagateNan<FT>(a, b);
    invalid = true;
    return GetQnan<FT>();
  }

  if (IsInf(b)) [[unlikely]]
    return c;

  if constexpr (rm == kRoundTiesToEven) {
    if (!inexact) [[unlikely]] {
      auto r = UpDiv<FT>(a, b, c);
      if (!IsZero(r))
        inexact = true;
    }
    if (!underflow) {
      if (IsTiny(c)) [[unlikely]] {
        auto r = UpDiv<FT>(a, b, c);
        if (!IsZero(r))
          underflow = true;
      }
    }
  } else {
    auto r = UpDiv<FT>(a, b, c);
    if (!IsZero(r)) {
      inexact = true;
      c = RoundResult<FT, typename TwiceWidthType<FT>::type, rm>(r, c);
      if (IsTiny(c)) [[unlikely]] {
        if (!IsZero(r))
          underflow = true;
      }
    }
  }

  return c;
}

template f16 FloppyFloat::Div<f16, FloppyFloat::kRoundTiesToEven>(f16 a, f16 b);
template f16 FloppyFloat::Div<f16, FloppyFloat::kRoundTowardPositive>(f16 a, f16 b);
template f16 FloppyFloat::Div<f16, FloppyFloat::kRoundTowardNegative>(f16 a, f16 b);
template f16 FloppyFloat::Div<f16, FloppyFloat::kRoundTowardZero>(f16 a, f16 b);

template f32 FloppyFloat::Div<f32, FloppyFloat::kRoundTiesToEven>(f32 a, f32 b);
template f32 FloppyFloat::Div<f32, FloppyFloat::kRoundTowardPositive>(f32 a, f32 b);
template f32 FloppyFloat::Div<f32, FloppyFloat::kRoundTowardNegative>(f32 a, f32 b);
template f32 FloppyFloat::Div<f32, FloppyFloat::kRoundTowardZero>(f32 a, f32 b);

template f64 FloppyFloat::Div<f64, FloppyFloat::kRoundTiesToEven>(f64 a, f64 b);
template f64 FloppyFloat::Div<f64, FloppyFloat::kRoundTowardPositive>(f64 a, f64 b);
template f64 FloppyFloat::Div<f64, FloppyFloat::kRoundTowardNegative>(f64 a, f64 b);
template f64 FloppyFloat::Div<f64, FloppyFloat::kRoundTowardZero>(f64 a, f64 b);

template <typename FT>
FT FloppyFloat::Sqrt(FT a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return Sqrt<FT, kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return Sqrt<FT, kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return Sqrt<FT, kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return Sqrt<FT, kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return Sqrt<FT, kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template f16 FloppyFloat::Sqrt<f16>(f16 a);
template f32 FloppyFloat::Sqrt<f32>(f32 a);
template f64 FloppyFloat::Sqrt<f64>(f64 a);

template <typename FT, FloppyFloat::RoundingMode rm>
constexpr FT FloppyFloat::Sqrt(FT a) {
  if constexpr (rm == kRoundTiesToAway) {
    RoundingMode old_rm = rounding_mode;
    rounding_mode = rm;
    FT b = SoftFloat::Sqrt(a);
    rounding_mode = old_rm;
    return b;
  }

  FT b = std::sqrt(a);

  if (IsNan(b)) [[unlikely]] {
    if (IsSnan(a))
      invalid = true;
    if (IsNan(a))
      return PropagateNan<FT>(a, a);
    invalid = true;
    return GetQnan<FT>();
  }

  if constexpr (rm == kRoundTiesToEven) {
    if (!inexact) [[unlikely]] {
      if (IsInf(a)) [[unlikely]]
        return b;
      auto r = UpSqrt<FT>(a, b);
      if (!IsZero(r))
        inexact = true;
    }
  } else {
    if (IsInf(a)) [[unlikely]]
      return b;
    auto r = UpSqrt<FT>(a, b);
    if (!IsZero(r)) {
      inexact = true;
      b = RoundResult<FT, typename TwiceWidthType<FT>::type, rm>(r, b);
    }
  }

  return b;
}

template f16 FloppyFloat::Sqrt<f16, FloppyFloat::kRoundTiesToEven>(f16 a);
template f16 FloppyFloat::Sqrt<f16, FloppyFloat::kRoundTowardPositive>(f16 a);
template f16 FloppyFloat::Sqrt<f16, FloppyFloat::kRoundTowardNegative>(f16 a);
template f16 FloppyFloat::Sqrt<f16, FloppyFloat::kRoundTowardZero>(f16 a);

template f32 FloppyFloat::Sqrt<f32, FloppyFloat::kRoundTiesToEven>(f32 a);
template f32 FloppyFloat::Sqrt<f32, FloppyFloat::kRoundTowardPositive>(f32 a);
template f32 FloppyFloat::Sqrt<f32, FloppyFloat::kRoundTowardNegative>(f32 a);
template f32 FloppyFloat::Sqrt<f32, FloppyFloat::kRoundTowardZero>(f32 a);

template f64 FloppyFloat::Sqrt<f64, FloppyFloat::kRoundTiesToEven>(f64 a);
template f64 FloppyFloat::Sqrt<f64, FloppyFloat::kRoundTowardPositive>(f64 a);
template f64 FloppyFloat::Sqrt<f64, FloppyFloat::kRoundTowardNegative>(f64 a);
template f64 FloppyFloat::Sqrt<f64, FloppyFloat::kRoundTowardZero>(f64 a);

template <typename FT>
FT FloppyFloat::Fma(FT a, FT b, FT c) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return Fma<FT, kRoundTiesToEven>(a, b, c);
  case kRoundTiesToAway:
    return Fma<FT, kRoundTiesToAway>(a, b, c);
  case kRoundTowardPositive:
    return Fma<FT, kRoundTowardPositive>(a, b, c);
  case kRoundTowardNegative:
    return Fma<FT, kRoundTowardNegative>(a, b, c);
  case kRoundTowardZero:
    return Fma<FT, kRoundTowardZero>(a, b, c);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template f16 FloppyFloat::Fma<f16>(f16 a, f16 b, f16 c);
template f32 FloppyFloat::Fma<f32>(f32 a, f32 b, f32 c);
template f64 FloppyFloat::Fma<f64>(f64 a, f64 b, f64 c);

template <typename FT, FloppyFloat::RoundingMode rm>
constexpr FT FloppyFloat::Fma(FT a, FT b, FT c) {
  if constexpr (std::is_same_v<FT, f16> || (rm == kRoundTiesToAway)) {
    // TODO: Remove once the f16 FMA issue of the standard library is solved.
    RoundingMode old_rm = rounding_mode;
    rounding_mode = rm;
    FT d = SoftFloat::Fma(a, b, c);
    rounding_mode = old_rm;
    return d;
  }

  FT d = std::fma(a, b, c);

  if (IsInfOrNan(d)) [[unlikely]] {
    if (IsInf(d)) {
      if (!IsInf(a) && !IsInf(b) && !IsInf(c)) {
        overflow = true;
        inexact = true;
        d = RoundInf<FT, rm>(d);
      }
      return d;
    }
    if ((IsZero(a) && IsInf(b)) || (IsZero(b) && IsInf(a)))
      invalid = invalid_fma ? true : invalid;
    if (IsSnan(a) || IsSnan(b) || IsSnan(c))
      invalid = true;
    if (IsNan(a) || IsNan(b) || IsNan(c))
      return PropagateNan<FT>(PropagateNan<FT>(a, b), c);
    invalid = true;
    return GetQnan<FT>();
  }

  if constexpr (rm == kRoundTowardNegative) {
    if (IsZero(d) && !std::signbit(d)) [[unlikely]] {
      if ((std::signbit(a) != std::signbit(b)) || std::signbit(c))
        d = -d;
    }
  }

  if constexpr (rm == kRoundTiesToEven) {
    if (!inexact) [[unlikely]] {
      auto r = UpFma<FT>(a, b, c, d);
      if (!IsZero(r))
        inexact = true;
    }
    if (!underflow) {
      if (IsTiny(d)) [[unlikely]] {
        auto r = UpFma<FT>(a, b, c, d);
        if (!IsZero(r))
          underflow = true;
      }
    }
  } else {
    auto r = UpFma<FT>(a, b, c, d);
    if (!IsZero(r)) {
      inexact = true;
      d = RoundResult<FT, typename TwiceWidthType<FT>::type, rm>(r, d);
      if (IsTiny(d)) [[unlikely]] {
        if (!IsZero(r))
          underflow = true;
      }
    }
  }

  return d;
}

template f16 FloppyFloat::Fma<f16, FloppyFloat::kRoundTiesToEven>(f16 a, f16 b, f16 c);
template f16 FloppyFloat::Fma<f16, FloppyFloat::kRoundTowardPositive>(f16 a, f16 b, f16 c);
template f16 FloppyFloat::Fma<f16, FloppyFloat::kRoundTowardNegative>(f16 a, f16 b, f16 c);
template f16 FloppyFloat::Fma<f16, FloppyFloat::kRoundTowardZero>(f16 a, f16 b, f16 c);

template f32 FloppyFloat::Fma<f32, FloppyFloat::kRoundTiesToEven>(f32 a, f32 b, f32 c);
template f32 FloppyFloat::Fma<f32, FloppyFloat::kRoundTowardPositive>(f32 a, f32 b, f32 c);
template f32 FloppyFloat::Fma<f32, FloppyFloat::kRoundTowardNegative>(f32 a, f32 b, f32 c);
template f32 FloppyFloat::Fma<f32, FloppyFloat::kRoundTowardZero>(f32 a, f32 b, f32 c);

template f64 FloppyFloat::Fma<f64, FloppyFloat::kRoundTiesToEven>(f64 a, f64 b, f64 c);
template f64 FloppyFloat::Fma<f64, FloppyFloat::kRoundTowardPositive>(f64 a, f64 b, f64 c);
template f64 FloppyFloat::Fma<f64, FloppyFloat::kRoundTowardNegative>(f64 a, f64 b, f64 c);
template f64 FloppyFloat::Fma<f64, FloppyFloat::kRoundTowardZero>(f64 a, f64 b, f64 c);

template <typename FT, bool quiet>
bool FloppyFloat::Eq(FT a, FT b) {
  if (IsNan(a) || IsNan(b)) [[unlikely]] {
    if (!quiet || IsSnan(a) || IsSnan(b))
      invalid = true;
    return false;
  }

  return a == b;
}

template bool FloppyFloat::Eq<f16, false>(f16 a, f16 b);
template bool FloppyFloat::Eq<f32, false>(f32 a, f32 b);
template bool FloppyFloat::Eq<f64, false>(f64 a, f64 b);
template bool FloppyFloat::Eq<f16, true>(f16 a, f16 b);
template bool FloppyFloat::Eq<f32, true>(f32 a, f32 b);
template bool FloppyFloat::Eq<f64, true>(f64 a, f64 b);

template <typename FT, bool quiet>
bool FloppyFloat::Le(FT a, FT b) {
  if (IsNan(a) || IsNan(b)) [[unlikely]] {
    if (!quiet || IsSnan(a) || IsSnan(b))
      invalid = true;
    return false;
  }

  return a <= b;
}

template bool FloppyFloat::Le<f16, false>(f16 a, f16 b);
template bool FloppyFloat::Le<f32, false>(f32 a, f32 b);
template bool FloppyFloat::Le<f64, false>(f64 a, f64 b);
template bool FloppyFloat::Le<f16, true>(f16 a, f16 b);
template bool FloppyFloat::Le<f32, true>(f32 a, f32 b);
template bool FloppyFloat::Le<f64, true>(f64 a, f64 b);

template <typename FT, bool quiet>
bool FloppyFloat::Lt(FT a, FT b) {
  if (IsNan(a) || IsNan(b)) [[unlikely]] {
    if (!quiet || IsSnan(a) || IsSnan(b))
      invalid = true;
    return false;
  }

  return a < b;
}

template bool FloppyFloat::Lt<f16, false>(f16 a, f16 b);
template bool FloppyFloat::Lt<f32, false>(f32 a, f32 b);
template bool FloppyFloat::Lt<f64, false>(f64 a, f64 b);
template bool FloppyFloat::Lt<f16, true>(f16 a, f16 b);
template bool FloppyFloat::Lt<f32, true>(f32 a, f32 b);
template bool FloppyFloat::Lt<f64, true>(f64 a, f64 b);

template <typename FT>
FT FloppyFloat::Maxx86(FT a, FT b) {
  if (IsNan(a) || IsNan(b)) [[unlikely]] {
    invalid = true;
    return b;
  }

  if (a == b)  // +0 and -0 case.
    return b;

  return (a > b) ? a : b;
}

template f16 FloppyFloat::Maxx86<f16>(f16 a, f16 b);
template f32 FloppyFloat::Maxx86<f32>(f32 a, f32 b);
template f64 FloppyFloat::Maxx86<f64>(f64 a, f64 b);

template <typename FT>
FT FloppyFloat::Minx86(FT a, FT b) {
  if (IsNan(a) || IsNan(b)) [[unlikely]] {
    invalid = true;
    return b;
  }

  if (a == b)  // +0 and -0 case.
    return b;

  return (a < b) ? a : b;
}

template f16 FloppyFloat::Minx86<f16>(f16 a, f16 b);
template f32 FloppyFloat::Minx86<f32>(f32 a, f32 b);
template f64 FloppyFloat::Minx86<f64>(f64 a, f64 b);

template <typename FT>
FT FloppyFloat::MaximumNumber(FT a, FT b) {
  if (IsNan(a) || IsNan(b)) [[unlikely]] {
    if (IsSnan(a) || IsSnan(b))
      invalid = true;
    if (IsNan(a) || IsNan(b))
      return GetQnan<FT>();
    return IsNan(a) ? b : a;
  }

  if (IsZero(a) && IsZero(b))
    return std::signbit(a) && std::signbit(b) ? (FT)-0.0f : (FT) + 0.0f;

  return (a > b) ? a : b;
}

template f16 FloppyFloat::MaximumNumber<f16>(f16 a, f16 b);
template f32 FloppyFloat::MaximumNumber<f32>(f32 a, f32 b);
template f64 FloppyFloat::MaximumNumber<f64>(f64 a, f64 b);

template <typename FT>
FT FloppyFloat::MinimumNumber(FT a, FT b) {
  if (IsNan(a) || IsNan(b)) [[unlikely]] {
    if (IsSnan(a) || IsSnan(b))
      invalid = true;
    if (IsNan(a) || IsNan(b))
      return GetQnan<FT>();
    return IsNan(a) ? b : a;
  }

  if (IsZero(a) && IsZero(b))
    return std::signbit(a) && std::signbit(b) ? (FT)-0.0f : (FT) + 0.0f;

  return (a > b) ? a : b;
}

template f16 FloppyFloat::MinimumNumber<f16>(f16 a, f16 b);
template f32 FloppyFloat::MinimumNumber<f32>(f32 a, f32 b);
template f64 FloppyFloat::MinimumNumber<f64>(f64 a, f64 b);

// Assumes that "result" was calculated with "kRoundTowardZero"
template <typename FT, typename IT, FloppyFloat::RoundingMode rm>
constexpr IT RoundIntegerResult(FT residual, FT source, IT result) {
  if constexpr (rm == FloppyFloat::kRoundTowardNegative) {
    if (residual > 0)
      result -= 1;
  } else if constexpr (rm == FloppyFloat::kRoundTowardPositive) {
    if (residual < 0)
      result += 1;
  } else {
    if (residual) {
      FT ia_p05 = static_cast<FT>(result) + static_cast<FT>(0.5);
      if constexpr (rm == FloppyFloat::kRoundTiesToEven) {
        if (ia_p05 <= source) {
          if (ia_p05 == source) [[unlikely]] {
            if (!(result % 2))
              result -= 1;
          }
          result += 1;
        }
      } else if constexpr (rm == FloppyFloat::kRoundTiesToAway) {
        if (ia_p05 <= source)
          result += 1;
      }
    }
  }
  return result;
}

i32 FloppyFloat::F32ToI32(f32 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return F32ToI32<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return F32ToI32<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return F32ToI32<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return F32ToI32<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return F32ToI32<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr inline i32 FloppyFloat::F32ToI32(f32 a) {
  if (IsNan(a)) [[unlikely]] {
    invalid = true;
    return nan_limit_i32_;
  } else if (a >= 2147483648.f32) [[unlikely]] {
    invalid = true;
    return max_limit_i32_;
  } else if (a < -2147483648.f32) [[unlikely]] {
    invalid = true;
    return min_limit_i32_;
  }

  i32 ia;
  if constexpr (rm == kRoundTiesToEven) {
    ia = std::lrint(a);
  } else if constexpr (rm == kRoundTiesToAway) {
    ia = std::lround(a);
  } else {
    ia = static_cast<i32>(a);  // C++ always truncates (i.e., rounds to zero).
  }

  f32 r = static_cast<f32>(ia) - a;
  if (!IsZero(r))
    inexact = true;

  if constexpr (rm == kRoundTowardNegative) {
    if (r > 0.f32)
      ia -= 1;
  } else if constexpr (rm == kRoundTowardPositive) {
    if (r < 0.f32)
      ia += 1;
  }

  return ia;
}

template i32 FloppyFloat::F32ToI32<FloppyFloat::kRoundTiesToEven>(f32 a);
template i32 FloppyFloat::F32ToI32<FloppyFloat::kRoundTiesToAway>(f32 a);
template i32 FloppyFloat::F32ToI32<FloppyFloat::kRoundTowardNegative>(f32 a);
template i32 FloppyFloat::F32ToI32<FloppyFloat::kRoundTowardPositive>(f32 a);
template i32 FloppyFloat::F32ToI32<FloppyFloat::kRoundTowardZero>(f32 a);

i64 FloppyFloat::F32ToI64(f32 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return F32ToI64<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return F32ToI64<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return F32ToI64<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return F32ToI64<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return F32ToI64<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr i64 FloppyFloat::F32ToI64(f32 a) {
  if (IsNan(a)) [[unlikely]] {
    invalid = true;
    return nan_limit_i64_;
  } else if (a >= 9223372036854775808.0f32) [[unlikely]] {
    invalid = true;
    return max_limit_i64_;
  } else if (a < -9223372036854775808.0f32) [[unlikely]] {
    invalid = true;
    return min_limit_i64_;
  }

  i64 ia;
  if constexpr (rm == kRoundTiesToEven) {
    ia = std::lrint(a);
  } else if constexpr (rm == kRoundTiesToAway) {
    ia = std::lround(a);
  } else {
    ia = static_cast<i64>(a);  // C++ always truncates (i.e., rounds to zero).
  }

  f32 r = static_cast<f32>(ia) - a;
  if (!IsZero(r))
    inexact = true;

  if constexpr (rm == kRoundTowardNegative) {
    if (r > 0.f32)
      ia -= 1;
  } else if constexpr (rm == kRoundTowardPositive) {
    if (r < 0.f32)
      ia += 1;
  }

  return ia;
}

template i64 FloppyFloat::F32ToI64<FloppyFloat::kRoundTiesToEven>(f32 a);
template i64 FloppyFloat::F32ToI64<FloppyFloat::kRoundTowardPositive>(f32 a);
template i64 FloppyFloat::F32ToI64<FloppyFloat::kRoundTowardNegative>(f32 a);
template i64 FloppyFloat::F32ToI64<FloppyFloat::kRoundTowardZero>(f32 a);
template i64 FloppyFloat::F32ToI64<FloppyFloat::kRoundTiesToAway>(f32 a);

template <typename FT, FloppyFloat::RoundingMode rm>
bool ResultOutOfURange(FT a) {
  if constexpr (rm == FloppyFloat::kRoundTowardZero || rm == FloppyFloat::kRoundTowardPositive) {
    return a <= -1.f;
  } else if constexpr (rm == FloppyFloat::kRoundTowardNegative) {
    return true;
  } else if constexpr (rm == FloppyFloat::kRoundTiesToEven) {
    return a < -0.5f;
  } else if constexpr (rm == FloppyFloat::kRoundTiesToAway) {
    return a <= -0.5f;
  } else {
    static_assert("Using unsupported rounding mode");
  }
}

u32 FloppyFloat::F32ToU32(f32 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return F32ToU32<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return F32ToU32<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return F32ToU32<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return F32ToU32<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return F32ToU32<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr u32 FloppyFloat::F32ToU32(f32 a) {
  if (IsNan(a)) [[unlikely]] {
    invalid = true;
    return nan_limit_u32_;
  } else if (a >= 4294967296.f32) [[unlikely]] {
    invalid = true;
    return max_limit_u32_;
  } else if (a < 0.f32) [[unlikely]] {
    if (ResultOutOfURange<f32, rm>(a)) {
      invalid = true;
      return min_limit_u32_;
    }
    inexact = true;
    return std::numeric_limits<u32>::min();
  }

  u32 ia;
  if constexpr (rm == kRoundTiesToEven) {
    ia = static_cast<u32>(std::llrint(a));
  } else if constexpr (rm == kRoundTiesToAway) {
    ia = static_cast<u32>(std::llround(a));
  } else {
    ia = static_cast<u32>(a);  // C++ always truncates (i.e., rounds to zero).
  }

  f32 r = static_cast<f32>(ia) - a;
  if (!IsZero(r))
    inexact = true;

  if constexpr (rm == kRoundTowardNegative) {
    if (r > 0.f32)
      ia -= 1;
  } else if constexpr (rm == kRoundTowardPositive) {
    if (r < 0.f32)
      ia += 1;
  }

  return ia;
}

template u32 FloppyFloat::F32ToU32<FloppyFloat::kRoundTiesToEven>(f32 a);
template u32 FloppyFloat::F32ToU32<FloppyFloat::kRoundTowardPositive>(f32 a);
template u32 FloppyFloat::F32ToU32<FloppyFloat::kRoundTowardNegative>(f32 a);
template u32 FloppyFloat::F32ToU32<FloppyFloat::kRoundTowardZero>(f32 a);
template u32 FloppyFloat::F32ToU32<FloppyFloat::kRoundTiesToAway>(f32 a);

u64 FloppyFloat::F32ToU64(f32 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return F32ToU64<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return F32ToU64<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return F32ToU64<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return F32ToU64<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return F32ToU64<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr u64 FloppyFloat::F32ToU64(f32 a) {
  if (IsNan(a)) [[unlikely]] {
    invalid = true;
    return nan_limit_u64_;
  } else if (a >= 18446744073709551616.0f32) [[unlikely]] {
    invalid = true;
    return max_limit_u64_;
  } else if (a < 0.f) [[unlikely]] {
    if (ResultOutOfURange<f32, rm>(a)) {
      invalid = true;
      return min_limit_u64_;
    }
    inexact = true;
    return std::numeric_limits<u64>::min();
  }

  u64 ia = static_cast<u64>(a);  // C++ always truncates (i.e., rounds to zero).

  f32 r = static_cast<f32>(ia) - a;
  if (!IsZero(r))
    inexact = true;

  return RoundIntegerResult<f32, u64, rm>(r, a, ia);
}

template u64 FloppyFloat::F32ToU64<FloppyFloat::kRoundTiesToEven>(f32 a);
template u64 FloppyFloat::F32ToU64<FloppyFloat::kRoundTowardPositive>(f32 a);
template u64 FloppyFloat::F32ToU64<FloppyFloat::kRoundTowardNegative>(f32 a);
template u64 FloppyFloat::F32ToU64<FloppyFloat::kRoundTowardZero>(f32 a);
template u64 FloppyFloat::F32ToU64<FloppyFloat::kRoundTiesToAway>(f32 a);

f64 FloppyFloat::F32ToF64(f32 a) {
  if (IsNan(a)) [[unlikely]] {
    if (!GetQuietBit(a))
      invalid = true;
    return PropagateNan(a);
  }

  return static_cast<f64>(a);
}

f16 FloppyFloat::F32ToF16(f32 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return F32ToF16<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return F32ToF16<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return F32ToF16<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return F32ToF16<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return F32ToF16<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr f16 FloppyFloat::F32ToF16(f32 a) {
  if (IsNan(a)) [[unlikely]] {
    if (!GetQuietBit(a))
      invalid = true;
    return GetQnan<f16>();
  }

  f16 result = static_cast<f16>(a);

  if (IsInfOrNan(result)) [[unlikely]] {
    if (IsNan(result)) {
      if (!GetQuietBit(a))
        invalid = true;
      return GetQnan<f16>();
    } else {  // Infinity case.
      if (!IsInf(a)) {
        overflow = true;
        inexact = true;
        result = RoundInf<f16, rm>(result);
      }
      return result;
    }
  }

  f32 residual = static_cast<f32>(result) - a;
  if (residual != 0.f32)
    inexact = true;

  result = RoundResult<f16, f32, rm>(residual, result);

  if (!underflow) {
    if (std::abs(result) <= nl<f16>::min()) [[unlikely]] {
      if (std::abs(result) == nl<f16>::min()) {
        residual = static_cast<f32>(result) - a;
        if (std::signbit(residual) == std::signbit(result))
          underflow = true;
      } else {
        if (residual != 0.f32)
          underflow = true;
      }
    }
  }

  return result;
}

template f16 FloppyFloat::F32ToF16<FloppyFloat::kRoundTiesToEven>(f32 a);
template f16 FloppyFloat::F32ToF16<FloppyFloat::kRoundTowardPositive>(f32 a);
template f16 FloppyFloat::F32ToF16<FloppyFloat::kRoundTowardNegative>(f32 a);
template f16 FloppyFloat::F32ToF16<FloppyFloat::kRoundTowardZero>(f32 a);
// template f16 FloppyFloat::F32ToF16<FloppyFloat::kRoundTiesToAway>(f32 a); // TODO

template <FloppyFloat::RoundingMode rm>
constexpr f64 F64ToI32NegLimit() {
  if constexpr (rm == FloppyFloat::kRoundTiesToEven) {
    return -2147483648.5f64;
  } else if constexpr (rm == FloppyFloat::kRoundTiesToAway) {
    return -2147483648.4999995f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardNegative) {
    return -2147483648.f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardPositive) {
    return -2147483648.9999995f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardZero) {
    return -2147483648.9999995f64;
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr f64 F64ToI32PosLimit() {
  if constexpr (rm == FloppyFloat::kRoundTiesToEven) {
    return 2147483647.4999998f64;
  } else if constexpr (rm == FloppyFloat::kRoundTiesToAway) {
    return 2147483647.4999998f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardNegative) {
    return 2147483647.9999998f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardPositive) {
    return 2147483647.f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardZero) {
    return 2147483647.9999998f64;
  }
}

i32 FloppyFloat::F64ToI32(f64 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return F64ToI32<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return F64ToI32<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return F64ToI32<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return F64ToI32<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return F64ToI32<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr i32 FloppyFloat::F64ToI32(f64 a) {
  if (IsNan(a)) [[unlikely]] {
    invalid = true;
    return nan_limit_i32_;
  } else if (a > F64ToI32PosLimit<rm>()) [[unlikely]] {
    invalid = true;
    return max_limit_i32_;
  } else if (a < F64ToI32NegLimit<rm>()) [[unlikely]] {
    invalid = true;
    return min_limit_i32_;
  }

  i32 ia;
  if constexpr (rm == kRoundTiesToAway) {
    ia = std::lround(a);
  } else if (rm == kRoundTiesToEven) {
    ia = std::lrint(a);
  } else {
    ia = static_cast<i32>(a);  // C++ always truncates (i.e., rounds to zero).
  }

  f64 r = static_cast<f64>(ia) - a;
  if (!IsZero(r))
    inexact = true;

  if constexpr (rm == kRoundTowardNegative) {
    if (r > 0)
      ia -= 1;
  } else if constexpr (rm == kRoundTowardPositive) {
    if (r < 0)
      ia += 1;
  }

  return ia;
}

template i32 FloppyFloat::F64ToI32<FloppyFloat::kRoundTiesToEven>(f64 a);
template i32 FloppyFloat::F64ToI32<FloppyFloat::kRoundTowardPositive>(f64 a);
template i32 FloppyFloat::F64ToI32<FloppyFloat::kRoundTowardNegative>(f64 a);
template i32 FloppyFloat::F64ToI32<FloppyFloat::kRoundTowardZero>(f64 a);
template i32 FloppyFloat::F64ToI32<FloppyFloat::kRoundTiesToAway>(f64 a);

i64 FloppyFloat::F64ToI64(f64 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return F64ToI64<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return F64ToI64<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return F64ToI64<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return F64ToI64<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return F64ToI64<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr i64 FloppyFloat::F64ToI64(f64 a) {
  if (IsNan(a)) [[unlikely]] {
    invalid = true;
    return nan_limit_i64_;
  } else if (a >= 9223372036854775808.0f64) [[unlikely]] {
    invalid = true;
    return max_limit_i64_;
  } else if (a < -9223372036854775808.0f64) [[unlikely]] {
    invalid = true;
    return min_limit_i64_;
  }

  i64 ia;
  if constexpr (rm == kRoundTiesToAway) {
    ia = std::lround(a);
  } else if (rm == kRoundTiesToEven) {
    ia = std::lrint(a);
  } else {
    ia = static_cast<i64>(a);  // C++ always truncates (i.e., rounds to zero).
  }

  f64 r = static_cast<f64>(ia) - a;
  if (!IsZero(r))
    inexact = true;

  if constexpr (rm == kRoundTowardNegative) {
    if (r > 0)
      ia -= 1;
  } else if constexpr (rm == kRoundTowardPositive) {
    if (r < 0)
      ia += 1;
  }

  return ia;
}

template i64 FloppyFloat::F64ToI64<FloppyFloat::kRoundTiesToEven>(f64 a);
template i64 FloppyFloat::F64ToI64<FloppyFloat::kRoundTowardPositive>(f64 a);
template i64 FloppyFloat::F64ToI64<FloppyFloat::kRoundTowardNegative>(f64 a);
template i64 FloppyFloat::F64ToI64<FloppyFloat::kRoundTowardZero>(f64 a);
template i64 FloppyFloat::F64ToI64<FloppyFloat::kRoundTiesToAway>(f64 a);

template <FloppyFloat::RoundingMode rm>
constexpr f64 F64ToU32NegLimit() {
  if constexpr (rm == FloppyFloat::kRoundTiesToEven) {
    return -0.5f64;
  } else if constexpr (rm == FloppyFloat::kRoundTiesToAway) {
    return -0.49999999999999994f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardNegative) {
    return -0.0f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardPositive) {
    return -0.9999999999999999f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardZero) {
    return -0.9999999999999999f64;
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr f64 F64ToU32PosLimit() {
  if constexpr (rm == FloppyFloat::kRoundTiesToEven) {
    return 4294967295.4999995f64;
  } else if constexpr (rm == FloppyFloat::kRoundTiesToAway) {
    return 4294967295.4999995f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardNegative) {
    return 4294967295.9999995f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardPositive) {
    return 4294967295.f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardZero) {
    return 4294967295.9999995f64;
  }
}

u32 FloppyFloat::F64ToU32(f64 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return F64ToU32<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return F64ToU32<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return F64ToU32<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return F64ToU32<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return F64ToU32<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr u32 FloppyFloat::F64ToU32(f64 a) {
  if (IsNan(a)) [[unlikely]] {
    invalid = true;
    return nan_limit_u32_;
  } else if (a > 4294967295.f64) [[unlikely]] {
    if (a > F64ToU32PosLimit<rm>()) {
      invalid = true;
      return max_limit_u32_;
    }
    inexact = true;
    return std::numeric_limits<u32>::max();
  } else if (a < 0.f64) [[unlikely]] {
    if (a < F64ToU32NegLimit<rm>()) {
      invalid = true;
      return min_limit_u32_;
    }
    inexact = true;
    return std::numeric_limits<u32>::min();
  }

  u32 ia = static_cast<u32>(a);  // C++ always truncates (i.e., rounds to zero).

  f64 r = static_cast<f64>(ia) - a;
  if (!IsZero(r))
    inexact = true;

  return RoundIntegerResult<f64, u32, rm>(r, a, ia);
}

template u32 FloppyFloat::F64ToU32<FloppyFloat::kRoundTiesToEven>(f64 a);
template u32 FloppyFloat::F64ToU32<FloppyFloat::kRoundTowardPositive>(f64 a);
template u32 FloppyFloat::F64ToU32<FloppyFloat::kRoundTowardNegative>(f64 a);
template u32 FloppyFloat::F64ToU32<FloppyFloat::kRoundTowardZero>(f64 a);
template u32 FloppyFloat::F64ToU32<FloppyFloat::kRoundTiesToAway>(f64 a);

template <FloppyFloat::RoundingMode rm>
constexpr f64 F64ToU64PosLimit() {
  if constexpr (rm == FloppyFloat::kRoundTiesToEven) {
    return 4294967295.4999995f64;
  } else if constexpr (rm == FloppyFloat::kRoundTiesToAway) {
    return 4294967295.4999995f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardNegative) {
    return 4294967295.9999995f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardPositive) {
    return 4294967295.f64;
  } else if constexpr (rm == FloppyFloat::kRoundTowardZero) {
    return 4294967295.9999995f64;
  }
}

u64 FloppyFloat::F64ToU64(f64 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return F64ToU64<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return F64ToU64<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return F64ToU64<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return F64ToU64<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return F64ToU64<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr u64 FloppyFloat::F64ToU64(f64 a) {
  if (IsNan(a)) [[unlikely]] {
    invalid = true;
    return nan_limit_u64_;
  } else if (a > 18446744073709551616.f64) [[unlikely]] {
    invalid = true;
    return max_limit_u64_;
  } else if (a < 0.f64) [[unlikely]] {
    if (a < F64ToU32NegLimit<rm>()) {
      invalid = true;
      return min_limit_u64_;
    }
    inexact = true;
    return std::numeric_limits<u64>::min();
  }

  u64 ia;
  ia = static_cast<u64>(a);  // C++ always truncates (i.e., rounds to zero).

  f64 r = static_cast<f64>(ia) - a;
  if (!IsZero(r))
    inexact = true;

  return RoundIntegerResult<f64, u64, rm>(r, a, ia);
}

template u64 FloppyFloat::F64ToU64<FloppyFloat::kRoundTiesToEven>(f64 a);
template u64 FloppyFloat::F64ToU64<FloppyFloat::kRoundTowardPositive>(f64 a);
template u64 FloppyFloat::F64ToU64<FloppyFloat::kRoundTowardNegative>(f64 a);
template u64 FloppyFloat::F64ToU64<FloppyFloat::kRoundTowardZero>(f64 a);
template u64 FloppyFloat::F64ToU64<FloppyFloat::kRoundTiesToAway>(f64 a);

f16 FloppyFloat::I32ToF16(i32 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return I32ToF16<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return I32ToF16<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return I32ToF16<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return I32ToF16<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return I32ToF16<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr f16 FloppyFloat::I32ToF16(i32 a) {
  f16 af = static_cast<f16>(a);
  u32 ua = std::abs(a);
  u32 shifted_ua = ua << std::countl_zero(ua);
  u32 r = shifted_ua & 0x1fffffu;

  if (r != 0) {
    inexact = true;
    if constexpr (rm == kRoundTiesToEven) {
      return af;
    }
    bool even = shifted_ua & 0x100u;
    if constexpr (rm == kRoundTowardPositive) {
      if (a > 0) {
        if ((r < 1048576) || ((r == 1048576) && !even))
          af = NextUpNoNegZero(af);
      } else {
        if ((r > 1048576) || ((r == 1048576) && even))
          af = NextUpNoNegZero(af);
      }
    } else if constexpr (rm == kRoundTowardNegative) {
      if (a > 0) {
        if ((r > 1048576) || ((r == 1048576) && even))
          af = NextDownNoPosZero(af);
      } else {
        if ((r < 1048576) || ((r == 1048576) && !even))
          af = NextDownNoPosZero(af);
      }
    } else if constexpr (rm == kRoundTowardZero) {
      if ((r > 1048576) || ((r == 1048576) && even)) {
        if (a > 0) {
          af = NextDownNoPosZero(af);
        } else {
          af = NextUpNoNegZero(af);
        }
      }
    } else if constexpr (rm == kRoundTiesToAway) {
      if ((a > 0) && (r == 1048576) && !even)
        af = NextUpNoNegZero(af);
      if ((a < 0) && (r == 1048576) && even)
        af = NextDownNoPosZero(af);
    }
  }

  return af;
}

template f16 FloppyFloat::I32ToF16<FloppyFloat::kRoundTiesToEven>(i32 a);
template f16 FloppyFloat::I32ToF16<FloppyFloat::kRoundTowardPositive>(i32 a);
template f16 FloppyFloat::I32ToF16<FloppyFloat::kRoundTowardNegative>(i32 a);
template f16 FloppyFloat::I32ToF16<FloppyFloat::kRoundTowardZero>(i32 a);
template f16 FloppyFloat::I32ToF16<FloppyFloat::kRoundTiesToAway>(i32 a);

f32 FloppyFloat::I32ToF32(i32 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return I32ToF32<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return I32ToF32<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return I32ToF32<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return I32ToF32<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return I32ToF32<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr f32 FloppyFloat::I32ToF32(i32 a) {
  f32 af = static_cast<f32>(a);
  u32 ua = std::abs(a);
  u32 shifted_ua = ua << std::countl_zero(ua);
  u32 r = shifted_ua & 0xffu;

  if (r != 0) {
    inexact = true;
    if constexpr (rm == kRoundTiesToEven) {
      return af;
    }
    bool even = shifted_ua & 0x100u;
    if constexpr (rm == kRoundTowardPositive) {
      if (a > 0) {
        if ((r < 128) || ((r == 128) && !even))
          af = NextUpNoNegZero(af);
      } else {
        if ((r > 128) || ((r == 128) && even))
          af = NextUpNoNegZero(af);
      }
    } else if constexpr (rm == kRoundTowardNegative) {
      if (a > 0) {
        if ((r > 128) || ((r == 128) && even))
          af = NextDownNoPosZero(af);
      } else {
        if ((r < 128) || ((r == 128) && !even))
          af = NextDownNoPosZero(af);
      }
    } else if constexpr (rm == kRoundTowardZero) {
      if ((r > 128) || ((r == 128) && even)) {
        if (a > 0) {
          af = NextDownNoPosZero(af);
        } else {
          af = NextUpNoNegZero(af);
        }
      }
    } else if constexpr (rm == kRoundTiesToAway) {
      if ((a > 0) && (r == 128) && !even)
        af = NextUpNoNegZero(af);
      if ((a < 0) && (r == 128) && even)
        af = NextDownNoPosZero(af);
    }
  }

  return af;
}

template f32 FloppyFloat::I32ToF32<FloppyFloat::kRoundTiesToEven>(i32 a);
template f32 FloppyFloat::I32ToF32<FloppyFloat::kRoundTowardPositive>(i32 a);
template f32 FloppyFloat::I32ToF32<FloppyFloat::kRoundTowardNegative>(i32 a);
template f32 FloppyFloat::I32ToF32<FloppyFloat::kRoundTowardZero>(i32 a);
template f32 FloppyFloat::I32ToF32<FloppyFloat::kRoundTiesToAway>(i32 a);

f64 FloppyFloat::I32ToF64(i32 a) {
  return static_cast<f64>(a);
}

f32 FloppyFloat::U32ToF32(u32 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return U32ToF32<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return U32ToF32<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return U32ToF32<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return U32ToF32<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return U32ToF32<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr f32 FloppyFloat::U32ToF32(u32 a) {
  f32 af = static_cast<f32>(a);
  u32 shifted_ua = a << std::countl_zero(a);
  u32 r = shifted_ua & 0xffu;

  if (r != 0) {
    inexact = true;
    if constexpr (rm == kRoundTiesToEven) {
      return af;
    }
    bool even = shifted_ua & 0x100u;
    if constexpr (rm == kRoundTowardPositive) {
      if ((r < 128) || ((r == 128) && !even))
        af = NextUpNoNegZero(af);
    } else if constexpr (rm == kRoundTowardNegative) {
      if ((r > 128) || ((r == 128) && even))
        af = NextDownNoPosZero(af);
    } else if constexpr (rm == kRoundTowardZero) {
      if ((r > 128) || ((r == 128) && even))
        af = NextDownNoPosZero(af);
    } else if constexpr (rm == kRoundTiesToAway) {
      if ((r == 128) && !even)
        af = NextUpNoNegZero(af);
    }
  }

  return af;
}

template f32 FloppyFloat::U32ToF32<FloppyFloat::kRoundTiesToEven>(u32 a);
template f32 FloppyFloat::U32ToF32<FloppyFloat::kRoundTowardPositive>(u32 a);
template f32 FloppyFloat::U32ToF32<FloppyFloat::kRoundTowardNegative>(u32 a);
template f32 FloppyFloat::U32ToF32<FloppyFloat::kRoundTowardZero>(u32 a);
template f32 FloppyFloat::U32ToF32<FloppyFloat::kRoundTiesToAway>(u32 a);

f32 FloppyFloat::U64ToF32(u64 a) {
  switch (rounding_mode) {
  case kRoundTiesToEven:
    return U64ToF32<kRoundTiesToEven>(a);
  case kRoundTiesToAway:
    return U64ToF32<kRoundTiesToAway>(a);
  case kRoundTowardPositive:
    return U64ToF32<kRoundTowardPositive>(a);
  case kRoundTowardNegative:
    return U64ToF32<kRoundTowardNegative>(a);
  case kRoundTowardZero:
    return U64ToF32<kRoundTowardZero>(a);
  default:
    throw std::runtime_error(std::string("Unknown rounding mode"));
  }
}

template <FloppyFloat::RoundingMode rm>
constexpr f32 FloppyFloat::U64ToF32(u64 a) {
  f32 af = static_cast<f32>(a);
  u64 shifted_ua = a << std::countl_zero(a);
  u64 r = shifted_ua & 0x7ffull;

  if (r != 0) {
    inexact = true;
    if constexpr (rm == kRoundTiesToEven) {
      return af;
    }
    bool even = shifted_ua & 0x100u;
    if constexpr (rm == kRoundTowardPositive) {
      if ((r < 1024) || ((r == 1024) && !even))
        af = NextUpNoNegZero(af);
    } else if constexpr (rm == kRoundTowardNegative) {
      if ((r > 1024) || ((r == 1024) && even))
        af = NextDownNoPosZero(af);
    } else if constexpr (rm == kRoundTowardZero) {
      if ((r > 1024) || ((r == 1024) && even))
        af = NextDownNoPosZero(af);
    } else if constexpr (rm == kRoundTiesToAway) {
      if ((r == 1024) && !even)
        af = NextUpNoNegZero(af);
    }
  }

  return af;
}

template f32 FloppyFloat::U64ToF32<FloppyFloat::kRoundTiesToEven>(u64 a);
template f32 FloppyFloat::U64ToF32<FloppyFloat::kRoundTowardPositive>(u64 a);
template f32 FloppyFloat::U64ToF32<FloppyFloat::kRoundTowardNegative>(u64 a);
template f32 FloppyFloat::U64ToF32<FloppyFloat::kRoundTowardZero>(u64 a);
template f32 FloppyFloat::U64ToF32<FloppyFloat::kRoundTiesToAway>(u64 a);

f64 FloppyFloat::U32ToF64(u32 a) {
  return static_cast<f64>(a);
}

template <typename FT>
u32 FloppyFloat::Class(FT a) {
  bool sign = std::signbit(a);
  bool is_inf = IsInf(a);
  bool is_tiny = IsTiny(a);
  bool is_zero = IsZero(a);
  bool is_subn = IsSubnormal(a);

  return (sign && is_inf) << 0 | (sign && !is_inf && !is_tiny) << 1 | (sign && is_subn) << 2 | (sign && is_zero) << 3 |
         (!sign && is_zero) << 4 | (!sign && is_subn) << 5 | (!sign && !is_inf && !is_tiny) << 6 |
         (!sign && is_inf) << 7 | (IsSnan(a)) << 8 | (IsNan(a) && !IsSnan(a)) << 9;
}

/**************************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 **************************************************************************************************/

#include "floppy_float.h"

#include <bit>
#include <cmath>
#include <stdexcept>

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
constexpr FT UpMulFma(FT a, FT b, FT c) {
  auto r = std::fma(a, b, -c);
  return r;
}

template <typename FT>
constexpr TwiceWidthType<FT>::type UpDiv(FT a, FT b, FT c) {
  auto da = static_cast<TwiceWidthType<FT>::type>(a);
  auto db = static_cast<TwiceWidthType<FT>::type>(b);
  auto dc = static_cast<TwiceWidthType<FT>::type>(c);
  auto r = da - dc * db;
  return std::signbit(b) ? -r : r;
}

template <typename FT>
constexpr FT UpDivFma(FT a, FT b, FT c) {
  auto r = std::fma(c, b, -a);
  return r;
}

template <typename FT>
constexpr TwiceWidthType<FT>::type UpSqrt(FT a, FT b) {
  auto da = static_cast<TwiceWidthType<FT>::type>(a);
  auto db = static_cast<TwiceWidthType<FT>::type>(b);
  auto r = da - db * db;
  return r;
}

template <typename FT>
constexpr FT UpSqrtFma(FT a, FT b) {
  auto r = std::fma(b, b, -a);
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
  } else if constexpr (rm == FloppyFloat::kRoundTowardPositive) {
    return IsNegInf(result) ? nl<FT>::lowest() : result;
  } else if constexpr (rm == FloppyFloat::kRoundTowardNegative) {
    return IsPosInf(result) ? nl<FT>::max() : result;
  } else if constexpr (rm == FloppyFloat::kRoundTowardZero) {
    return IsNegInf(result) ? nl<FT>::lowest() : nl<FT>::max();
  } else {
    static_assert("Using unsupported rounding mode");
  }
}

template <typename FT, typename TFT, FloppyFloat::RoundingMode rm>
constexpr FT FloppyFloat::RoundResult([[maybe_unused]] TFT residual, FT result) {
  if constexpr (rm == kRoundTiesToEven) {
    // Nothing to do.
  } else if constexpr (rm == kRoundTowardPositive) {
    if (residual > 0.) {
      auto uresult = std::bit_cast<typename FloatToUint<FT>::type>(result);
      uresult += result >= 0. ? 1 : -1;  // Next up of result cannot be -0.
      result = std::bit_cast<FT>(uresult);
      overflow = (result == nl<FT>::infinity()) ? true : overflow;
    }
  } else if constexpr (rm == kRoundTowardNegative) {
    if (residual < 0.) {
      auto uresult = std::bit_cast<typename FloatToUint<FT>::type>(result);
      uresult -= result > 0. ? 1 : -1;  // Nextdown of result cannot be +0.
      result = std::bit_cast<FT>(uresult);
      overflow = (result == -nl<FT>::infinity()) ? true : overflow;
    }
  } else if constexpr (rm == kRoundTowardZero) {
    if (residual > 0. && result < 0.) {
      auto uresult = std::bit_cast<typename FloatToUint<FT>::type>(result);
      uresult += result >= 0. ? 1 : -1;  // Nextup of result cannot be -0.
      result = std::bit_cast<FT>(uresult);
      overflow = (result == nl<FT>::infinity()) ? true : overflow;
    } else if (residual < 0 && result > 0) {  // Fix a round-up.
      auto uresult = std::bit_cast<typename FloatToUint<FT>::type>(result);
      uresult -= result > 0. ? 1 : -1;  // Nextdown of result cannot be +0.
      result = std::bit_cast<FT>(uresult);
      overflow = (result == -nl<FT>::infinity()) ? true : overflow;
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
  } else {
    throw std::runtime_error(std::string("Unknown NaN propagation scheme"));
  }
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
        return PropagateNan<FT>(a, b);
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
      c = RoundResult<FT, FT, rm>(r, c);
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
        return PropagateNan<FT>(a, b);
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
    FT r = TwoSum(a, -b, c);
    if (r != 0) {
      inexact = true;
      c = RoundResult<FT, FT, rm>(r, c);
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
      if (IsSnan(a) || IsSnan(b))
        invalid = true;
      if (IsNan(a) || IsNan(b))
        return PropagateNan<FT>(a, b);
      invalid = true;
      return GetQnan<FT>();
    }
  }

  if constexpr (rm == kRoundTiesToEven) {
    if (!inexact) [[unlikely]] {
      auto r = UpMul<FT>(a, b, c);
      if (r != 0.)
        inexact = true;
    }
    if (!underflow) {
      if (IsSubnormal(c)) [[unlikely]] {
        auto r = UpMul<FT>(a, b, c);
        if (r != 0.)
          underflow = true;
      }
    }
  } else {
    auto r = UpMul(a, b, c);
    if (r != 0.) {
      inexact = true;
      c = RoundResult<FT, typename TwiceWidthType<FT>::type, rm>(r, c);
      if (IsSubnormal(c)) [[unlikely]] {
        if (r != 0.)
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

template <typename FT, FloppyFloat::RoundingMode rm>
FT FloppyFloat::Div(FT a, FT b) {
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
    } else {  // NaN case.
      if (IsSnan(a) || IsSnan(b))
        invalid = true;
      if (IsNan(a) || IsNan(b))
        return PropagateNan<FT>(a, b);
      invalid = true;
      return GetQnan<FT>();
    }
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
      if (IsSubnormal(c)) [[unlikely]] {
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
      if (IsSubnormal(c)) [[unlikely]] {
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

template <typename FT, FloppyFloat::RoundingMode rm>
FT FloppyFloat::Sqrt(FT a) {
  FT b = std::sqrt(a);

  if (IsNan(b)) [[unlikely]] {
    if (IsSnan(a))
      invalid = true;
    if (IsNan(a))
      return SetQuietBit(a);
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
bool FloppyFloat::Eq(FT a, FT b, bool quiet) {
  if (IsNan(a) || IsNan(b)) {
    if (!quiet || IsSnan(a) || IsSnan(b))
      invalid = true;
    return false;
  }

  return a == b;
}

template bool FloppyFloat::Eq<f16>(f16 a, f16 b, bool quiet);
template bool FloppyFloat::Eq<f32>(f32 a, f32 b, bool quiet);
template bool FloppyFloat::Eq<f64>(f64 a, f64 b, bool quiet);

template <typename FT>
bool FloppyFloat::Le(FT a, FT b, bool quiet) {
  if (IsNan(a) || IsNan(b)) {
    if (!quiet || IsSnan(a) || IsSnan(b))
      invalid = true;
    return false;
  }

  return a <= b;
}

template bool FloppyFloat::Le<f16>(f16 a, f16 b, bool quiet);
template bool FloppyFloat::Le<f32>(f32 a, f32 b, bool quiet);
template bool FloppyFloat::Le<f64>(f64 a, f64 b, bool quiet);

template <typename FT>
bool FloppyFloat::Lt(FT a, FT b, bool quiet) {
  if (IsNan(a) || IsNan(b)) {
    if (!quiet || IsSnan(a) || IsSnan(b))
      invalid = true;
    return false;
  }

  return a < b;
}

template bool FloppyFloat::Lt<f16>(f16 a, f16 b, bool quiet);
template bool FloppyFloat::Lt<f32>(f32 a, f32 b, bool quiet);
template bool FloppyFloat::Lt<f64>(f64 a, f64 b, bool quiet);

template <typename FT>
FT FloppyFloat::Maxx86(FT a, FT b) {
  if (IsNan(a) || IsNan(b)) {
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
  if (IsNan(a) || IsNan(b)) {
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
  if (IsNan(a) || IsNan(b)) {
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
  if (IsNan(a) || IsNan(b)) {
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

void FloppyFloat::SetupToArm() {
  SetQnan<f16>(0x7e00u);
  SetQnan<f32>(0x7fc00000u);
  SetQnan<f64>(0x7ff8000000000000ull);
  tininess_before_rounding = true;
  nan_propagation_scheme = kNanPropRiscv;  // Shares the same NaN propagation as ARM.
}

void FloppyFloat::SetupToRiscv() {
  SetQnan<f16>(0x7e00u);
  SetQnan<f32>(0x7fc00000u);
  SetQnan<f64>(0x7ff8000000000000ull);
  tininess_before_rounding = false;
  nan_propagation_scheme = kNanPropRiscv;
}

void FloppyFloat::SetupTox86() {
  SetQnan<f16>(0xfe00u);
  SetQnan<f32>(0xffc00000u);
  SetQnan<f64>(0xfff8000000000000ull);
  tininess_before_rounding = false;
  nan_propagation_scheme = kNanPropX86sse;
}

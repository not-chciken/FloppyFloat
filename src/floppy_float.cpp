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
TwiceWidthType<FT> UpMul(FT a, FT b, FT c) {
  auto da = static_cast<TwiceWidthType<FT>>(a);
  auto db = static_cast<TwiceWidthType<FT>>(b);
  auto dc = static_cast<TwiceWidthType<FT>>(c);
  auto r = da * db - dc;
  return r;
}

FloppyFloat::FloppyFloat() {
  qnan32 = std::bit_cast<f32>(0x7fc00000u);
  qnan64 = std::bit_cast<f64>(0x7ff8000000000000ull);
}

template <typename FT, FloppyFloat::RoundingMode rm>
constexpr FT RoundInf(FT result);

template<>
constexpr f32 RoundInf<f32, FloppyFloat::kRoundTiesToEven>(f32 result) {
  return result;
}

template<>
constexpr f32 RoundInf<f32, FloppyFloat::kRoundTowardPositive>(f32 result) {
  return IsNegInf(result) ? nl<f32>::lowest() : result;
}

template<>
constexpr f32 RoundInf<f32, FloppyFloat::kRoundTowardNegative>(f32 result) {
  return IsPosInf(result) ? nl<f32>::max() : result;
}

template<>
constexpr f32 RoundInf<f32, FloppyFloat::kRoundTowardZero>(f32 result) {
  return IsNegInf(result) ? nl<f32>::lowest() : nl<f32>::max();
}

template<>
constexpr f64 RoundInf<f64, FloppyFloat::kRoundTiesToEven>(f64 result) {
  return result;
}

template<>
constexpr f64 RoundInf<f64, FloppyFloat::kRoundTowardPositive>(f64 result) {
  return IsNegInf(result) ? nl<f64>::lowest() : result;
}

template<>
constexpr f64 RoundInf<f64, FloppyFloat::kRoundTowardNegative>(f64 result) {
  return IsPosInf(result) ? nl<f64>::max() : result;
}

template<>
constexpr f64 RoundInf<f64, FloppyFloat::kRoundTowardZero>(f64 result) {
  return IsNegInf(result) ? nl<f64>::lowest() : nl<f64>::max();
}

template <typename FT, FloppyFloat::RoundingMode rm>
constexpr FT FloppyFloat::RoundResult([[maybe_unused]] FT residual, FT result) {
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
      uresult -= result > 0. ? 1 : -1;  // Nextdown. c.f cannot be 0.
      result = std::bit_cast<FT>(uresult);
      overflow = (result == -nl<FT>::infinity()) ? true : overflow;
    }
  }
  if constexpr (rm == kRoundTowardZero) {
    if (residual > 0. && result < 0.) {
      auto uresult = std::bit_cast<typename FloatToUint<FT>::type>(result);
      uresult += result > 0. ? 1 : -1;  // Nextup. c.f cannot be 0.
      result = std::bit_cast<FT>(uresult);
      overflow = (result == nl<FT>::infinity()) ? true : overflow;
    } else if (residual < 0 && result > 0) {  // Fix a round-up.
      auto uresult = std::bit_cast<typename FloatToUint<FT>::type>(result);
      uresult -= result > 0. ? 1 : -1;  // Nextdown. c.f cannot be 0.
      result = std::bit_cast<FT>(uresult);
      overflow = (result == -nl<FT>::infinity()) ? true : overflow;
    }
  }
  return result;
}

void FloppyFloat::SetQnan32(u32 val) {
  qnan32 = std::bit_cast<f32>(val);
}

void FloppyFloat::SetQnan64(u64 val) {
  qnan64 = std::bit_cast<f64>(val);
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
        return qnan32;
      }
      if (IsSnan(a) || IsSnan(b))
        invalid = true;
      if (IsNan(a) || IsNan(b))
        return qnan32;
    }
  }

  // See: IEEE 754-2019: 6.3 The sign bit
  if constexpr (rm == kRoundTowardNegative) {
    if (IsZero(c) && IsPos(c)) {
      if (IsNeg(a) || IsNeg(b))
        c = -c;
    }
  }

  if constexpr (rm == kRoundTiesToEven) {
    if (!inexact) [[unlikely]] {
      FT r = TwoSum(a, b, c);
      if (r != 0)
        inexact = true;
    }
  } else {
    FT r = TwoSum(a, b, c);
    if (r != 0) {
      inexact = true;
      RoundResult<FT, rm>(r, c);
    }
  }

  return c;
}

template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTiesToEven>(f32 a, f32 b);
template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTowardPositive>(f32 a, f32 b);
template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTowardNegative>(f32 a, f32 b);
template f32 FloppyFloat::Add<f32, FloppyFloat::kRoundTowardZero>(f32 a, f32 b);
template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTiesToEven>(f64 a, f64 b);
template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTowardPositive>(f64 a, f64 b);
template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTowardNegative>(f64 a, f64 b);
template f64 FloppyFloat::Add<f64, FloppyFloat::kRoundTowardZero>(f64 a, f64 b);

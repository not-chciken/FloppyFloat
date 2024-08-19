#include "soft_float.h"

#include <utility>

using namespace FfUtils;

SoftFloat::SoftFloat() : Vfpu() {
}

template <typename FT, typename UT>
constexpr UT SoftFloat::NormalizeSubnormal(i32& exp, UT mant) {
  int shift = NumSignificandBits<FT>() - (NumBits<FT>() - 1 - std::countl_zero(mant));
  exp = 1 - shift;
  return mant << shift;
}

template <typename FT, typename UT>
constexpr FT SoftFloat::Normalize(u32 a_sign, i32 a_exp, UT a_mant) {
  int shift = std::countl_zero(a_mant) - (NumBits<FT>() - 1 - NumImantBits<FT>());
  return RoundPack<FT>(a_sign, a_exp - shift, (UT)(a_mant << shift));
}

template <typename UT>
constexpr UT RshiftRnd(UT a, int d) {
  if (d == 0)
    return a;
  if (d >= NumBits<UT>())
    return !!a;

  UT mask = (1ull << d) - 1ull;
  return (a >> d) | !!(a & mask);
}

template <typename UT>
constexpr std::pair<UT, UT> Umul(UT a, UT b) {
  auto ta = static_cast<typename TwiceWidthType<UT>::type>(a);
  auto tb = static_cast<typename TwiceWidthType<UT>::type>(b);
  auto r = ta * tb;
  return std::make_pair(r, r >> NumBits<UT>());
}

template <typename UT>
constexpr std::pair<UT, UT> DivRem(UT ah, UT al, UT b) {
    using UTT = typename TwiceWidthType<UT>::type;
    UTT a = static_cast<UTT>(ah) << NumBits<UT>() | al;
    return std::make_pair(a / b, a % b);
}

template <typename FT, typename UT>
FT SoftFloat::RoundPack(bool a_sign, i32 a_exp, UT a_mant) {
  u32 addend, rnd_bits;
  switch (rounding_mode) {
  case kRoundTiesToEven:
  case kRoundTiesToAway:
    addend = 1u << (NumRoundBits<FT>() - 1);
    break;
  case kRoundTowardZero:
    addend = 0;
    break;
  default:
  case kRoundTowardNegative:
    addend = a_sign ^ 0 ? RoundMask<FT>() : 0;
    break;
  case kRoundTowardPositive:
    addend = a_sign ^ 1 ? RoundMask<FT>() : 0;
    break;
  }

  if (a_exp > 0) {
    rnd_bits = a_mant & RoundMask<FT>();
  } else {
    bool subnormal = a_exp < 0 || (a_mant + addend) < (1ull << (NumBits<FT>() - 1));
    a_mant = RshiftRnd<UT>(a_mant, 1 - a_exp);
    rnd_bits = a_mant & RoundMask<FT>();
    if (subnormal && rnd_bits)
      underflow = true;
    a_exp = 1;
  }

  if (rnd_bits)
    inexact = true;

  a_mant = (a_mant + addend) >> NumRoundBits<FT>();
  if (rounding_mode == kRoundTiesToEven && rnd_bits == 1 << (NumRoundBits<FT>() - 1))
    a_mant &= ~1;

  a_exp += a_mant >> (NumSignificandBits<FT>() + 1);
  if (a_mant <= MaxSignificand<FT>()) {
    a_exp = 0;
  } else if (a_exp >= (i32)MaxExponent<FT>()) {
    a_exp = addend ? MaxExponent<FT>() : MaxExponent<FT>() - 1;
    a_mant = addend ? 0 : MaxSignificand<FT>();
    overflow = true;
    inexact = true;
  }

  return FloatFrom3Tuple<FT>(a_sign, a_exp, a_mant);
}

template <typename FT>
FT SoftFloat::Add(FT a, FT b) {
  using UT = FloatToUint<FT>::type;
  if ((std::bit_cast<UT>(a) & ~SignMask<FT>()) < (std::bit_cast<UT>(b) & ~SignMask<FT>()))
    std::swap(a, b);

  bool a_sign = std::signbit(a);
  bool b_sign = std::signbit(b);
  i32 a_exp = GetExponent<FT>(a);
  i32 b_exp = GetExponent<FT>(b);
  UT a_mant = GetSignificand<FT>(a) << 3;
  UT b_mant = GetSignificand<FT>(b) << 3;

  if (a_exp == MaxExponent<FT>()) [[unlikely]] {
    if (a_mant != 0) {  // NaN case.
      if (!IsQnan(a) || IsSnan(b))
        invalid = true;
      return GetQnan<FT>();
    } else if ((b_exp == MaxExponent<FT>()) && (a_sign != b_sign)) {  // Infinity case.
      invalid = true;
      return GetQnan<FT>();
    }

    return a;
  }

  if (a_exp == 0)
    a_exp = 1;
  else
    a_mant |= 1ull << (NumSignificandBits<FT>() + 3);

  if (b_exp == 0)
    b_exp = 1;
  else
    b_mant |= 1ull << (NumSignificandBits<FT>() + 3);

  b_mant = RshiftRnd(b_mant, a_exp - b_exp);

  if (a_sign == b_sign)
    a_mant += b_mant;
  else {
    a_mant -= b_mant;
    if (a_mant == 0)
      a_sign = (rounding_mode == kRoundTowardNegative);
  }

  a_exp += NumRoundBits<FT>() - 3;
  return Normalize<FT>(a_sign, a_exp, a_mant);
}

template f16 SoftFloat::Add<f16>(f16 a, f16 b);
template f32 SoftFloat::Add<f32>(f32 a, f32 b);
template f64 SoftFloat::Add<f64>(f64 a, f64 b);

template <typename FT>
FT SoftFloat::Sub(FT a, FT b) {
  using UT = FloatToUint<FT>::type;
  bool a_sign = std::signbit(a);
  bool b_sign = !std::signbit(b);

  if ((std::bit_cast<UT>(a) & ~SignMask<FT>()) < (std::bit_cast<UT>(b) & ~SignMask<FT>())) {
    std::swap(a, b);
    std::swap(a_sign, b_sign);
  }

  i32 a_exp = GetExponent<FT>(a);
  i32 b_exp = GetExponent<FT>(b);
  UT a_mant = GetSignificand<FT>(a) << 3;
  UT b_mant = GetSignificand<FT>(b) << 3;

  if (a_exp == MaxExponent<FT>()) [[unlikely]] {
    if (a_mant != 0) {  // NaN case.
      if (!IsQnan(a) || IsSnan(b))
        invalid = true;
      return GetQnan<FT>();
    } else if ((b_exp == MaxExponent<FT>()) && (a_sign != b_sign)) {  // Infinity case.
      invalid = true;
      return GetQnan<FT>();
    }

    return std::copysign(a, a_sign ? -(FT)1.f : (FT)1.f);
  }

  if (a_exp == 0)
    a_exp = 1;
  else
    a_mant |= 1ull << (NumSignificandBits<FT>() + 3);

  if (b_exp == 0)
    b_exp = 1;
  else
    b_mant |= 1ull << (NumSignificandBits<FT>() + 3);

  b_mant = RshiftRnd(b_mant, a_exp - b_exp);

  if (a_sign == b_sign)
    a_mant += b_mant;
  else {
    a_mant -= b_mant;
    if (a_mant == 0)
      a_sign = (rounding_mode == kRoundTowardNegative);
  }

  a_exp += NumRoundBits<FT>() - 3;
  return Normalize<FT>(a_sign, a_exp, a_mant);
}

template f16 SoftFloat::Sub<f16>(f16 a, f16 b);
template f32 SoftFloat::Sub<f32>(f32 a, f32 b);
template f64 SoftFloat::Sub<f64>(f64 a, f64 b);

template <typename FT>
inline FT SoftFloat::Mul(FT a, FT b) {
  using UT = FloatToUint<FT>::type;
  bool a_sign = std::signbit(a);
  bool b_sign = std::signbit(b);
  bool r_sign = a_sign ^ b_sign;
  i32 a_exp = GetExponent<FT>(a);
  i32 b_exp = GetExponent<FT>(b);
  UT a_mant = GetSignificand<FT>(a);
  UT b_mant = GetSignificand<FT>(b);

  if (a_exp == MaxExponent<FT>() || b_exp == MaxExponent<FT>()) {
    if (IsNan(a) || IsNan(b)) {
      if (IsSnan(a) || IsSnan(b))
        invalid = true;
      return GetQnan<FT>();  // TODO nan prop
    } else {
      if ((a_exp == MaxExponent<FT>() && (b_exp == 0 && b_mant == 0)) ||
          (b_exp == MaxExponent<FT>() && (a_exp == 0 && a_mant == 0))) {
        invalid = true;
        return GetQnan<FT>();  // TODO nan prop
      } else {
        return FloatFrom3Tuple<FT>(r_sign, MaxExponent<FT>(), 0u);
      }
    }
  }

  if (a_exp == 0) {
    if (a_mant == 0)
      return FloatFrom3Tuple<FT>(r_sign, 0, 0);
    a_mant = NormalizeSubnormal<FT>(a_exp, a_mant);
  } else {
    a_mant |= (UT)1 << NumSignificandBits<FT>();
  }

  if (b_exp == 0) {
    if (b_mant == 0)
      return FloatFrom3Tuple<FT>(r_sign, 0, 0);
    b_mant = NormalizeSubnormal<FT>(b_exp, b_mant);
  } else {
    b_mant |= (UT)1 << NumSignificandBits<FT>();
  }

  i32 r_exp = a_exp + b_exp - (1 << (NumExponentBits<FT>() - 1)) + 2;

  auto [lo, hi] = Umul((UT)(a_mant << NumRoundBits<FT>()), (UT)(b_mant << (NumRoundBits<FT>() + 1)));

  UT r_mant = hi | !!lo;
  return Normalize<FT>(r_sign, r_exp, r_mant);
}

template f16 SoftFloat::Mul<f16>(f16 a, f16 b);
template f32 SoftFloat::Mul<f32>(f32 a, f32 b);
template f64 SoftFloat::Mul<f64>(f64 a, f64 b);

template <typename FT>
FT SoftFloat::Div(FT a, FT b) {
  using UT = FloatToUint<FT>::type;
  bool a_sign = std::signbit(a);
  bool b_sign = std::signbit(b);
  bool r_sign = a_sign ^ b_sign;
  i32 a_exp = GetExponent(a);
  i32 b_exp = GetExponent(b);
  UT a_mant = GetSignificand(a);
  UT b_mant = GetSignificand(b);

  if (a_exp == MaxExponent<FT>()) {
    if (a_mant != 0 || IsNan(b)) {
      if (IsSnan(a) || IsSnan(b))
        invalid = true;
      return GetQnan<FT>();
    } else if (b_exp == MaxExponent<FT>()) {
      invalid = true;
      return GetQnan<FT>();
    } else {
      return FloatFrom3Tuple<FT>(r_sign, MaxExponent<FT>(), 0);
    }
  } else if (b_exp == MaxExponent<FT>()) {
    if (b_mant != 0) {
      if (IsSnan(a) || IsSnan(b))
        invalid = true;
      return GetQnan<FT>();
    } else {
      return FloatFrom3Tuple<FT>(r_sign, 0, 0u);
    }
  }

  if (b_exp == 0) {
    if (b_mant == 0) {
      if (a_exp == 0 && a_mant == 0) {
        invalid = true;
        return GetQnan<FT>();
      } else {
        division_by_zero = true;
        return FloatFrom3Tuple<FT>(r_sign, MaxExponent<FT>(), 0u);
      }
    }
    b_mant = NormalizeSubnormal<FT>(b_exp, b_mant);
  } else {
    b_mant |= (UT)1 << NumSignificandBits<FT>();
  }

  if (a_exp == 0) {
    if (a_mant == 0)
      return FloatFrom3Tuple<FT>(r_sign, 0, 0u);
    a_mant = NormalizeSubnormal<FT>(a_exp, a_mant);
  } else {
    a_mant |= (UT)1 << NumSignificandBits<FT>();
  }

  i32 r_exp = a_exp - b_exp + (1 << (NumExponentBits<FT>() - 1)) - 1;
  auto [r_mant, r] = DivRem<UT>(a_mant, 0, b_mant << 2);
  if (r != 0)
    r_mant |= 1;

  return Normalize<FT>(r_sign, r_exp, r_mant);
}

template f16 SoftFloat::Div<f16>(f16 a, f16 b);
template f32 SoftFloat::Div<f32>(f32 a, f32 b);
template f64 SoftFloat::Div<f64>(f64 a, f64 b);

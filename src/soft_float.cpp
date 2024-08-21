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

template <typename FT, typename UT>
constexpr FT SoftFloat::Normalize(u32 a_sign, i32 a_exp, UT a_mant1, UT a_mant0) {
  int l = a_mant1 ? std::countl_zero(a_mant1) : NumBits<FT>() + std::countl_zero(a_mant0);
  int shift = l - (NumBits<FT>() - 1 - NumImantBits<FT>());
  if (shift == 0) {
    a_mant1 |= (a_mant0 != 0);
  } else if (shift < (i32)NumBits<FT>()) {
    a_mant1 = (a_mant1 << shift) | (a_mant0 >> (NumBits<FT>() - shift));
    a_mant0 <<= shift;
    a_mant1 |= (a_mant0 != 0);
  } else {
    a_mant1 = a_mant0 << (shift - NumBits<FT>());
  }

  return RoundPack<FT>(a_sign, a_exp - shift, a_mant1);
}

template <typename UT>
constexpr UT RshiftRnd(UT a, int d) {
  static_assert(std::is_integral_v<UT>);
  if (d == 0)
    return a;
  if (d >= NumBits<UT>())
    return !!a;

  UT mask = (1ull << d) - 1ull;
  return (a >> d) | !!(a & mask);
}

template <typename UT>
constexpr std::pair<UT, UT> Umul(UT a, UT b) {
  static_assert(std::is_integral_v<UT>);
  auto ta = static_cast<typename TwiceWidthType<UT>::type>(a);
  auto tb = static_cast<typename TwiceWidthType<UT>::type>(b);
  auto r = ta * tb;
  return std::make_pair(r, r >> NumBits<UT>());
}

template <typename UT>
constexpr std::pair<UT, UT> DivRem(UT ah, UT al, UT b) {
  static_assert(std::is_integral_v<UT>);
  using UTT = typename TwiceWidthType<UT>::type;
  UTT a = static_cast<UTT>(ah) << NumBits<UT>() | al;
  return std::make_pair(a / b, a % b);
}

template <typename UT>
constexpr bool Usqrt(UT& root, UT ah, UT al) {
  static_assert(std::is_integral_v<UT>);
  using UTT = typename TwiceWidthType<UT>::type;
  if (ah == 0 && al == 0) {
    root = 0;
    return false;
  }

  int l = ah ? NumBits<UTT>() - std::countl_zero(static_cast<UT>(ah - 1))
             : NumBits<UT>() - std::countl_zero(static_cast<UT>(al - 1));
  UTT u = 1ull << (l + 1) / 2;
  UTT a = static_cast<UTT>(ah) << NumBits<UT>() | al;
  UTT s = 0;

  do {
    s = u;
    u = (a / s + s) / 2;
  } while (u < s);

  root = s;
  return (a - s * s) != 0;
}

template <typename FT, typename UT>
FT SoftFloat::RoundPack(bool a_sign, i32 a_exp, UT a_mant) {
  u32 addend, rnd_bits;
  switch (rounding_mode) {
  case kRoundTiesToEven:
    [[fallthrough]];
  case kRoundTiesToAway:
    addend = 1u << (NumRoundBits<FT>() - 1);
    break;
  case kRoundTowardZero:
    addend = 0;
    break;
  default:
  case kRoundTowardNegative:
    addend = a_sign ? RoundMask<FT>() : 0;
    break;
  case kRoundTowardPositive:
    addend = !a_sign ? RoundMask<FT>() : 0;
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
    a_mant |= static_cast<UT>(1) << (NumSignificandBits<FT>() + 3);

  if (b_exp == 0)
    b_exp = 1;
  else
    b_mant |= static_cast<UT>(1) << (NumSignificandBits<FT>() + 3);

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
    a_mant |= static_cast<UT>(1) << (NumSignificandBits<FT>() + 3);

  if (b_exp == 0)
    b_exp = 1;
  else
    b_mant |= static_cast<UT>(1) << (NumSignificandBits<FT>() + 3);

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
    b_mant |= static_cast<UT>(1) << NumSignificandBits<FT>();
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
    a_mant |= static_cast<UT>(1) << NumSignificandBits<FT>();
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

template <typename FT>
FT SoftFloat::Sqrt(FT a) {
  using UT = FloatToUint<FT>::type;
  u32 a_sign = std::signbit(a);
  i32 a_exp = GetExponent<FT>(a);
  UT a_mant = GetSignificand<FT>(a);

  if (a_exp == MaxExponent<FT>()) {
    if (a_mant != 0) {
      if (IsSnan(a))
        invalid = true;
      return GetQnan<FT>();
    } else if (a_sign) {
      invalid = true;
      return GetQnan<FT>();
    } else {
      return a;
    }
  }

  if (a_sign) {
    if (a_exp == 0 && a_mant == 0)
      return a;  // -zero

    invalid = true;
    return GetQnan<FT>();
  }

  if (a_exp == 0) {
    if (a_mant == 0)
      return FloatFrom3Tuple<FT>(0, 0, 0);
    a_mant = NormalizeSubnormal<FT>(a_exp, a_mant);
  } else {
    a_mant |= static_cast<UT>(1) << NumSignificandBits<FT>();
  }

  a_exp -= Bias<FT>();

  if (a_exp & 1) {
    a_exp--;
    a_mant <<= 1;
  }

  a_exp = (a_exp >> 1) + Bias<FT>();
  a_mant <<= (NumBits<FT>() - 4 - NumSignificandBits<FT>());
  if (Usqrt<UT>(a_mant, a_mant, 0))
    a_mant |= 1;

  return Normalize<FT>(a_sign, a_exp, a_mant);
}

template f16 SoftFloat::Sqrt<f16>(f16 a);
template f32 SoftFloat::Sqrt<f32>(f32 a);
template f64 SoftFloat::Sqrt<f64>(f64 a);

template <typename FT>
FT SoftFloat::Fma(FT a, FT b, FT c) {
  using UT = FloatToUint<FT>::type;
  bool a_sign = std::signbit(a);
  bool b_sign = std::signbit(b);
  bool c_sign = std::signbit(c);
  bool r_sign = a_sign ^ b_sign;
  i32 a_exp = GetExponent<FT>(a);
  i32 b_exp = GetExponent<FT>(b);
  i32 c_exp = GetExponent<FT>(c);
  UT a_mant = GetSignificand<FT>(a);
  UT b_mant = GetSignificand<FT>(b);
  UT c_mant = GetSignificand<FT>(c);

  if (a_exp == MaxExponent<FT>() || b_exp == MaxExponent<FT>() || c_exp == MaxExponent<FT>()) {
    if (IsNan(a) || IsNan(b) || IsNan(c)) {
      if (IsSnan(a) || IsSnan(b) || IsSnan(c))
        invalid = true;

      if (IsNan(c) && ((IsZero(a) && IsInf(b)) || (IsZero(b) && IsInf(a)))) {
        invalid = true;
      }

      return GetQnan<FT>();
    } else {
      if ((a_exp == MaxExponent<FT>() && (b_exp == 0 && b_mant == 0)) ||
          (b_exp == MaxExponent<FT>() && (a_exp == 0 && a_mant == 0)) ||
          ((a_exp == MaxExponent<FT>() || b_exp == MaxExponent<FT>()) &&
           (c_exp == MaxExponent<FT>() && r_sign != c_sign))) {
        invalid = true;
        return GetQnan<FT>();
      } else if (c_exp == MaxExponent<FT>()) {
        return FloatFrom3Tuple<FT>(c_sign, MaxExponent<FT>(), 0);
      } else {
        return FloatFrom3Tuple<FT>(r_sign, MaxExponent<FT>(), 0);
      }
    }
  }

  if (a_exp == 0) {
    if (a_mant == 0) {
      if (c_exp || c_mant)
        return c;

      if (c_sign != r_sign)
        r_sign = (rounding_mode == kRoundTowardNegative);
      return FloatFrom3Tuple<FT>(r_sign, 0, 0);
    }

    a_mant = NormalizeSubnormal<FT>(a_exp, a_mant);
  } else {
    a_mant |= static_cast<UT>(1) << NumSignificandBits<FT>();
  }

  if (b_exp == 0) {
    if (b_mant == 0) {
      if (c_exp || c_mant)
        return c;

      if (c_sign != r_sign)
        r_sign = (rounding_mode == kRoundTowardNegative);
      return FloatFrom3Tuple<FT>(r_sign, 0, 0);
    }

    b_mant = NormalizeSubnormal<FT>(b_exp, b_mant);
  } else {
    b_mant |= static_cast<UT>(1) << NumSignificandBits<FT>();
  }

  i32 r_exp = a_exp + b_exp - (1 << (NumExponentBits<FT>() - 1)) + 3;
  auto [r_mant0, r_mant1] = Umul<UT>(a_mant << NumRoundBits<FT>(), b_mant << NumRoundBits<FT>());

  if (r_mant1 < (1ull << (NumBits<FT>() - 3))) {
    r_mant1 = (r_mant1 << 1) | (r_mant0 >> (NumBits<FT>() - 1));
    r_mant0 <<= 1;
    r_exp--;
  }

  if (c_exp == 0) {
    if (c_mant == 0) {
      r_mant1 |= (r_mant0 != 0);
      return Normalize<FT>(r_sign, r_exp, r_mant1);
    }
    c_mant = NormalizeSubnormal<FT>(c_exp, c_mant);
  } else {
    c_mant |= static_cast<UT>(1) << NumSignificandBits<FT>();
  }

  c_exp++;

  UT c_mant1 = c_mant << (NumRoundBits<FT>() - 1);
  UT c_mant0 = 0;

  if (!(r_exp > c_exp || (r_exp == c_exp && r_mant1 >= c_mant1))) {
    UT tmp = r_mant1;
    r_mant1 = c_mant1;
    c_mant1 = tmp;
    tmp = r_mant0;
    r_mant0 = c_mant0;
    c_mant0 = tmp;
    i32 c_tmp = r_exp;
    r_exp = c_exp;
    c_exp = c_tmp;
    c_tmp = r_sign;
    r_sign = c_sign;
    c_sign = c_tmp;
  }

  i32 shift = r_exp - c_exp;
  if (shift >= 2 * (i32)NumBits<FT>()) {
    c_mant0 = (c_mant0 | c_mant1) != 0;
    c_mant1 = 0;
  } else if (shift >= (i32)NumBits<FT>() + 1) {
    c_mant0 = RshiftRnd<UT>(c_mant1, shift - NumBits<FT>());
    c_mant1 = 0;
  } else if (shift == NumBits<FT>()) {
    c_mant0 = c_mant1 | (c_mant0 != 0);
    c_mant1 = 0;
  } else if (shift != 0) {
    UT mask = (1ull << shift) - 1;
    c_mant0 = (c_mant1 << (NumBits<FT>() - shift)) | (c_mant0 >> shift) | ((c_mant0 & mask) != 0);
    c_mant1 = c_mant1 >> shift;
  }

  if (r_sign == c_sign) {
    r_mant0 += c_mant0;
    r_mant1 += c_mant1 + (r_mant0 < c_mant0);
  } else {
    UT tmp = r_mant0;
    r_mant0 -= c_mant0;
    r_mant1 = r_mant1 - c_mant1 - (r_mant0 > tmp);
    if ((r_mant0 | r_mant1) == 0) {
      r_sign = (rounding_mode == kRoundTowardNegative);
    }
  }

  return Normalize<FT>(r_sign, r_exp, r_mant1, r_mant0);
}

template f16 SoftFloat::Fma<f16>(f16 a, f16 b, f16 c);
template f32 SoftFloat::Fma<f32>(f32 a, f32 b, f32 c);
template f64 SoftFloat::Fma<f64>(f64 a, f64 b, f64 c);

f16 SoftFloat::I32ToF16(i32 a) {
  return IToF<i32, f16>(a);
}

f32 SoftFloat::I32ToF32(i32 a) {
  return IToF<i32, f32>(a);
}

f64 SoftFloat::I32ToF64(i32 a) {
  return IToF<i32, f64>(a);
}

f16 SoftFloat::U32ToF16(u32 a) {
  return IToF<u32, f16>(a);
}

f32 SoftFloat::U32ToF32(u32 a) {
  return IToF<u32, f32>(a);
}

f64 SoftFloat::U32ToF64(u32 a) {
  return IToF<u32, f64>(a);
}

f16 SoftFloat::I64ToF16(i64 a) {
  return IToF<i64, f16>(a);
}

f32 SoftFloat::I64ToF32(i64 a) {
  return IToF<i64, f32>(a);
}

f64 SoftFloat::I64ToF64(i64 a) {
  return IToF<i64, f64>(a);
}

f16 SoftFloat::U64ToF16(u64 a) {
  return IToF<u64, f16>(a);
}

f32 SoftFloat::U64ToF32(u64 a) {
  return IToF<u64, f32>(a);
}

f64 SoftFloat::U64ToF64(u64 a) {
  return IToF<u64, f64>(a);
}

template <typename TFROM, typename TTO>
TTO SoftFloat::FToF(TFROM a) {
  static_assert(std::is_floating_point_v<TFROM>);
  static_assert(std::is_floating_point_v<TTO>);
  static_assert(NumBits<TFROM>() > NumBits<TTO>());
  using UTFROM = FloatToUint<TFROM>::type;
  using UTTO = FloatToUint<TTO>::type;

  UTFROM a_mant = GetSignificand(a);
  i32 a_exp = GetExponent(a);
  bool a_sign = std::signbit(a);

  if (a_exp == MaxExponent<TFROM>()) {
    if (a_mant != 0) {
      if (IsSnan(a))
        invalid = true;
      return GetQnan<TTO>();
    }

    return FloatFrom3Tuple<TTO>(a_sign, MaxExponent<TTO>(), 0);
  }

  if (a_exp == 0) {
    if (a_mant == 0)
      return FloatFrom3Tuple<TTO>(a_sign, 0, 0);
    NormalizeSubnormal<TTO>(a_exp, a_mant);
  } else {
    a_mant |= static_cast<UTFROM>(1) << NumSignificandBits<TFROM>();
  }

  a_exp = a_exp - Bias<TFROM>() + Bias<TTO>();
  a_mant = RshiftRnd<UTFROM>(a_mant, NumSignificandBits<TFROM>() - (NumBits<TTO>() - 2));
  return Normalize<TTO>(a_sign, a_exp, static_cast<UTTO>(a_mant));
}

template f16 SoftFloat::FToF<f32, f16>(f32 a);
template f16 SoftFloat::FToF<f64, f16>(f64 a);
template f32 SoftFloat::FToF<f64, f32>(f64 a);

template <typename TFROM, typename TTO>
TTO SoftFloat::FToI(TFROM a) {
  static_assert(std::is_floating_point_v<TFROM>);
  static_assert(std::is_integral_v<TTO>);
  using UTFROM = FloatToUint<TFROM>::type;
  using UTTO = typename std::make_unsigned<TTO>::type;

  bool a_sign = std::signbit(a);
  i32 a_exp = GetExponent(a);
  UTFROM a_mant = GetSignificand(a);

  if (IsNan(a)) {
    invalid = true;
    return NanLimit<TTO>();
  }

  if (IsInf(a)) {
    invalid = true;
    return a_sign ? MinLimit<TTO>() : MaxLimit<TTO>();
  }

  if (a_exp == 0)
    a_exp = 1;
  else
    a_mant |= (UTFROM)1 << NumSignificandBits<TFROM>();

  a_mant <<= NumRoundBits<TFROM>();
  a_exp = a_exp - Bias<TFROM>() - NumSignificandBits<TFROM>();

  UTTO r, r_max;
  if (!std::is_signed_v<TTO>)
    r_max = (UTTO)a_sign - 1;
  else
    r_max = ((UTTO)1 << (NumBits<TTO>() - 1)) - (UTTO)(a_sign ^ 1);

  if (a_exp >= 0) {
    if (a_exp > (i32)(NumBits<TTO>() - 1 - NumSignificandBits<TFROM>())) {
      invalid = true;
      return a_sign ? MinLimit<TTO>() : MaxLimit<TTO>();
    }

    r = (UTTO)(a_mant >> NumRoundBits<TFROM>()) << a_exp;
    if (r > r_max) {
      invalid = true;
      return a_sign ? MinLimit<TTO>() : MaxLimit<TTO>();
    }

  } else {
    u32 addend = 0;
    a_mant = RshiftRnd<UTFROM>(a_mant, -a_exp);

    switch (rounding_mode) {
      case kRoundTiesToEven:
        [[fallthrough]];
      case kRoundTiesToAway:
        addend = 1 << (NumRoundBits<TFROM>() - 1);
        break;
      case kRoundTowardZero:
        addend = 0;
        break;
      case kRoundTowardNegative:
        addend = a_sign ? (1 << NumRoundBits<TFROM>()) - 1 : 0;
        break;
      case kRoundTowardPositive:
        addend = !a_sign ? (1 << NumRoundBits<TFROM>()) - 1 : 0;
        break;
      default:
        break;
    }

    auto rnd_bits = a_mant & ((1 << NumRoundBits<TFROM>()) - 1);
    a_mant = (a_mant + addend) >> NumRoundBits<TFROM>();

    if (rounding_mode == kRoundTiesToEven && rnd_bits == 1 << (NumRoundBits<TFROM>() - 1))
      a_mant &= ~1;

    if (a_mant > r_max) {
      invalid = true;
      return a_sign ? MinLimit<TTO>() : MaxLimit<TTO>();
    }

    r = a_mant;
    if (rnd_bits)
      inexact = true;
  }

  if (a_sign)
    r = -r;

  return r;
}

template i32 SoftFloat::FToI<f16, i32>(f16 a);
template i64 SoftFloat::FToI<f16, i64>(f16 a);
template u32 SoftFloat::FToI<f16, u32>(f16 a);
template u64 SoftFloat::FToI<f16, u64>(f16 a);
template i32 SoftFloat::FToI<f32, i32>(f32 a);
template i64 SoftFloat::FToI<f32, i64>(f32 a);
template u32 SoftFloat::FToI<f32, u32>(f32 a);
template u64 SoftFloat::FToI<f32, u64>(f32 a);
template i32 SoftFloat::FToI<f64, i32>(f64 a);
template i64 SoftFloat::FToI<f64, i64>(f64 a);
template u32 SoftFloat::FToI<f64, u32>(f64 a);
template u64 SoftFloat::FToI<f64, u64>(f64 a);

template <typename TFROM, typename TTO>
TTO SoftFloat::IToF(TFROM a) {
  using UTTO = FloatToUint<TTO>::type;
  typedef typename std::make_unsigned<TFROM>::type UT;

  bool a_sign;
  i32 a_exp;
  UT a_mant;
  UT r, mask;
  int l;

  if (std::is_signed<TFROM>::value && a < 0) {
    a_sign = 1;
    r = -(UT)a;
  } else {
    a_sign = 0;
    r = a;
  }
  a_exp = Bias<TTO>() + NumBits<TTO>() - 2;

  l = NumBits<TFROM>() - std::countl_zero(r) - (NumBits<TTO>() - 1);
  if (l > 0) {
    mask = r & (((UT)1 << l) - 1);
    r = (r >> l) | ((r & mask) != 0);
    a_exp += l;
  }
  a_mant = r;
  return Normalize<TTO>(a_sign, a_exp, static_cast<UTTO>(a_mant));
}

template f16 SoftFloat::IToF<i32, f16>(i32 a);
template f32 SoftFloat::IToF<i32, f32>(i32 a);
template f64 SoftFloat::IToF<i32, f64>(i32 a);

i32 SoftFloat::F16ToI32(f16 a) {
  return FToI<f16, i32>(a);
}

i64 SoftFloat::F16ToI64(f16 a) {
  return FToI<f16, i64>(a);
}

u32 SoftFloat::F16ToU32(f16 a) {
  return FToI<f16, u32>(a);
}

u64 SoftFloat::F16ToU64(f16 a) {
  return FToI<f16, u64>(a);
}

i32 SoftFloat::F32ToI32(f32 a) {
  return FToI<f32, i32>(a);
}

i64 SoftFloat::F32ToI64(f32 a) {
  return FToI<f32, i64>(a);
}

u32 SoftFloat::F32ToU32(f32 a) {
  return FToI<f32, u32>(a);
}

u64 SoftFloat::F32ToU64(f32 a) {
  return FToI<f32, u64>(a);
}

i32 SoftFloat::F64ToI32(f64 a) {
  return FToI<f64, i32>(a);
}

i64 SoftFloat::F64ToI64(f64 a) {
  return FToI<f64, i64>(a);
}

u32 SoftFloat::F64ToU32(f64 a) {
  return FToI<f64, u32>(a);
}

u64 SoftFloat::F64ToU64(f64 a) {
  return FToI<f64, u64>(a);
}

f16 SoftFloat::F32ToF16(f32 a) {
  return FToF<f32, f16>(a);
}

f16 SoftFloat::F64ToF16(f64 a) {
  return FToF<f64, f16>(a);
}

f32 SoftFloat::F64ToF32(f64 a) {
  return FToF<f64, f32>(a);
}
#pragma once

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdfloat>
#include <type_traits>

#define FLOPPY_FLOAT_FUNC_1(result, rounding_mode, func, ...)       \
  switch (rounding_mode) {                                          \
  case Vfpu::kRoundTiesToEven:                                      \
    result = func<Vfpu::kRoundTiesToEven>(__VA_ARGS__);             \
    break;                                                          \
  case Vfpu::kRoundTiesToAway:                                      \
    result = func<Vfpu::kRoundTiesToAway>(__VA_ARGS__);             \
    break;                                                          \
  case Vfpu::kRoundTowardPositive:                                  \
    result = func<Vfpu::kRoundTowardPositive>(__VA_ARGS__);         \
    break;                                                          \
  case Vfpu::kRoundTowardNegative:                                  \
    result = func<Vfpu::kRoundTowardNegative>(__VA_ARGS__);         \
    break;                                                          \
  case Vfpu::kRoundTowardZero:                                      \
    result = func<Vfpu::kRoundTowardZero>(__VA_ARGS__);             \
    break;                                                          \
  default:                                                          \
    throw std::runtime_error(std::string("Unknown rounding mode")); \
  }

#define FLOPPY_FLOAT_FUNC_2(result, rounding_mode, func, ftype, ...) \
  switch (rounding_mode) {                                           \
  case Vfpu::kRoundTiesToEven:                                       \
    result = func<ftype, Vfpu::kRoundTiesToEven>(__VA_ARGS__);       \
    break;                                                           \
  case Vfpu::kRoundTiesToAway:                                       \
    result = func<ftype, Vfpu::kRoundTiesToAway>(__VA_ARGS__);       \
    break;                                                           \
  case Vfpu::kRoundTowardPositive:                                   \
    result = func<ftype, Vfpu::kRoundTowardPositive>(__VA_ARGS__);   \
    break;                                                           \
  case Vfpu::kRoundTowardNegative:                                   \
    result = func<ftype, Vfpu::kRoundTowardNegative>(__VA_ARGS__);   \
    break;                                                           \
  case Vfpu::kRoundTowardZero:                                       \
    result = func<ftype, Vfpu::kRoundTowardZero>(__VA_ARGS__);       \
    break;                                                           \
  default:                                                           \
    throw std::runtime_error(std::string("Unknown rounding mode"));  \
  }

namespace FfUtils {

using f16 = std::float16_t;
using f32 = std::float32_t;
using f64 = std::float64_t;
using f128 = std::float128_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using i128 = __int128_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using u128 = __uint128_t;

template <typename T>
using nl = std::numeric_limits<T>;

template <typename T>
struct TwiceWidthType;

template <>
struct TwiceWidthType<f16> {
  using type = f32;
};

template <>
struct TwiceWidthType<f32> {
  using type = f64;
};

template <>
struct TwiceWidthType<f64> {
  using type = f128;
};

template <>
struct TwiceWidthType<u16> {
  using type = u32;
};

template <>
struct TwiceWidthType<u32> {
  using type = u64;
};

template <>
struct TwiceWidthType<u64> {
  using type = u128;
};

template <typename T>
struct FloatToUint;

template <>
struct FloatToUint<f16> {
  using type = u16;
};
template <>
struct FloatToUint<f32> {
  using type = u32;
};
template <>
struct FloatToUint<f64> {
  using type = u64;
};

template <typename T>
struct FloatToInt;

template <>
struct FloatToInt<f16> {
  using type = i16;
};
template <>
struct FloatToInt<f32> {
  using type = i32;
};
template <>
struct FloatToInt<f64> {
  using type = i64;
};

template <typename T>
struct UintToFloat;

template <>
struct UintToFloat<u16> {
  using type = f16;
};
template <>
struct UintToFloat<u32> {
  using type = f32;
};
template <>
struct UintToFloat<f64> {
  using type = f64;
};

template <typename FT>
struct QuietBit;

template <>
struct QuietBit<f16> {
  static constexpr u16 u = 0x0200u;
};

template <>
struct QuietBit<f32> {
  static constexpr u32 u = 0x00400000u;
};

template <>
struct QuietBit<f64> {
  static constexpr u64 u = 0x0008000000000000ull;
};

template <typename FT>
constexpr int Bias() {
  static_assert(std::is_floating_point<FT>::value);
  if constexpr (std::is_same_v<FT, f16>) {
    return 15;
  } else if constexpr (std::is_same_v<FT, f32>) {
    return 127;
  } else if constexpr (std::is_same_v<FT, f64>) {
    return 1023;
  } else {
    static_assert(false, "Type needs to be f16, f32, or f64");
  }
}

template <typename T>
constexpr int NumBits() {
  if constexpr (std::is_same_v<T, f16> || std::is_same_v<T, u16> || std::is_same_v<T, i16>) {
    return 16;
  } else if constexpr (std::is_same_v<T, f32> || std::is_same_v<T, u32> || std::is_same_v<T, i32>) {
    return 32;
  } else if constexpr (std::is_same_v<T, f64> || std::is_same_v<T, u64> || std::is_same_v<T, i64>) {
    return 64;
  } else if constexpr (std::is_same_v<T, f128> || std::is_same_v<T, u128> || std::is_same_v<T, i128>) {
    return 128;
  } else {
    static_assert(false, "Unsupported data type");
  }
}

template <typename FT>
constexpr int NumSignificandBits() {
  static_assert(std::is_floating_point<FT>::value);
  if constexpr (std::is_same<FT, f16>::value) {
    return 10;
  } else if constexpr (std::is_same<FT, f32>::value) {
    return 23;
  } else if constexpr (std::is_same<FT, f64>::value) {
    return 52;
  } else {
    static_assert(false, "Type needs to be f16, f32, or f64");
  }
}

template <typename FT>
constexpr int NumImantBits() {
  static_assert(std::is_floating_point<FT>::value);
  if constexpr (std::is_same<FT, f16>::value) {
    return 14;
  } else if constexpr (std::is_same<FT, f32>::value) {
    return 30;
  } else if constexpr (std::is_same<FT, f64>::value) {
    return 62;
  } else {
    static_assert(false, "Type needs to be f16, f32, or f64");
  }
}

template <typename FT>
constexpr int NumExponentBits() {
  static_assert(std::is_floating_point<FT>::value);
  if constexpr (std::is_same<FT, f16>::value) {
    return 5;
  } else if constexpr (std::is_same<FT, f32>::value) {
    return 8;
  } else if constexpr (std::is_same<FT, f64>::value) {
    return 11;
  } else {
    static_assert(false, "Type needs to be f16, f32, or f64");
  }
}

template <typename FT>
constexpr i32 MaxExponent() {
  static_assert(std::is_floating_point<FT>::value);
  if constexpr (std::is_same<FT, f16>::value) {
    return 31;
  } else if constexpr (std::is_same<FT, f32>::value) {
    return 255;
  } else if constexpr (std::is_same<FT, f64>::value) {
    return 2047;
  } else {
    static_assert(false, "Type needs to be f16, f32, or f64");
  }
}

template <typename FT>
constexpr int NumRoundBits() {
  static_assert(std::is_floating_point<FT>::value);
  return NumImantBits<FT>() - NumSignificandBits<FT>();
}

template <typename FT>
constexpr u64 RoundMask() {
  static_assert(std::is_floating_point<FT>::value);
  return (1ull << NumRoundBits<FT>()) - 1ull;
}

template <typename FT>
constexpr FT ClearSignificand(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  auto u = std::bit_cast<typename FloatToUint<FT>::type>(a);
  if constexpr (std::is_same_v<FT, f16>) {
    u &= 0xfc00u;
  } else if constexpr (std::is_same_v<FT, f32>) {
    u &= 0xff800000u;
  } else if constexpr (std::is_same_v<FT, f64>) {
    u &= 0xfff0000000000000ull;
  } else {
    static_assert(false, "Type needs to be f16, f32, or f64");
  }
  return std::bit_cast<FT>(u);
}

template <typename FT>
constexpr FT CreateQnanWithPayload(typename FloatToUint<FT>::type payload) {
  using UT = decltype(payload);
  UT u;
  if constexpr (std::is_same<FT, f16>::value) {
    u = 0x7e00u;
  } else if constexpr (std::is_same<FT, f32>::value) {
    u = 0x7fc00000u;
  } else if constexpr (std::is_same<FT, f64>::value) {
    u = 0x7ff8000000000000ull;
  } else {
    static_assert(false, "Type needs to be f16, f32, or f64");
  }
  return std::bit_cast<FT>((UT)(u | payload));
}

template <typename FT>
constexpr auto FloatFrom3Tuple(bool sign, u32 exponent, u64 significand) {
  static_assert(std::is_floating_point<FT>::value);
  using UT = typename FloatToUint<FT>::type;
  UT u = 0;
  if constexpr (std::is_same<FT, f16>::value) {
    u |= static_cast<UT>(sign) << 15;
    u |= static_cast<UT>(exponent) << NumSignificandBits<FT>();
    u |= static_cast<UT>(significand) & 0x3ffu;
  } else if constexpr (std::is_same<FT, f32>::value) {
    u |= static_cast<UT>(sign) << 31;
    u |= static_cast<UT>(exponent) << NumSignificandBits<FT>();
    u |= static_cast<UT>(significand) & 0x007fffffu;
  } else if constexpr (std::is_same<FT, f64>::value) {
    u |= static_cast<UT>(sign) << 63;
    u |= static_cast<UT>(exponent) << NumSignificandBits<FT>();
    u |= static_cast<UT>(significand) & 0xfffffffffffffull;
  } else {
    static_assert(false, "Type needs to be f16, f32, or f64");
  }
  return std::bit_cast<FT>(u);
}

template <typename FT>
constexpr auto GetSignificand(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  using UT = typename FloatToUint<FT>::type;
  UT u = std::bit_cast<typename FloatToUint<FT>::type>(a);
  if constexpr (std::is_same_v<FT, f16>) {
    u &= 0x3ffu;
  } else if constexpr (std::is_same_v<FT, f32>) {
    u &= 0x007fffffu;
  } else if constexpr (std::is_same_v<FT, f64>) {
    u &= 0xfffffffffffffull;
  } else {
    static_assert(false, "Type needs to be f16, f32, or f64");
  }
  return u;
}

template <typename FT>
constexpr auto GetPayload(FT a) {
  using UT = FloatToUint<FT>::type;
  UT u;
  if constexpr (std::is_same_v<FT, f16>) {
    u = std::bit_cast<UT>(a) & 0x1ffu;
  } else if constexpr (std::is_same_v<FT, f32>) {
    u = std::bit_cast<UT>(a) & 0x3fffffu;
  } else if constexpr (std::is_same_v<FT, f64>) {
    u = std::bit_cast<UT>(a) & 0xfffffffffffffull;
  } else {
    static_assert(false, "Type needs to be f16, f32, or f64");
  }
  return u;
}

template <typename FT>
constexpr bool GetQuietBit(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return QuietBit<FT>::u & std::bit_cast<typename FloatToUint<FT>::type>(a);
}

template <typename FT>
constexpr auto GetExponent(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  using UT = typename FloatToUint<FT>::type;
  UT u = std::bit_cast<typename FloatToUint<FT>::type>(a);
  if constexpr (std::is_same_v<FT, f16>) {
    u = (u >> NumSignificandBits<FT>()) & 0x1fu;
  } else if constexpr (std::is_same_v<FT, f32>) {
    u = (u >> NumSignificandBits<FT>()) & 0xffu;
  } else if constexpr (std::is_same_v<FT, f64>) {
    u = (u >> NumSignificandBits<FT>()) & 0x7ffull;
  } else {
    static_assert(false, "Type needs to be f16, f32, or f64");
  }
  return u;
}

template <typename FT>
constexpr bool IsInf(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  FT res = a - a;
  return (a == a) && (res != res);
}

template <typename FT>
constexpr bool IsInfOrNan(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  FT res = a - a;
  return res != res;
}

// template <typename FT>
// constexpr bool IsInfOrNan(FT a) {
//   static_assert(std::is_floating_point<FT>::value);
//   return GetExponent<FT>(a) == MaxExponent<FT>();
// }

template <typename FT>
constexpr bool IsNan(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return a != a;
}

template <typename FT>
constexpr bool IsNeg(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return std::signbit(a);
}

template <typename FT>
constexpr bool IsNegInf(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return a == -nl<FT>::infinity();
}

template <typename FT>
constexpr bool IsPos(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return !std::signbit(a);
}

template <typename FT>
constexpr bool IsPosInf(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return a == nl<FT>::infinity();
}

template <typename FT>
constexpr bool IsPosZero(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return 0 == std::bit_cast<typename FloatToUint<FT>::type>(a);
}

template <typename FT>
constexpr bool IsQnan(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return IsNan(a) && (std::bit_cast<typename FloatToUint<FT>::type>(a) & QuietBit<FT>::u);
}

template <typename FT>
constexpr bool IsSnan(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return IsNan(a) && !(std::bit_cast<typename FloatToUint<FT>::type>(a) & QuietBit<FT>::u);
}

template <typename FT>
constexpr bool IsTiny(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return std::abs(a) < nl<FT>::min();
}

template <typename FT>
constexpr bool MayResultFromUnderflow(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return std::abs(a) <= nl<FT>::min();
}

template <typename FT>
constexpr bool IsSubnormal(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return (std::abs(a) < nl<FT>::min()) && !IsZero(a);
}

template <typename FT>
constexpr bool IsZero(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return (a == -a);
}

template <typename FT>
constexpr FT NextDownNoPosZero(FT a) {
  auto au = std::bit_cast<typename FloatToInt<FT>::type>(a);
  au -= a > 0. ? 1 : -1;  // Nextdown of result cannot be +0.
  return std::bit_cast<FT>(au);
}

template <typename FT>
constexpr FT NextUpNoNegZero(FT a) {
  auto au = std::bit_cast<typename FloatToInt<FT>::type>(a);
  au += a >= 0. ? 1 : -1;  // Nextup of result cannot be -0.
  return std::bit_cast<FT>(au);
}

template <typename FT>
constexpr u64 MaxSignificand() {
  static_assert(std::is_floating_point<FT>::value);
  return (1ull << NumSignificandBits<FT>()) - 1ull;
}

template <typename FT>
constexpr FT SetQuietBit(FT a) {
  auto au = std::bit_cast<typename FloatToUint<FT>::type>(a);
  return std::bit_cast<FT>((decltype(au))(QuietBit<FT>::u | au));
}

template <typename FT>
constexpr auto ExponentMask() {
  static_assert(std::is_floating_point<FT>::value);
  using UT = typename FloatToUint<FT>::type;
  UT u;
  if constexpr (std::is_same_v<FT, f16>) {
    u = 0x7c00u;
  } else if constexpr (std::is_same_v<FT, f32>) {
    u = 0x7f800000u;
  } else if constexpr (std::is_same_v<FT, f64>) {
    u = 0x7ff0000000000000ull;
  } else {
    static_assert("Type needs to be f16, f32, or f64");
  }
  return u;
};

template <typename FT>
constexpr auto SignMask() {
  static_assert(std::is_floating_point<FT>::value);
  using UT = typename FloatToUint<FT>::type;
  UT u;
  if constexpr (std::is_same_v<FT, f16>) {
    u = 1u << 15;
  } else if constexpr (std::is_same_v<FT, f32>) {
    u = 1u << 31;
  } else if constexpr (std::is_same_v<FT, f64>) {
    u = 1ull << 63;
  } else {
    static_assert("Type needs to be f16, f32, or f64");
  }
  return u;
}

};  // namespace FfUtils
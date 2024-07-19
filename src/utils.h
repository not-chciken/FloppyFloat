#pragma once

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdfloat>
#include <type_traits>

using f16 = std::float16_t;
using f32 = std::float32_t;
using f64 = std::float64_t;
using f128 = std::float128_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

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
    static_assert("Type needs to be f16, f32, or f64");
  }
  return std::bit_cast<FT>((UT)(u | payload));
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
constexpr bool IsSubnormal(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return (std::abs(a) < nl<FT>::min()) && !IsZero(a);
}

template <typename FT>
constexpr bool IsZero(FT a) {
  static_assert(std::is_floating_point<FT>::value);
  return (a == -a);
}

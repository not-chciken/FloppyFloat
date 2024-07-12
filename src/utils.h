#pragma once

#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdfloat>

using f16 = std::float16_t;
using f32 = std::float32_t;
using f64 = std::float64_t;
using f128 = std::float128_t;

using u8  = uint8_t;
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

template<typename T>
struct FloatToUint;

template<> struct FloatToUint<f16> { using type = f16; };
template<> struct FloatToUint<f32> { using type = u32; };
template<> struct FloatToUint<f64> { using type = u64; };

template<typename FT>
struct QuietBit;

template<>
struct QuietBit<f32> {
  static constexpr u32 u = 0x00400000u;
};

template<>
struct QuietBit<f64> {
  static constexpr u64 u = 0x0008000000000000ull;
};


template <typename FT>
constexpr bool IsInf(FT a) {
    FT res = a - a;
    return (a == a) && (res != res);
}

template <typename FT>
constexpr bool IsInfOrNan(FT a) {
  FT res = a - a;
  return res != res;
}

template <typename FT>
constexpr bool IsNan(FT a) {
  return a != a;
}

template <typename FT>
constexpr bool IsNeg(FT a) {
    return std::signbit(a);
}

template <typename FT>
constexpr bool IsNegInf(FT a) {
    return a == -nl<FT>::infinity();
}

template <typename FT>
constexpr bool IsPos(FT a) {
    return !std::signbit(a);
}

template <typename FT>
constexpr bool IsPosInf(FT a) {
    return a == nl<FT>::infinity();
}

template <typename FT>
constexpr bool IsSnan(FT a) {
  return IsNan(a) && !(std::bit_cast<typename FloatToUint<FT>::type>(a) & QuietBit<FT>::u);
}

template <typename FT>
constexpr bool IsZero(FT a) {
  return (a == -a);
}

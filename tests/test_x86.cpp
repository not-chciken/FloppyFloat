/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 * These tests are supposed to run on an x86 host.
 ******************************************************************************/

#include <gtest/gtest.h>

#include <bit>
#include <cfenv>
#include <cmath>
#include <iostream>

#include "float_rng.h"
#include "floppy_float.h"
#include "utils.h"

//constexpr i32 kNumIterations = 20000;
constexpr i32 kNumIterations = 100;
constexpr i32 kRngSeed = 42;

TEST(x86Tests, Addf32) {
  FloppyFloat ff;
  ff.SetupTox86();

  for (auto rm : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    FloatRng<f32> float_rng(kRngSeed);
    f32 a{float_rng.Gen()};
    f32 b{float_rng.Gen()};
    for (i32 i = 0; i < kNumIterations; ++i) {
      f32 ff_result;

      ff.ClearFlags();
      switch (rm) {
      case FE_TONEAREST:
        ff_result = ff.Add<f32, FloppyFloat::kRoundTiesToEven>(a, b);
        break;
      case FE_TOWARDZERO:
        ff_result = ff.Add<f32, FloppyFloat::kRoundTowardZero>(a, b);
        break;
      case FE_DOWNWARD:
        ff_result = ff.Add<f32, FloppyFloat::kRoundTowardNegative>(a, b);
        break;
      case FE_UPWARD:
        ff_result = ff.Add<f32, FloppyFloat::kRoundTowardPositive>(a, b);
        break;
      default:
        break;
      }

      std::feclearexcept(FE_ALL_EXCEPT);
      fesetround(rm);
      f32 x86_result;
      asm volatile(
          "movss %1, %%xmm0;"
          "addss %2, %%xmm0;"
          "movss %%xmm0, %0;"
          : "=m"(x86_result)
          : "m"(a), "m"(b)
          : "%xmm0");
      fesetround(FE_TONEAREST);

      ASSERT_EQ(std::bit_cast<u32>(x86_result), std::bit_cast<u32>(ff_result));
      ASSERT_EQ(ff.invalid, static_cast<bool>(std::fetestexcept(FE_INVALID)));
      ASSERT_EQ(ff.division_by_zero, static_cast<bool>(std::fetestexcept(FE_DIVBYZERO)));
      ASSERT_EQ(ff.overflow, static_cast<bool>(std::fetestexcept(FE_OVERFLOW)));
      ASSERT_EQ(ff.underflow, static_cast<bool>(std::fetestexcept(FE_UNDERFLOW)));
      ASSERT_EQ(ff.inexact, static_cast<bool>(std::fetestexcept(FE_INEXACT)));

      b = a;
      a = float_rng.Gen();
    }
  }
}

TEST(x86Tests, Subf32) {
  FloppyFloat ff;
  ff.SetupTox86();

  for (auto rm : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    FloatRng<f32> float_rng(kRngSeed);
    f32 a{float_rng.Gen()};
    f32 b{float_rng.Gen()};
    for (i32 i = 0; i < kNumIterations; ++i) {
      f32 ff_result;

      ff.ClearFlags();
      switch (rm) {
      case FE_TONEAREST:
        ff_result = ff.Sub<f32, FloppyFloat::kRoundTiesToEven>(a, b);
        break;
      case FE_TOWARDZERO:
        ff_result = ff.Sub<f32, FloppyFloat::kRoundTowardZero>(a, b);
        break;
      case FE_DOWNWARD:
        ff_result = ff.Sub<f32, FloppyFloat::kRoundTowardNegative>(a, b);
        break;
      case FE_UPWARD:
        ff_result = ff.Sub<f32, FloppyFloat::kRoundTowardPositive>(a, b);
        break;
      default:
        break;
      }

      std::feclearexcept(FE_ALL_EXCEPT);
      fesetround(rm);
      f32 x86_result;
      asm volatile(
          "movss %1, %%xmm0;"
          "subss %2, %%xmm0;"
          "movss %%xmm0, %0;"
          : "=m"(x86_result)
          : "m"(a), "m"(b)
          : "%xmm0");
      fesetround(FE_TONEAREST);

      ASSERT_EQ(std::bit_cast<u32>(x86_result), std::bit_cast<u32>(ff_result));
      ASSERT_EQ(ff.invalid, static_cast<bool>(std::fetestexcept(FE_INVALID)));
      ASSERT_EQ(ff.division_by_zero, static_cast<bool>(std::fetestexcept(FE_DIVBYZERO)));
      ASSERT_EQ(ff.overflow, static_cast<bool>(std::fetestexcept(FE_OVERFLOW)));
      ASSERT_EQ(ff.underflow, static_cast<bool>(std::fetestexcept(FE_UNDERFLOW)));
      ASSERT_EQ(ff.inexact, static_cast<bool>(std::fetestexcept(FE_INEXACT)));

      b = a;
      a = float_rng.Gen();
    }
  }
}

TEST(x86Tests, Mul32) {
  FloppyFloat ff;
  ff.SetupTox86();

  for (auto rm : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    FloatRng<f32> float_rng(kRngSeed);
    f32 a{float_rng.Gen()};
    f32 b{float_rng.Gen()};
    for (i32 i = 0; i < kNumIterations; ++i) {
      f32 ff_result;

      ff.ClearFlags();
      switch (rm) {
      case FE_TONEAREST:
        ff_result = ff.Mul<f32, FloppyFloat::kRoundTiesToEven>(a, b);
        break;
      case FE_TOWARDZERO:
        ff_result = ff.Mul<f32, FloppyFloat::kRoundTowardZero>(a, b);
        break;
      case FE_DOWNWARD:
        ff_result = ff.Mul<f32, FloppyFloat::kRoundTowardNegative>(a, b);
        break;
      case FE_UPWARD:
        ff_result = ff.Mul<f32, FloppyFloat::kRoundTowardPositive>(a, b);
        break;
      default:
        break;
      }

      std::feclearexcept(FE_ALL_EXCEPT);
      fesetround(rm);
      f32 x86_result;
      asm volatile(
          "movss %1, %%xmm0;"
          "mulss %2, %%xmm0;"
          "movss %%xmm0, %0;"
          : "=m"(x86_result)
          : "m"(a), "m"(b)
          : "%xmm0");
      fesetround(FE_TONEAREST);

      ASSERT_EQ(std::bit_cast<u32>(x86_result), std::bit_cast<u32>(ff_result));
      ASSERT_EQ(ff.invalid, static_cast<bool>(std::fetestexcept(FE_INVALID)));
      ASSERT_EQ(ff.division_by_zero, static_cast<bool>(std::fetestexcept(FE_DIVBYZERO)));
      ASSERT_EQ(ff.overflow, static_cast<bool>(std::fetestexcept(FE_OVERFLOW)));
      ASSERT_EQ(ff.underflow, static_cast<bool>(std::fetestexcept(FE_UNDERFLOW)));
      ASSERT_EQ(ff.inexact, static_cast<bool>(std::fetestexcept(FE_INEXACT)));

      b = a;
      a = float_rng.Gen();
    }
  }
}

TEST(x86Tests, Div32) {
  FloppyFloat ff;
  ff.SetupTox86();

  for (auto rm : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    FloatRng<f32> float_rng(kRngSeed);
    f32 a{float_rng.Gen()};
    f32 b{float_rng.Gen()};
    for (i32 i = 0; i < kNumIterations; ++i) {
      f32 ff_result;

      ff.ClearFlags();
      switch (rm) {
      case FE_TONEAREST:
        ff_result = ff.Div<f32, FloppyFloat::kRoundTiesToEven>(a, b);
        break;
      case FE_TOWARDZERO:
        ff_result = ff.Div<f32, FloppyFloat::kRoundTowardZero>(a, b);
        break;
      case FE_DOWNWARD:
        ff_result = ff.Div<f32, FloppyFloat::kRoundTowardNegative>(a, b);
        break;
      case FE_UPWARD:
        ff_result = ff.Div<f32, FloppyFloat::kRoundTowardPositive>(a, b);
        break;
      default:
        break;
      }

      std::feclearexcept(FE_ALL_EXCEPT);
      fesetround(rm);
      f32 x86_result;
      asm volatile(
          "movss %1, %%xmm0;"
          "divss %2, %%xmm0;"
          "movss %%xmm0, %0;"
          : "=m"(x86_result)
          : "m"(a), "m"(b)
          : "%xmm0");
      fesetround(FE_TONEAREST);

      ASSERT_EQ(std::bit_cast<u32>(x86_result), std::bit_cast<u32>(ff_result));
      ASSERT_EQ(ff.invalid, static_cast<bool>(std::fetestexcept(FE_INVALID)));
      ASSERT_EQ(ff.division_by_zero, static_cast<bool>(std::fetestexcept(FE_DIVBYZERO)));
      ASSERT_EQ(ff.overflow, static_cast<bool>(std::fetestexcept(FE_OVERFLOW)));
      ASSERT_EQ(ff.underflow, static_cast<bool>(std::fetestexcept(FE_UNDERFLOW)));
      ASSERT_EQ(ff.inexact, static_cast<bool>(std::fetestexcept(FE_INEXACT)));

      b = a;
      a = float_rng.Gen();
    }
  }
}

TEST(x86Tests, Sqrt32) {
  FloppyFloat ff;
  ff.SetupTox86();

  for (auto rm : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    FloatRng<f32> float_rng(kRngSeed);
    f32 a{float_rng.Gen()};
    for (i32 i = 0; i < kNumIterations; ++i) {
      f32 ff_result;

      ff.ClearFlags();
      switch (rm) {
      case FE_TONEAREST:
        ff_result = ff.Sqrt<f32, FloppyFloat::kRoundTiesToEven>(a);
        break;
      case FE_TOWARDZERO:
        ff_result = ff.Sqrt<f32, FloppyFloat::kRoundTowardZero>(a);
        break;
      case FE_DOWNWARD:
        ff_result = ff.Sqrt<f32, FloppyFloat::kRoundTowardNegative>(a);
        break;
      case FE_UPWARD:
        ff_result = ff.Sqrt<f32, FloppyFloat::kRoundTowardPositive>(a);
        break;
      default:
        break;
      }

      std::feclearexcept(FE_ALL_EXCEPT);
      fesetround(rm);
      f32 x86_result;
      asm volatile(
          "movss %1, %%xmm0;"
          "sqrtss %2, %%xmm0;"
          "movss %%xmm0, %0;"
          : "=m"(x86_result)
          : "m"(a), "m"(a)
          : "%xmm0");
      fesetround(FE_TONEAREST);

      ASSERT_EQ(std::bit_cast<u32>(x86_result), std::bit_cast<u32>(ff_result));
      ASSERT_EQ(ff.invalid, static_cast<bool>(std::fetestexcept(FE_INVALID)));
      ASSERT_EQ(ff.division_by_zero, static_cast<bool>(std::fetestexcept(FE_DIVBYZERO)));
      ASSERT_EQ(ff.overflow, static_cast<bool>(std::fetestexcept(FE_OVERFLOW)));
      ASSERT_EQ(ff.underflow, static_cast<bool>(std::fetestexcept(FE_UNDERFLOW)));
      ASSERT_EQ(ff.inexact, static_cast<bool>(std::fetestexcept(FE_INEXACT)));

      a = float_rng.Gen();
    }
  }
}

TEST(x86Tests, Addf64) {
  FloppyFloat ff;
  ff.SetupTox86();

  for (auto rm : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    FloatRng<f64> float_rng(kRngSeed);
    f64 a{float_rng.Gen()};
    f64 b{float_rng.Gen()};
    for (i32 i = 0; i < kNumIterations; ++i) {
      f64 ff_result;

      ff.ClearFlags();
      switch (rm) {
      case FE_TONEAREST:
        ff_result = ff.Add<f64, FloppyFloat::kRoundTiesToEven>(a, b);
        break;
      case FE_TOWARDZERO:
        ff_result = ff.Add<f64, FloppyFloat::kRoundTowardZero>(a, b);
        break;
      case FE_DOWNWARD:
        ff_result = ff.Add<f64, FloppyFloat::kRoundTowardNegative>(a, b);
        break;
      case FE_UPWARD:
        ff_result = ff.Add<f64, FloppyFloat::kRoundTowardPositive>(a, b);
        break;
      default:
        break;
      }

      std::feclearexcept(FE_ALL_EXCEPT);
      fesetround(rm);
      f64 x86_result;
      asm volatile(
          "movsd %1, %%xmm0;"
          "addsd %2, %%xmm0;"
          "movsd %%xmm0, %0;"
          : "=m"(x86_result)
          : "m"(a), "m"(b)
          : "%xmm0");
      fesetround(FE_TONEAREST);

      ASSERT_EQ(std::bit_cast<u64>(x86_result), std::bit_cast<u64>(ff_result));
      ASSERT_EQ(ff.invalid, static_cast<bool>(std::fetestexcept(FE_INVALID)));
      ASSERT_EQ(ff.division_by_zero, static_cast<bool>(std::fetestexcept(FE_DIVBYZERO)));
      ASSERT_EQ(ff.overflow, static_cast<bool>(std::fetestexcept(FE_OVERFLOW)));
      ASSERT_EQ(ff.underflow, static_cast<bool>(std::fetestexcept(FE_UNDERFLOW)));
      ASSERT_EQ(ff.inexact, static_cast<bool>(std::fetestexcept(FE_INEXACT)));

      b = a;
      a = float_rng.Gen();
    }
  }
}

TEST(x86Tests, Subf64) {
  FloppyFloat ff;
  ff.SetupTox86();

  for (auto rm : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    FloatRng<f64> float_rng(kRngSeed);
    f64 a{float_rng.Gen()};
    f64 b{float_rng.Gen()};
    for (i32 i = 0; i < kNumIterations; ++i) {
      f64 ff_result;

      ff.ClearFlags();
      switch (rm) {
      case FE_TONEAREST:
        ff_result = ff.Sub<f64, FloppyFloat::kRoundTiesToEven>(a, b);
        break;
      case FE_TOWARDZERO:
        ff_result = ff.Sub<f64, FloppyFloat::kRoundTowardZero>(a, b);
        break;
      case FE_DOWNWARD:
        ff_result = ff.Sub<f64, FloppyFloat::kRoundTowardNegative>(a, b);
        break;
      case FE_UPWARD:
        ff_result = ff.Sub<f64, FloppyFloat::kRoundTowardPositive>(a, b);
        break;
      default:
        break;
      }

      std::feclearexcept(FE_ALL_EXCEPT);
      fesetround(rm);
      f64 x86_result;
      asm volatile(
          "movsd %1, %%xmm0;"
          "subsd %2, %%xmm0;"
          "movsd %%xmm0, %0;"
          : "=m"(x86_result)
          : "m"(a), "m"(b)
          : "%xmm0");
      fesetround(FE_TONEAREST);

      ASSERT_EQ(std::bit_cast<u64>(x86_result), std::bit_cast<u64>(ff_result));
      ASSERT_EQ(ff.invalid, static_cast<bool>(std::fetestexcept(FE_INVALID)));
      ASSERT_EQ(ff.division_by_zero, static_cast<bool>(std::fetestexcept(FE_DIVBYZERO)));
      ASSERT_EQ(ff.overflow, static_cast<bool>(std::fetestexcept(FE_OVERFLOW)));
      ASSERT_EQ(ff.underflow, static_cast<bool>(std::fetestexcept(FE_UNDERFLOW)));
      ASSERT_EQ(ff.inexact, static_cast<bool>(std::fetestexcept(FE_INEXACT)));

      b = a;
      a = float_rng.Gen();
    }
  }
}

TEST(x86Tests, Mul64) {
  FloppyFloat ff;
  ff.SetupTox86();

  for (auto rm : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    FloatRng<f64> float_rng(kRngSeed);
    f64 a{float_rng.Gen()};
    f64 b{float_rng.Gen()};
    for (i32 i = 0; i < kNumIterations; ++i) {
      f64 ff_result;

      ff.ClearFlags();
      switch (rm) {
      case FE_TONEAREST:
        ff_result = ff.Mul<f64, FloppyFloat::kRoundTiesToEven>(a, b);
        break;
      case FE_TOWARDZERO:
        ff_result = ff.Mul<f64, FloppyFloat::kRoundTowardZero>(a, b);
        break;
      case FE_DOWNWARD:
        ff_result = ff.Mul<f64, FloppyFloat::kRoundTowardNegative>(a, b);
        break;
      case FE_UPWARD:
        ff_result = ff.Mul<f64, FloppyFloat::kRoundTowardPositive>(a, b);
        break;
      default:
        break;
      }

      std::feclearexcept(FE_ALL_EXCEPT);
      fesetround(rm);
      f64 x86_result;
      asm volatile(
          "movsd %1, %%xmm0;"
          "mulsd %2, %%xmm0;"
          "movsd %%xmm0, %0;"
          : "=m"(x86_result)
          : "m"(a), "m"(b)
          : "%xmm0");
      fesetround(FE_TONEAREST);

      ASSERT_EQ(std::bit_cast<u64>(x86_result), std::bit_cast<u64>(ff_result));
      ASSERT_EQ(ff.invalid, static_cast<bool>(std::fetestexcept(FE_INVALID)));
      ASSERT_EQ(ff.division_by_zero, static_cast<bool>(std::fetestexcept(FE_DIVBYZERO)));
      ASSERT_EQ(ff.overflow, static_cast<bool>(std::fetestexcept(FE_OVERFLOW)));
      ASSERT_EQ(ff.underflow, static_cast<bool>(std::fetestexcept(FE_UNDERFLOW)));
      ASSERT_EQ(ff.inexact, static_cast<bool>(std::fetestexcept(FE_INEXACT)));

      b = a;
      a = float_rng.Gen();
    }
  }
}

TEST(x86Tests, Div64) {
  FloppyFloat ff;
  ff.SetupTox86();

  for (auto rm : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    FloatRng<f64> float_rng(kRngSeed);
    f64 a{float_rng.Gen()};
    f64 b{float_rng.Gen()};
    for (i32 i = 0; i < kNumIterations; ++i) {
      f64 ff_result;

      ff.ClearFlags();
      switch (rm) {
      case FE_TONEAREST:
        ff_result = ff.Div<f64, FloppyFloat::kRoundTiesToEven>(a, b);
        break;
      case FE_TOWARDZERO:
        ff_result = ff.Div<f64, FloppyFloat::kRoundTowardZero>(a, b);
        break;
      case FE_DOWNWARD:
        ff_result = ff.Div<f64, FloppyFloat::kRoundTowardNegative>(a, b);
        break;
      case FE_UPWARD:
        ff_result = ff.Div<f64, FloppyFloat::kRoundTowardPositive>(a, b);
        break;
      default:
        break;
      }

      std::feclearexcept(FE_ALL_EXCEPT);
      fesetround(rm);
      f64 x86_result;
      asm volatile(
          "movsd %1, %%xmm0;"
          "divsd %2, %%xmm0;"
          "movsd %%xmm0, %0;"
          : "=m"(x86_result)
          : "m"(a), "m"(b)
          : "%xmm0");
      fesetround(FE_TONEAREST);

      ASSERT_EQ(std::bit_cast<u64>(x86_result), std::bit_cast<u64>(ff_result));
      ASSERT_EQ(ff.invalid, static_cast<bool>(std::fetestexcept(FE_INVALID)));
      ASSERT_EQ(ff.division_by_zero, static_cast<bool>(std::fetestexcept(FE_DIVBYZERO)));
      ASSERT_EQ(ff.overflow, static_cast<bool>(std::fetestexcept(FE_OVERFLOW)));
      ASSERT_EQ(ff.underflow, static_cast<bool>(std::fetestexcept(FE_UNDERFLOW)));
      ASSERT_EQ(ff.inexact, static_cast<bool>(std::fetestexcept(FE_INEXACT)));

      b = a;
      a = float_rng.Gen();
    }
  }
}

TEST(x86Tests, Sqrt64) {
  FloppyFloat ff;
  ff.SetupTox86();

  for (auto rm : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    FloatRng<f64> float_rng(kRngSeed);
    f64 a{float_rng.Gen()};
    for (i32 i = 0; i < kNumIterations; ++i) {
      f64 ff_result;

      ff.ClearFlags();
      switch (rm) {
      case FE_TONEAREST:
        ff_result = ff.Sqrt<f64, FloppyFloat::kRoundTiesToEven>(a);
        break;
      case FE_TOWARDZERO:
        ff_result = ff.Sqrt<f64, FloppyFloat::kRoundTowardZero>(a);
        break;
      case FE_DOWNWARD:
        ff_result = ff.Sqrt<f64, FloppyFloat::kRoundTowardNegative>(a);
        break;
      case FE_UPWARD:
        ff_result = ff.Sqrt<f64, FloppyFloat::kRoundTowardPositive>(a);
        break;
      default:
        break;
      }

      std::feclearexcept(FE_ALL_EXCEPT);
      fesetround(rm);
      f64 x86_result;
      asm volatile(
          "movsd %1, %%xmm0;"
          "sqrtsd %2, %%xmm0;"
          "movsd %%xmm0, %0;"
          : "=m"(x86_result)
          : "m"(a), "m"(a)
          : "%xmm0");
      fesetround(FE_TONEAREST);

      ASSERT_EQ(std::bit_cast<u64>(x86_result), std::bit_cast<u64>(ff_result));
      ASSERT_EQ(ff.invalid, static_cast<bool>(std::fetestexcept(FE_INVALID)));
      ASSERT_EQ(ff.division_by_zero, static_cast<bool>(std::fetestexcept(FE_DIVBYZERO)));
      ASSERT_EQ(ff.overflow, static_cast<bool>(std::fetestexcept(FE_OVERFLOW)));
      ASSERT_EQ(ff.underflow, static_cast<bool>(std::fetestexcept(FE_UNDERFLOW)));
      ASSERT_EQ(ff.inexact, static_cast<bool>(std::fetestexcept(FE_INEXACT)));

      a = float_rng.Gen();
    }
  }
}

TEST(x86Tests, Max64) {
  FloppyFloat ff;
  ff.SetupTox86();

  for (auto rm : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    FloatRng<f64> float_rng(kRngSeed);
    f64 a{float_rng.Gen()};
    f64 b{float_rng.Gen()};
    for (i32 i = 0; i < kNumIterations; ++i) {
      f64 ff_result;

      ff.ClearFlags();
      switch (rm) {
      case FE_TONEAREST:
        ff_result = ff.Maxx86<f64>(a, b);
        break;
      case FE_TOWARDZERO:
        ff_result = ff.Maxx86<f64>(a, b);
        break;
      case FE_DOWNWARD:
        ff_result = ff.Maxx86<f64>(a, b);
        break;
      case FE_UPWARD:
        ff_result = ff.Maxx86<f64>(a, b);
        break;
      default:
        break;
      }

      std::feclearexcept(FE_ALL_EXCEPT);
      fesetround(rm);
      f64 x86_result;
      asm volatile(
          "movsd %1, %%xmm0;"
          "maxsd %2, %%xmm0;"
          "movsd %%xmm0, %0;"
          : "=m"(x86_result)
          : "m"(a), "m"(b)
          : "%xmm0");
      fesetround(FE_TONEAREST);

      ASSERT_EQ(std::bit_cast<u64>(x86_result), std::bit_cast<u64>(ff_result));
      ASSERT_EQ(ff.invalid, static_cast<bool>(std::fetestexcept(FE_INVALID)));
      ASSERT_EQ(ff.division_by_zero, static_cast<bool>(std::fetestexcept(FE_DIVBYZERO)));
      ASSERT_EQ(ff.overflow, static_cast<bool>(std::fetestexcept(FE_OVERFLOW)));
      ASSERT_EQ(ff.underflow, static_cast<bool>(std::fetestexcept(FE_UNDERFLOW)));
      ASSERT_EQ(ff.inexact, static_cast<bool>(std::fetestexcept(FE_INEXACT)));

      b = a;
      a = float_rng.Gen();
    }
  }
}

TEST(x86Tests, Min64) {
  FloppyFloat ff;
  ff.SetupTox86();

  for (auto rm : {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    FloatRng<f64> float_rng(kRngSeed);
    f64 a{float_rng.Gen()};
    f64 b{float_rng.Gen()};
    for (i32 i = 0; i < kNumIterations; ++i) {
      f64 ff_result;

      ff.ClearFlags();
      switch (rm) {
      case FE_TONEAREST:
        ff_result = ff.Minx86<f64>(a, b);
        break;
      case FE_TOWARDZERO:
        ff_result = ff.Minx86<f64>(a, b);
        break;
      case FE_DOWNWARD:
        ff_result = ff.Minx86<f64>(a, b);
        break;
      case FE_UPWARD:
        ff_result = ff.Minx86<f64>(a, b);
        break;
      default:
        break;
      }

      std::feclearexcept(FE_ALL_EXCEPT);
      fesetround(rm);
      f64 x86_result;
      asm volatile(
          "movsd %1, %%xmm0;"
          "minsd %2, %%xmm0;"
          "movsd %%xmm0, %0;"
          : "=m"(x86_result)
          : "m"(a), "m"(b)
          : "%xmm0");
      fesetround(FE_TONEAREST);

      ASSERT_EQ(std::bit_cast<u64>(x86_result), std::bit_cast<u64>(ff_result));
      ASSERT_EQ(ff.invalid, static_cast<bool>(std::fetestexcept(FE_INVALID)));
      ASSERT_EQ(ff.division_by_zero, static_cast<bool>(std::fetestexcept(FE_DIVBYZERO)));
      ASSERT_EQ(ff.overflow, static_cast<bool>(std::fetestexcept(FE_OVERFLOW)));
      ASSERT_EQ(ff.underflow, static_cast<bool>(std::fetestexcept(FE_UNDERFLOW)));
      ASSERT_EQ(ff.inexact, static_cast<bool>(std::fetestexcept(FE_INEXACT)));

      b = a;
      a = float_rng.Gen();
    }
  }
}



int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
#pragma once
/**************************************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 *
 * A faster than soft float library to simulate floating point instructions.
 * Based on: https://www.chciken.com/simulation/2023/11/12/fast-floating-point-simulation.html
 **************************************************************************************************/

#include "utils.h"

class FloppyFloat {
 public:

  // See IEEE 754-2019: 4.3 Rounding-direction attributes
  enum RoundingMode {
    kRoundTiesToEven,
    kRoundTiesToAway,
    kRoundTowardPositive,
    kRoundTowardNegative,
    kRoundTowardZero
  };

  // Floating Exception Flags.
  bool invalid;
  bool division_by_zero;
  bool overflow;
  bool underflow;
  bool inexact;

  FloppyFloat();

  template <typename FT>
  void SetQnan(typename FloatToUint<FT>::type val);

  template <typename FT>
  constexpr FT GetQnan();

  template <typename FT, RoundingMode rm = kRoundTiesToEven>
  FT Add(FT a, FT b);

protected:
  template <typename FT, RoundingMode rm>
  constexpr FT RoundResult(FT residual, FT result);

  f16 qnan16_;
  f32 qnan32_;
  f64 qnan64_;
};
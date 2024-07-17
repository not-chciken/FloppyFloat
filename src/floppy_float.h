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

  enum NanPropagationSchemes { kNanPropRiscv, kNanPropX86sse } nan_propagation_scheme;
  bool tininess_before_rounding = false;

  FloppyFloat();

  void ClearFlags();

  template <typename FT>
  void SetQnan(typename FloatToUint<FT>::type val);

  template <typename FT>
  constexpr FT GetQnan();

  template <typename FT, RoundingMode rm = kRoundTiesToEven>
  FT Add(FT a, FT b);
  template <typename FT, RoundingMode rm = kRoundTiesToEven>
  FT Sub(FT a, FT b);
  template <typename FT, RoundingMode rm = kRoundTiesToEven>
  FT Mul(FT a, FT b);
  template <typename FT, RoundingMode rm = kRoundTiesToEven>
  FT Sqrt(FT a);

  void SetupToArm();
  void SetupToRiscv();
  void SetupTox86();

 protected:
  template <typename FT, typename TFT, RoundingMode rm>
  constexpr FT RoundResult(TFT residual, FT result);
  template <typename FT>
  constexpr FT PropagateNan(FT a, FT b);

  f16 qnan16_;
  f32 qnan32_;
  f64 qnan64_;
};
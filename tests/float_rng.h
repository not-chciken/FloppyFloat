/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 ******************************************************************************/

#include <cmath>
#include <limits>
#include <random>

#include "utils.h"

template <typename T>
using nl = std::numeric_limits<T>;

template <typename FT>
class FloatRng {
 public:
  FloatRng(int seed) : index_(0), engine_(seed), dist_(), values_() {
    values_.push_back(nl<FT>::round_error());
    values_.push_back(nl<FT>::epsilon());
    values_.push_back(nl<FT>::infinity());
    values_.push_back(-nl<FT>::infinity());
    values_.push_back(nl<FT>::lowest());
    values_.push_back(nl<FT>::max());
    values_.push_back(nl<FT>::min());
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back((FT)2147483904.0);
    values_.push_back((FT)-2147483904.0);
    values_.push_back((FT)2147483648.0);
    values_.push_back((FT)-2147483648.0);
    values_.push_back((FT)2147483648.5);
    values_.push_back((FT)-2147483648.5);
    values_.push_back((FT)2147483648.75);
    values_.push_back((FT)-2147483648.75);
    values_.push_back((FT)2147483520.0);
    values_.push_back((FT)-2147483520.0);
    values_.push_back((FT)0.0);
    values_.push_back((FT)0.0);
    values_.push_back((FT)-0.0);
    values_.push_back((FT)0.0);
    values_.push_back((FT)4.3125);
    values_.push_back((FT)0.0);
    values_.push_back((FT)-4.3125);
    values_.push_back((FT)0.0);
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back((FT)0.0);
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back((FT)0.0);
    values_.push_back(nl<FT>::infinity());
    values_.push_back((FT)0.0);
    values_.push_back(-nl<FT>::infinity());
    values_.push_back((FT)0.0);
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back((FT)0.0);
    values_.push_back(-nl<FT>::denorm_min());
    values_.push_back((FT)0.0);
    values_.push_back(nl<FT>::max());
    values_.push_back((FT)0.0);
    values_.push_back(nl<FT>::lowest());
    values_.push_back((FT)0.0);
    values_.push_back(nl<FT>::lowest());
    values_.push_back(nl<FT>::lowest());
    values_.push_back((FT)-0.0);
    values_.push_back((FT)-0.0);
    values_.push_back((FT)4.3125);
    values_.push_back((FT)-0.0);
    values_.push_back((FT)-4.3125);
    values_.push_back((FT)-0.0);
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back((FT)-0.0);
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back((FT)-0.0);
    values_.push_back(nl<FT>::infinity());
    values_.push_back((FT)-0.0);
    values_.push_back(-nl<FT>::infinity());
    values_.push_back((FT)-0.0);
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back((FT)-0.0);
    values_.push_back(-nl<FT>::denorm_min());
    values_.push_back((FT)-0.0);
    values_.push_back(nl<FT>::max());
    values_.push_back((FT)-0.0);
    values_.push_back(nl<FT>::lowest());
    values_.push_back((FT)4.3125);
    values_.push_back((FT)4.3125);
    values_.push_back((FT)-4.3125);
    values_.push_back((FT)4.3125);
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back((FT)4.3125);
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back((FT)4.3125);
    values_.push_back(nl<FT>::infinity());
    values_.push_back((FT)4.3125);
    values_.push_back(-nl<FT>::infinity());
    values_.push_back((FT)4.3125);
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back((FT)4.3125);
    values_.push_back(-nl<FT>::denorm_min());
    values_.push_back((FT)4.3125);
    values_.push_back(nl<FT>::max());
    values_.push_back((FT)4.3125);
    values_.push_back(nl<FT>::lowest());
    values_.push_back((FT)-4.3125);
    values_.push_back((FT)-4.3125);
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back((FT)-4.3125);
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back((FT)-4.3125);
    values_.push_back(nl<FT>::infinity());
    values_.push_back((FT)-4.3125);
    values_.push_back(-nl<FT>::infinity());
    values_.push_back((FT)-4.3125);
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back((FT)-4.3125);
    values_.push_back(-nl<FT>::denorm_min());
    values_.push_back((FT)-4.3125);
    values_.push_back(nl<FT>::max());
    values_.push_back((FT)-4.3125);
    values_.push_back(nl<FT>::lowest());
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back(nl<FT>::infinity());
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back(-nl<FT>::infinity());
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back(-nl<FT>::denorm_min());
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back(nl<FT>::max());
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back(nl<FT>::lowest());
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back(nl<FT>::infinity());
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back(-nl<FT>::infinity());
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back(-nl<FT>::denorm_min());
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back(nl<FT>::max());
    values_.push_back(nl<FT>::signaling_NaN());
    values_.push_back(nl<FT>::lowest());
    values_.push_back(nl<FT>::infinity());
    values_.push_back(nl<FT>::infinity());
    values_.push_back(-nl<FT>::infinity());
    values_.push_back(nl<FT>::infinity());
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back(nl<FT>::infinity());
    values_.push_back(-nl<FT>::denorm_min());
    values_.push_back(nl<FT>::infinity());
    values_.push_back(nl<FT>::max());
    values_.push_back(nl<FT>::infinity());
    values_.push_back(nl<FT>::lowest());
    values_.push_back(-nl<FT>::infinity());
    values_.push_back(-nl<FT>::infinity());
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back(-nl<FT>::infinity());
    values_.push_back(-nl<FT>::denorm_min());
    values_.push_back(-nl<FT>::infinity());
    values_.push_back(nl<FT>::max());
    values_.push_back(-nl<FT>::infinity());
    values_.push_back(nl<FT>::lowest());
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back(-nl<FT>::denorm_min());
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back(nl<FT>::max());
    values_.push_back(nl<FT>::denorm_min());
    values_.push_back(nl<FT>::lowest());
    values_.push_back(-nl<FT>::denorm_min());
    values_.push_back(-nl<FT>::denorm_min());
    values_.push_back(nl<FT>::max());
    values_.push_back(-nl<FT>::denorm_min());
    values_.push_back(nl<FT>::lowest());
    values_.push_back(nl<FT>::max());
    values_.push_back(nl<FT>::max());
    values_.push_back(nl<FT>::lowest());
    values_.push_back(nl<FT>::quiet_NaN());
    values_.push_back(nl<FT>::infinity());
    values_.push_back((FT)0.);
    values_.push_back(FfUtils::CreateQnanWithPayload<FT>(1u));
    values_.push_back(FfUtils::CreateQnanWithPayload<FT>(1u));
    values_.push_back(FfUtils::CreateQnanWithPayload<FT>(128u));
    values_.push_back(FfUtils::CreateQnanWithPayload<FT>(37u));
    values_.push_back((FT)0.5);
    values_.push_back((FT)-0.5);
    values_.push_back(nl<FT>::max());
    values_.push_back((FT)32.);
    values_.push_back(nl<FT>::max());
    values_.push_back((FT)48.);
    values_.push_back(nl<FT>::max());
    values_.push_back((FT)16.);
    values_.push_back(nl<FT>::max());
    values_.push_back(FfUtils::NextUpNoNegZero((FT)1.));
    values_.push_back(FfUtils::NextDownNoPosZero(nl<FT>::max()));
    values_.push_back(nl<FT>::max());
    values_.push_back((FT)-32.);
    values_.push_back(-nl<FT>::max());
    values_.push_back((FT)-48.);
    values_.push_back(-nl<FT>::max());
    values_.push_back((FT)-16.);
    values_.push_back(-nl<FT>::max());
    values_.push_back(FfUtils::NextUpNoNegZero((FT)1.));
    values_.push_back((FT)-FfUtils::NextDownNoPosZero(nl<FT>::max()));
    values_.push_back(FfUtils::NextDownNoPosZero((FT)1.));
    values_.push_back(nl<FT>::max());
    values_.push_back(FfUtils::NextDownNoPosZero((FT)1.));
    values_.push_back((FT)36.);
    values_.push_back((FT)1);
    values_.push_back(nl<FT>::max());
    values_.push_back((FT)32.);
    values_.push_back((FT)1);
    values_.push_back(nl<FT>::max());
    values_.push_back((FT)64.);
    values_.push_back((FT)1.);
    values_.push_back(nl<FT>::max());
    values_.push_back((FT)0.);
    values_.push_back(FfUtils::NextUpNoNegZero((FT)1.));
    values_.push_back((FT)-FfUtils::NextDownNoPosZero(nl<FT>::max()));
    values_.push_back((FT)0.);
    values_.push_back(FfUtils::NextDownNoPosZero((FT)1.));
    values_.push_back((FT)-FfUtils::NextDownNoPosZero(nl<FT>::max()));
    values_.push_back((FT)std::bit_cast<FfUtils::f32>(0x387ff800u));
    values_.push_back((FT)std::bit_cast<FfUtils::f32>(0x387ff000u));
    values_.push_back((FT)std::bit_cast<FfUtils::f32>(0x387fe800u));
    values_.push_back((FT)std::bit_cast<FfUtils::f32>(0x387fe000u));
    values_.push_back((FT)std::bit_cast<FfUtils::f64>(0x380ffffff8000000ull));
    values_.push_back((FT)std::bit_cast<FfUtils::f64>(0x380ffffff0000000ull));
    values_.push_back((FT)std::bit_cast<FfUtils::f64>(0x380fffffe8000000ull));
    values_.push_back((FT)std::bit_cast<FfUtils::f64>(0x380fffffe0000000ull));
    values_.push_back((FT)0.0);
    values_.push_back((FT)0.01171875);
    values_.push_back((FT)0.005207062);
    values_.push_back((FT)0.015136719);
    values_.push_back((FT)0.004032135);
    values_.push_back((FT)0.0);
    values_.push_back((FT)std::bit_cast<FfUtils::f32>(0x207e0000u));
    values_.push_back((FT)std::bit_cast<FfUtils::f32>(0x1f810204u));
    values_.push_back(-(FT)std::bit_cast<FfUtils::f32>(0x207e0000u));
    values_.push_back((FT)0.0);
    values_.push_back((FT)std::bit_cast<FfUtils::f64>(0x1ffffffffffffffeu));
    values_.push_back((FT)std::bit_cast<FfUtils::f64>(0x2000000000000001u));
    values_.push_back(-(FT)std::bit_cast<FfUtils::f64>(0x1ffffffffffffffeu));
  }

  FT Gen() {
    if (index_ < values_.size())
      return values_[index_++];

    return std::bit_cast<FT>(dist_(engine_));
  }

  void Reset() {
    index_ = 0;
  }

 private:
  size_t index_;
  std::mt19937 engine_;
  std::uniform_int_distribution<typename FfUtils::FloatToUint<FT>::type> dist_;
  std::vector<FT> values_;
};

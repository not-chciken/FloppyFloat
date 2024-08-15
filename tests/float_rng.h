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

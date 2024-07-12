/*******************************************************************************
 * Apache License, Version 2.0
 * Copyright (c) 2024 chciken/Niko Zurstraßen
 ******************************************************************************/

#include <gtest/gtest.h>

#include "floppy_float.h"


TEST(CoreTests, RoundResult) {
  FloppyFloat ff;
  ASSERT_EQ(ff.Add<f32>(1.f, 1.f), 2.f);
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
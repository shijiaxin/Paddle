#include "paddle/platform/place.h"
#include <sstream>
#include "gtest/gtest.h"

TEST(Place, Equality) {
  paddle::platform::CPUPlace cpu;
  paddle::platform::GPUPlace g0(0), g1(1), gg0(0);

  EXPECT_EQ(cpu, cpu);
  EXPECT_EQ(g0, g0);
  EXPECT_EQ(g1, g1);
  EXPECT_EQ(g0, gg0);

  EXPECT_NE(g0, g1);

  EXPECT_TRUE(paddle::platform::places_are_same_class(g0, gg0));
  EXPECT_FALSE(paddle::platform::places_are_same_class(g0, cpu));
}

TEST(Place, Default) {
  EXPECT_TRUE(paddle::platform::is_gpu_place(paddle::platform::get_place()));
  EXPECT_TRUE(paddle::platform::is_gpu_place(paddle::platform::default_gpu()));
  EXPECT_TRUE(paddle::platform::is_cpu_place(paddle::platform::default_cpu()));

  paddle::platform::set_place(paddle::platform::CPUPlace());
  EXPECT_TRUE(paddle::platform::is_cpu_place(paddle::platform::get_place()));
}

TEST(Place, WithFPGAPlace) {
  paddle::platform::CPUPlace cpu;
  paddle::platform::GPUPlace g0(0);
  paddle::platform::FPGAPlace f0(0), f1(1), ff0(0);

  EXPECT_EQ(f0, f0);
  EXPECT_EQ(f0, ff0);

  EXPECT_NE(f0, f1);

  EXPECT_TRUE(paddle::platform::places_are_same_class(f0, f1));
  EXPECT_TRUE(paddle::platform::places_are_same_class(f0, ff0));
  EXPECT_FALSE(paddle::platform::places_are_same_class(f0, cpu));
  EXPECT_FALSE(paddle::platform::places_are_same_class(f1, g0));

  EXPECT_FALSE(
      paddle::platform::is_fpga_place(paddle::platform::default_cpu()));
  EXPECT_TRUE(
      paddle::platform::is_fpga_place(paddle::platform::default_fpga()));

  paddle::platform::set_place(paddle::platform::FPGAPlace());
  EXPECT_TRUE(paddle::platform::is_fpga_place(paddle::platform::get_place()));

  {
    std::stringstream ss;
    ss << paddle::platform::FPGAPlace(1);
    EXPECT_EQ("FPGAPlace(1)", ss.str());
  }
}

TEST(Place, Print) {
  {
    std::stringstream ss;
    ss << paddle::platform::GPUPlace(1);
    EXPECT_EQ("GPUPlace(1)", ss.str());
  }
  {
    std::stringstream ss;
    ss << paddle::platform::CPUPlace();
    EXPECT_EQ("CPUPlace", ss.str());
  }
}

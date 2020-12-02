#include <stan/math/prim.hpp>
#include <test/unit/util.hpp>
#include <gtest/gtest.h>

TEST(MathMatrixPrim, Zero) {
  stan::math::matrix_d m0(0, 0);
  stan::math::matrix_d ginv = stan::math::generalized_inverse(m0);
  EXPECT_EQ(0, ginv.rows());
  EXPECT_EQ(0, ginv.cols());
}

TEST(MathMatrixPrim, Equal) {
  using stan::math::generalized_inverse;

  stan::math::matrix_d m1(3, 2);
  m1 << 1, 2, 2, 4, 1, 2;

  stan::math::matrix_d m2(2, 3);
  m2 << 1 / 30.0, 1 / 15.0, 1 / 30.0, 1 / 15.0, 2 / 15.0, 1 / 15.0;
  stan::math::matrix_d m3 = generalized_inverse(m1);
  EXPECT_MATRIX_NEAR(m2, m3, 1e-9);
}

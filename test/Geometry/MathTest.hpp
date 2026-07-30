#ifndef MATH_TEST_HPP
#define MATH_TEST_HPP

#include "Test.hpp"
#include "Math/Math.hpp"

class MathTest : public Test
{
  public:
    MathTest() : Test("Math")
    {
    }

    bool Run() noexcept override
    {
        TEST_MESSAGE("Math::Constants::Pi");
        EXPECT_TRUE(Math::Constants::Pi > 3.14 && Math::Constants::Pi < 3.15, "Pi is ~3.14.", true);

        TEST_MESSAGE("Math::Abs");
        EXPECT_TRUE(Math::Abs(5) == 5, "Abs(5) = 5", true);
        EXPECT_TRUE(Math::Abs(-5) == 5, "Abs(-5) = 5", true);
        EXPECT_TRUE(Math::Abs(0.0) == 0.0, "Abs(0) = 0", true);

        TEST_MESSAGE("Math::Min / Max");
        EXPECT_TRUE(Math::Min(3, 7) == 3, "Min(3,7)=3", true);
        EXPECT_TRUE(Math::Max(3, 7) == 7, "Max(3,7)=7", true);
        EXPECT_TRUE(Math::Min(-1.0, 1.0) == -1.0, "Min(-1,1)=-1", true);

        TEST_MESSAGE("Math::Exponent");
        EXPECT_TRUE(Math::Exponent(0.0) == 1.0, "e^0 = 1", true);
        EXPECT_TRUE(Math::Abs(Math::Exponent(1.0) - 2.7182818) < 1e-4, "e^1 ~ 2.71828", true);
        EXPECT_TRUE(Math::Abs(Math::Exponent(-1.0) - 0.367879) < 1e-4, "e^-1 ~ 0.3679", true);

        TEST_MESSAGE("Math::NaturalLog");
        EXPECT_TRUE(Math::Abs(Math::NaturalLog(1.0)) < 1e-6, "ln(1) = 0", true);
        EXPECT_TRUE(Math::Abs(Math::NaturalLog(2.7182818) - 1.0) < 1e-4, "ln(e) ~ 1", true);

        TEST_MESSAGE("Math::Sine / Cosine");
        EXPECT_TRUE(Math::Abs(Math::Sine(0.0)) < 1e-6, "sin(0) = 0", true);
        EXPECT_TRUE(Math::Abs(Math::Cosine(0.0) - 1.0) < 1e-6, "cos(0) = 1", true);
        EXPECT_TRUE(Math::Abs(Math::Sine((float)Math::Constants::Pi / 2) - 1.0f) < 1e-4f, "sin(pi/2) ~ 1", true);
        EXPECT_TRUE(Math::Abs(Math::Cosine((float)Math::Constants::Pi) - (-1.0f)) < 1e-4f, "cos(pi) ~ -1", true);

        TEST_MESSAGE("Math::Tangent");
        EXPECT_TRUE(Math::Abs(Math::Tangent(0.0)) < 1e-6, "tan(0) = 0", true);
        EXPECT_TRUE(Math::Abs(Math::Tangent((float)Math::Constants::Pi / 4) - 1.0f) < 1e-4f, "tan(pi/4) ~ 1", true);

        TEST_MESSAGE("Math::Sinh / Cosh / Tanh");
        EXPECT_TRUE(Math::Abs(Math::Sinh(0.0)) < 1e-6, "sinh(0) = 0", true);
        EXPECT_TRUE(Math::Abs(Math::Cosh(0.0) - 1.0) < 1e-6, "cosh(0) = 1", true);
        EXPECT_TRUE(Math::Abs(Math::Tanh(0.0)) < 1e-6, "tanh(0) = 0", true);

        TEST_MESSAGE("Math::Power");
        EXPECT_TRUE(Math::Power(2, 0) == 1, "2^0 = 1", true);
        EXPECT_TRUE(Math::Power(2, 3) == 8, "2^3 = 8", true);
        EXPECT_TRUE(Math::Abs(Math::Power(2.0, -1) - 0.5) < 1e-9, "2^-1 = 0.5", true);
        EXPECT_TRUE(Math::Abs(Math::Power(9.0, 0.5) - 3.0) < 1e-4, "9^0.5 ~ 3", true);

        TEST_MESSAGE("Math::ReLU");
        EXPECT_TRUE(Math::ReLU(5) == 5, "ReLU(5)=5", true);
        EXPECT_TRUE(Math::ReLU(-3) == 0, "ReLU(-3)=0", true);

        TEST_MESSAGE("Math::Sigmoid");
        EXPECT_TRUE(Math::Abs(Math::Sigmoid(0.0) - 0.5) < 1e-4, "sigmoid(0)=0.5", true);
        EXPECT_TRUE(Math::Sigmoid(100.0) > 0.99f, "sigmoid(100)~1", true);
        EXPECT_TRUE(Math::Sigmoid(-100.0) < 0.01f, "sigmoid(-100)~0", true);

        TEST_MESSAGE("Math::ToRadians");
        EXPECT_TRUE(Math::Abs(Math::ToRadians(180.0) - Math::Constants::Pi) < 1e-6, "180deg = pi rad", true);
        EXPECT_TRUE(Math::Abs(Math::ToRadians(90.0) - Math::Constants::Pi / 2) < 1e-6, "90deg = pi/2 rad", true);

        SUCCESS_MESSAGE("Math");
        return true;
    }
};

#endif
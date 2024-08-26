#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Rational.h>
#include <Foundation/Strings/StringBuilder.h>

EZ_CREATE_SIMPLE_TEST(Math, Rational)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Rational")
  {
    ezRational r1(100, 1);

    EZ_TEST_BOOL(r1.IsValid());
    EZ_TEST_BOOL(r1.IsIntegral());

    ezRational r2(100, 0);
    EZ_TEST_BOOL(!r2.IsValid());

    EZ_TEST_BOOL(r1 != r2);

    ezRational r3(100, 1);
    EZ_TEST_BOOL(r3 == r1);

    ezRational r4(0, 0);
    EZ_TEST_BOOL(r4.IsValid());


    ezRational r5(30, 6);
    EZ_TEST_BOOL(r5.IsIntegral());
    EZ_TEST_INT(r5.GetIntegralResult(), 5);
    EZ_TEST_FLOAT(r5.GetFloatingPointResult(), 5, ezMath::SmallEpsilon<double>());

    ezRational reducedTest(5, 1);
    EZ_TEST_BOOL(r5.ReduceIntegralFraction() == reducedTest);

    ezRational r6(31, 6);
    EZ_TEST_BOOL(!r6.IsIntegral());
    EZ_TEST_FLOAT(r6.GetFloatingPointResult(), 5.16666666666, ezMath::SmallEpsilon<double>());


    EZ_TEST_INT(r6.GetDenominator(), 6);
    EZ_TEST_INT(r6.GetNumerator(), 31);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Rational String Formatting")
  {
    ezRational r1(50, 25);

    ezStringBuilder sb;
    sb.SetFormat("Rational: {}", r1);
    EZ_TEST_STRING(sb, "Rational: 2");


    ezRational r2(233, 76);
    sb.SetFormat("Rational: {}", r2);
    EZ_TEST_STRING(sb, "Rational: 233/76");
  }
}

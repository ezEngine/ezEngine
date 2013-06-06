#include <PCH.h>
#include <Foundation/Math/FixedPoint.h>

//template<typename T>
//T Sqrt(T a)
//{
//  T x = a / 2;
//
//  for (ezUInt32 i = 0; i < 6; ++i)
//  {
//    T ax = a / x;
//    T xpax = x + ax;
//    x = xpax / 2;
//  }
//
//  return x;
//}

EZ_CREATE_SIMPLE_TEST(Math, FixedPoint)
{
  //EZ_TEST_BLOCK(true, "Sqrt")
  //{
  //  float f1 = Sqrt(ezFixedPoint<12> (16.0f));
  //  float f2 = Sqrt(ezFixedPoint<12> (25.0f));
  //  float f3 = Sqrt(ezFixedPoint<12> (64.0f));
  //  float f4 = Sqrt(ezFixedPoint<12> (100.0f));

  //  int i = 2;
  //}

  EZ_TEST_BLOCK(true, "Constructor (int) / Conversion to Int")
  {
    // positive values
    for (ezInt32 i = 0; i < 1024; ++i)
    {
      ezFixedPoint<12> fp (i);
      EZ_TEST_INT(fp, i);
    }

    // negative values
    for (ezInt32 i = 0; i < 1024; ++i)
    {
      ezFixedPoint<12> fp (-i);
      EZ_TEST_INT(fp, -i);
    }
  }

  EZ_TEST_BLOCK(true, "Constructor (float) / Conversion to Float")
  {
    // positive values
    for (float f = 0.0f; f < 100.0f; f += 0.01f)
    {
      ezFixedPoint<12> fp (f);

      EZ_TEST_FLOAT(fp, f, 0.001f);
    }

    // negative values
    for (float f = 0.0f; f < 100.0f; f += 0.01f)
    {
      ezFixedPoint<12> fp (-f);

      EZ_TEST_FLOAT(fp, -f, 0.001f);
    }
  }

  EZ_TEST_BLOCK(true, "Constructor (double) / Conversion to double")
  {
    // positive values
    for (double f = 0.0; f < 100.0; f += 0.01)
    {
      ezFixedPoint<12> fp (f);

      EZ_TEST_DOUBLE(fp, f, 0.001);
    }

    // negative values
    for (double f = 0.0; f < 100.0f; f += 0.01)
    {
      ezFixedPoint<12> fp (-f);

      EZ_TEST_DOUBLE(fp, -f, 0.001);
    }
  }

  EZ_TEST_BLOCK(true, "Constructor (Other) / Assignment")
  {
    ezFixedPoint<12> fp1 (2.4f);
    ezFixedPoint<12> fp2 (fp1);
    ezFixedPoint<12> fp3;

    fp3 = fp1;

    EZ_TEST(fp1 == fp1);
    EZ_TEST(fp2 == fp2);
    EZ_TEST(fp3 == fp3);

    EZ_TEST(fp1 == fp2);
    EZ_TEST(fp1 == fp3);
    EZ_TEST(fp2 == fp3);
  }

  EZ_TEST_BLOCK(true, "Max Value")
  {
    ezFixedPoint<12> fp1 ( (1 << 19) - 1);
    ezFixedPoint<12> fp2 ( (1 << 19));
    ezFixedPoint<12> fp3 (-(1 << 19)); // one more value available in the negative range
    ezFixedPoint<12> fp4 (-(1 << 19) - 1);

    // 12 Bits for the fraction -> 19 Bits for the integral part and 1 'Sign Bit'
    EZ_TEST((ezInt32) fp1 ==  (1 << 19) - 1); // This maximum value is still representable
    EZ_TEST((ezInt32) fp2 !=  (1 << 19));     // The next value isn't representable anymore
    EZ_TEST((ezInt32) fp3 == -(1 << 19));
    EZ_TEST((ezInt32) fp4 != -(1 << 19) - 1);
  }

  EZ_TEST_BLOCK(true, "operator*(fp, int)")
  {
    ezFixedPoint<12> fp (3.2f);
    fp = fp * 2;

    EZ_TEST_FLOAT((float) fp, 6.4f, 0.001f);

    fp = 3 * fp;

    EZ_TEST_FLOAT((float) fp, 19.2f, 0.001f);
  }

  EZ_TEST_BLOCK(true, "operator/(fp, int)")
  {
    ezFixedPoint<12> fp (12.4f);
    fp = fp / 2;

    EZ_TEST_FLOAT((float) fp, 6.2f, 0.001f);

    fp = fp / 3;

    EZ_TEST_FLOAT((float) fp, 2.066f, 0.001f);
  }

  EZ_TEST_BLOCK(true, "operator+(fp, fp)")
  {
    ezFixedPoint<12> fp (3.2f);
    fp = fp + ezFixedPoint<12>(2);

    EZ_TEST_FLOAT((float) fp, 5.2f, 0.001f);

    fp = ezFixedPoint<12>(3) + fp;

    EZ_TEST_FLOAT((float) fp, 8.2f, 0.001f);
  }

  EZ_TEST_BLOCK(true, "operator-(fp, fp)")
  {
    ezFixedPoint<12> fp (3.2f);
    fp = fp - ezFixedPoint<12>(2);

    EZ_TEST_FLOAT((float) fp, 1.2f, 0.001f);

    fp = ezFixedPoint<12>(3) - fp;

    EZ_TEST_FLOAT((float) fp, 1.8f, 0.001f);
  }

  EZ_TEST_BLOCK(true, "operator*(fp, fp)")
  {
    ezFixedPoint<12> fp (3.2f);

    fp = fp * ezFixedPoint<12>(2.5f);
    EZ_TEST_FLOAT((float) fp, 8.0f, 0.001f);

    fp = fp * ezFixedPoint<12>(-123.456f);
    EZ_TEST_FLOAT((float) fp, -987.648f, 0.1f);
  }

  EZ_TEST_BLOCK(true, "operator/(fp, fp)")
  {
    ezFixedPoint<12> fp (100000.248f);

    fp = fp / ezFixedPoint<12>(-2);
    EZ_TEST_FLOAT((float) fp, -50000.124f, 0.001f);

    fp = fp / ezFixedPoint<12>(-4);
    EZ_TEST_FLOAT((float) fp, 12500.031f, 0.001f);
  }

  EZ_TEST_BLOCK(true, "Operator<,>,<=,>=,==,!=")
  {
    ezFixedPoint<12> fp1 (1);
    ezFixedPoint<12> fp2 (2.0f);
    ezFixedPoint<12> fp3 (3);
    ezFixedPoint<12> fp3b (3.0f);

    EZ_TEST(fp1 < fp2);
    EZ_TEST(fp3 > fp2);
    EZ_TEST(fp3 <= fp3b);
    EZ_TEST(fp3 >= fp3b);
    EZ_TEST(fp1 != fp2);
    EZ_TEST(fp3 == fp3b);
  }

  EZ_TEST_BLOCK(true, "Assignment Rounding")
  {
    ezFixedPoint<2> fp; // 2 Bits -> 4 fractional values

    fp = 1000.25f;
    EZ_TEST_FLOAT(fp, 1000.25f, 0.01f);

    fp = 1000.75f;
    EZ_TEST_FLOAT(fp, 1000.75f, 0.01f);



    fp = 1000.1f;
    EZ_TEST_DOUBLE(fp, 1000.0, 0.01);

    fp = 1000.2f;
    EZ_TEST_DOUBLE(fp, 1000.25, 0.01);

    fp = 1000.3f;
    EZ_TEST_DOUBLE(fp, 1000.25, 0.01);

    fp = 1000.4f;
    EZ_TEST_DOUBLE(fp, 1000.5, 0.01);

    fp = 1000.5f;
    EZ_TEST_DOUBLE(fp, 1000.5, 0.01);

    fp = 1000.6f;
    EZ_TEST_DOUBLE(fp, 1000.5, 0.01);

    fp = 1000.7f;
    EZ_TEST_DOUBLE(fp, 1000.75, 0.01);

    fp = 1000.8f;
    EZ_TEST_DOUBLE(fp, 1000.75, 0.01);

    fp = 1000.9f;
    EZ_TEST_DOUBLE(fp, 1001.0, 0.01);


    // negative
    fp = -1000.1;
    EZ_TEST_FLOAT(fp, -1000.0f, 0.01f);

    fp = -1000.2;
    EZ_TEST_FLOAT(fp, -1000.25f, 0.01f);

    fp = -1000.3;
    EZ_TEST_FLOAT(fp, -1000.25f, 0.01f);

    fp = -1000.4;
    EZ_TEST_FLOAT(fp, -1000.5f, 0.01f);

    fp = -1000.5;
    EZ_TEST_FLOAT(fp, -1000.5f, 0.01f);

    fp = -1000.6;
    EZ_TEST_FLOAT(fp, -1000.5f, 0.01f);

    fp = -1000.7;
    EZ_TEST_FLOAT(fp, -1000.75f, 0.01f);

    fp = -1000.8;
    EZ_TEST_FLOAT(fp, -1000.75f, 0.01f);

    fp = -1000.9;
    EZ_TEST_FLOAT(fp, -1001.0f, 0.01f);
  }

  EZ_TEST_BLOCK(true, "Multiplication Rounding")
  {
    ezFixedPoint<2> fp; // 2 Bits -> 4 fractional values

    fp = 0.25;
    fp *= ezFixedPoint<2>(1.5); // -> should be 0.375, which is not representable -> will be rounded up

    EZ_TEST_FLOAT(fp, 0.5f, 0.01f);

    fp = -0.25;
    fp *= ezFixedPoint<2>(1.5); // -> should be -0.375, which is not representable -> will be rounded up (towards zero)

    EZ_TEST_FLOAT(fp, -0.25f, 0.01f);
  }

  EZ_TEST_BLOCK(true, "Division Rounding")
  {
    ezFixedPoint<12> fp2 (1000);
    EZ_TEST_INT(fp2.GetRawValue(), 1000 << 12);

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), 500 << 12);

    fp2 += ezFixedPoint<12>(1);
    EZ_TEST_INT(fp2.GetRawValue(), 501 << 12);

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (250 << 12) + (1 << 11));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (125 << 12) + (1 << 10));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (62 << 12) + (1 << 11) + (1 << 9));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (31 << 12) + (1 << 10) + (1 << 8));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (15 << 12) + (1 << 11) + (1 << 9) + (1 << 7));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (7 << 12) + (1 << 11) + (1 << 10) + (1 << 8) + (1 << 6));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (3 << 12) + (1 << 11) + (1 << 10) + (1 << 9) + (1 << 7) + (1 << 5));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 12) + (1 << 11) + (1 << 10) + (1 << 9) + (1 << 8) + (1 << 6) + (1 << 4));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 11) + (1 << 10) + (1 << 9) + (1 << 8) + (1 << 7) + (1 << 5) + (1 << 3));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 10) + (1 << 9) + (1 << 8) + (1 << 7) + (1 << 6) + (1 << 4) + (1 << 2));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 9) + (1 << 8) + (1 << 7) + (1 << 6) + (1 << 5) + (1 << 3) + (1 << 1));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 8) + (1 << 7) + (1 << 6) + (1 << 5) + (1 << 4) + (1 << 2) + (1 << 0));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 7) + (1 << 6) + (1 << 5) + (1 << 4) + (1 << 3) + (1 << 1) + (1 << 0)); // here we round up

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 6) + (1 << 5) + (1 << 4) + (1 << 3) + (1 << 2) + (1 << 1));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 5) + (1 << 4) + (1 << 3) + (1 << 2) + (1 << 1) + (1 << 0));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 4) + (1 << 3) + (1 << 2) + (1 << 1) + (1 << 0) + (1 << 0)); // here we round up again

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 3) + (1 << 2) + (1 << 1) + (1 << 0) + (1 << 0)); // here we round up again

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 2) + (1 << 1) + (1 << 1)); // here we round up again

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 1) + (1 << 1)); // here we round up again

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 1));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 0));

    fp2 /= ezFixedPoint<12>(2);
    EZ_TEST_INT(fp2.GetRawValue(), (1 << 0)); // we can never get lower than this by dividing by 2, as it will always get rounded up again

    fp2 /= ezFixedPoint<12>(2.01);
    EZ_TEST_INT(fp2.GetRawValue(), 0); // finally we round down
  }
}




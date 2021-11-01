#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/MathExpression.h>

EZ_CREATE_SIMPLE_TEST(CodeUtils, MathExpression)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Basics")
  {
    {
      ezMathExpression expr("");
      EZ_TEST_BOOL(!expr.IsValid());

      expr.Reset("");
      EZ_TEST_BOOL(!expr.IsValid());
    }
    {
      ezMathExpression expr(nullptr);
      EZ_TEST_BOOL(!expr.IsValid());

      expr.Reset(nullptr);
      EZ_TEST_BOOL(!expr.IsValid());
    }
    {
      ezMathExpression expr("1.5 + 2.5");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 4.0, 0.0);
    }
    {
      ezMathExpression expr("1- 2");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), -1.0, 0.0);
    }
    {
      ezMathExpression expr("1 *2");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
    {
      ezMathExpression expr(" 1/2 ");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 0.5, 0.0);
    }
    {
      ezMathExpression expr("1 - -1");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
    {
      ezMathExpression expr("abs -1");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 1.0, 0.0);
    }
    {
      ezMathExpression expr("abs(-3)");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
    {
      ezMathExpression expr("sqrt(4)");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Operator Priority")
  {
    {
      ezMathExpression expr("1 - 2 * 4");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), -7.0, 0.0);
    }
    {
      ezMathExpression expr("-1 - 2 * 4");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), -9.0, 0.0);
    }
    {
      ezMathExpression expr("1 - 2 / 4");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 0.5, 0.0);
    }
    {
      ezMathExpression expr("abs -4 + 2");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 6.0, 0.0);
    }
    {
      ezMathExpression expr("abs (-4 + 2)");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Braces")
  {
    {
      ezMathExpression expr("(1 - 2) * 4");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), -4.0, 0.0);
    }
    {
      ezMathExpression expr("(((((0)))))");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 0.0, 0.0);
    }
    {
      ezMathExpression expr("(1 + 2) * (3 - 2)");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Variables")
  {
    ezHybridArray<ezMathExpression::Input, 4> inputs;
    inputs.SetCount(4);

    {
      ezMathExpression expr("_var1 + v2Ar");
      EZ_TEST_BOOL(expr.IsValid());

      inputs[0] = {ezMakeHashedString("_var1"), 1.0};
      inputs[1] = {ezMakeHashedString("v2Ar"), 2.0};

      double result = expr.Evaluate(inputs);
      EZ_TEST_DOUBLE(result, 3.0, 0.0);

      inputs[0].m_fValue = 2.0;
      inputs[1].m_fValue = 0.5;

      result = expr.Evaluate(inputs);
      EZ_TEST_DOUBLE(result, 2.5, 0.0);
    }

    // Make sure we got the spaces right and don't count it as part of the variable.
    {
      ezMathExpression expr("  a +  b /c*d");
      EZ_TEST_BOOL(expr.IsValid());

      inputs[0] = {ezMakeHashedString("a"), 1.0};
      inputs[1] = {ezMakeHashedString("b"), 4.0};
      inputs[2] = {ezMakeHashedString("c"), 2.0};
      inputs[3] = {ezMakeHashedString("d"), 3.0};

      double result = expr.Evaluate(inputs);
      EZ_TEST_DOUBLE(result, 7.0, 0.0);
    }
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invalid Expressions")
  {
    ezMuteLog logErrorSink;
    ezLogSystemScope ls(&logErrorSink);

    {
      ezMathExpression expr("1+");
      EZ_TEST_BOOL(!expr.IsValid());
    }
    {
      ezMathExpression expr("1+/1");
      EZ_TEST_BOOL(!expr.IsValid());
    }
    {
      ezMathExpression expr("(((((0))))");
      EZ_TEST_BOOL(!expr.IsValid());
    }
    {
      ezMathExpression expr("_va£r + asdf");
      EZ_TEST_BOOL(!expr.IsValid());
    }
  }
}

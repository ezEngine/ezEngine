#include <PCH.h>

#include <Foundation/CodeUtils/MathExpression.h>

class IgnoreLogInterface : public ezLogInterface
{
public:
  virtual void HandleLogMessage(const ezLoggingEventData& le) override {}
};

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
      ezMathExpression expr(nullptr, nullptr);
      EZ_TEST_BOOL(!expr.IsValid());

      expr.Reset(nullptr);
      EZ_TEST_BOOL(!expr.IsValid());
    }
    {
      ezMathExpression expr("1 + 2");
      EZ_TEST_BOOL(expr.IsValid());
      EZ_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
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

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Variable Resolve")
  {
    {
      ezMathExpression expr("_var1 + v2Ar");
      EZ_TEST_BOOL(expr.IsValid());

      double result = expr.Evaluate([](const ezStringView& str) {
        if (str == "_var1")
          return 1.0;
        else if (str == "v2Ar")
          return 2.0;
        else
        {
          EZ_TEST_FAILURE("Test failed", "Unexpected variable name: {0}", str);
          return 0.0;
        }
      });
      EZ_TEST_DOUBLE(result, 3.0, 0.0);

      result = expr.Evaluate([](const ezStringView& str) {
        if (str == "_var1")
          return 2.0;
        else if (str == "v2Ar")
          return 0.5;
        else
        {
          EZ_TEST_FAILURE("Test failed", "Unexpected variable name: {0}", str);
          return 0.0;
        }
      });
      EZ_TEST_DOUBLE(result, 2.5, 0.0);
    }

    // Make sure we got the spaces right and don't count it as part of the variable.
    {
      ezMathExpression expr("  a +  b /c*d");
      EZ_TEST_BOOL(expr.IsValid());
      double result = expr.Evaluate([](const ezStringView& str) {
        if (str == "a")
          return 1.0;
        else if (str == "b")
          return 4.0;
        else if (str == "c")
          return 2.0;
        else if (str == "d")
          return 3.0;
        else
        {
          EZ_TEST_FAILURE("Test failed", "Unexpected variable name: {0}", str);
          return 0.0;
        }
      });
      EZ_TEST_DOUBLE(result, 7.0, 0.0);
    }
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Invalid Expressions")
  {
    IgnoreLogInterface logErrorSink;

    {
      ezMathExpression expr("1+", &logErrorSink);
      EZ_TEST_BOOL(!expr.IsValid());
    }
    {
      ezMathExpression expr("1+/1", &logErrorSink);
      EZ_TEST_BOOL(!expr.IsValid());
    }
    {
      ezMathExpression expr("(((((0))))", &logErrorSink);
      EZ_TEST_BOOL(!expr.IsValid());
    }
    {
      ezMathExpression expr("_va£r + asdf", &logErrorSink);
      EZ_TEST_BOOL(!expr.IsValid());
    }
  }
}

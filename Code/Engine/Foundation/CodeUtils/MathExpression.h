#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Strings/String.h>

class ezLogInterface;

/// \brief A wrapper around ezExpression infrastructure to evaluate simple math expressions
class EZ_FOUNDATION_DLL ezMathExpression
{
public:
  /// \brief Creates a new invalid math expression.
  ///
  /// Need to call Reset before you can do anything with it.
  ezMathExpression();

  /// \brief Initializes using a given expression.
  ///
  /// If anything goes wrong it is logged and the math expression is in an invalid state.
  /// \param log
  ///   If null, default log interface will be used.
  explicit ezMathExpression(const char* szExpressionString); // [tested]

  /// \brief Reinitializes using the given expression.
  ///
  /// An empty string or nullptr are considered to be 'invalid' expressions.
  void Reset(const char* szExpressionString);

  /// Whether the expression is valid and can be evaluated.
  bool IsValid() const { return m_bIsValid; }

  /// Returns the original expression string that this MathExpression can evaluate.
  const char* GetExpressionString() const { return m_OriginalExpression; }

  struct Input
  {
    ezHashedString m_sName;
    float m_fValue;
  };

  /// \brief Evaluates parsed expression with the given inputs.
  ///
  /// Only way this function can fail is if the expression was not valid.
  /// \see IsValid
  float Evaluate(ezArrayPtr<Input> inputs = ezArrayPtr<Input>()); // [tested]

private:
  ezHashedString m_OriginalExpression;
  bool m_bIsValid = false;

  ezExpressionByteCode m_ByteCode;
  ezExpressionVM m_VM;
};

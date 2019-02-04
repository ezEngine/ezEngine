#pragma once

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>

struct ezStringView;
class ezLogInterface;

template <typename T>
struct ezDelegate;

/// \brief Parses a math expression from a string and allows efficient evaluation with different variable configurations.
///
/// Valid is any functional, mathematical expression containing:
/// - Numbers (all numbers treated as doubles): 0123456789.
/// - Binary mathematical operators: * / - +
/// - Unary mathematical operators: - +
/// - Parenthesis: ( )
/// - Variables consisting of an arbitrary chain of: abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789 (s_szValidVariableCharacters) but mustn't start with a number.
/// Hard-coded to double.
class EZ_FOUNDATION_DLL ezMathExpression
{
public:
  static const char* s_szValidVariableCharacters;

  /// \brief Creates a new invalid math expression.
  ///
  /// Need to call Reset before you can do anything with it.
  explicit ezMathExpression(ezLogInterface* pLog = nullptr);

  /// \brief Initializes using a given expression.
  ///
  /// If anything goes wrong it is logged and the math expression is in an invalid state.
  /// \param log
  ///   If null, default log interface will be used.
  ezMathExpression(const char* szExpressionString, ezLogInterface* pLog = ezLog::GetThreadLocalLogSystem()); // [tested]

  /// \brief Reinitializes using the given expression.
  ///
  /// An empty string or nullptr are considered to be 'invalid' expressions.
  void Reset(const char* szExpressionString);

  /// Whether the expression is valid and can be evaluated.
  bool IsValid() const { return m_bIsValid; }

  /// Returns the original expression string that this MathExpression can evaluate.
  const char* GetExpressionString() const { return m_OriginalExpression; }

  /// \brief Evaluates parsed expression with given variable configuration.
  ///
  /// Only way this function can fail is if the expression was not valid.
  /// \see IsValid
  double Evaluate(const ezDelegate<double(const ezStringView&)>& variableResolveFunction = [](const ezStringView&) { return 0.0; }) const; // [tested]


  // Parsing the expression - recursive parser using "precedence climbing".
  // Note as of writing the ezPreprocessor parser uses a classic recursive descent parser.
  // http://www.engr.mun.ca/~theo/Misc/exp_parsing.htm
private:
  ezResult ParseExpression(const ezTokenParseUtils::TokenStream& tokens, ezUInt32& uiCurToken, int precedence = 0);
  ezResult ParseFactor(const ezTokenParseUtils::TokenStream& tokens, ezUInt32& uiCurToken);

  ezLogInterface* const m_pLog;
  ezString m_OriginalExpression;

  // Instruction stream.
public:
  struct InstructionType
  {
    enum Enum : ezUInt32
    {
      // Binary
      Add,      ///< Add last two elements of evaluation stack and push result back.
      Subtract, ///< Subtract last two elements of evaluation stack and push result back.
      Multiply, ///< Multiply last two elements of evaluation stack and push result back.
      Divide,   ///< Divide last two elements of evaluation stack and push result back.

      // Unary
      Negate, ///< Negate top element of the evaluation stack.

      // Special
      PushConstant, ///< Instruction is followed by an integer that determines which constant should be pushed onto the evaluation stack.
      PushVariable, ///< Instruction is followed by two integers, giving begin and end of a variable name in m_OriginalExpression

      Invalid,
    };
  };

private:
  ezDynamicArray<ezUInt32> m_InstructionStream;
  ezDynamicArray<double> m_Constants;

  bool m_bIsValid;
};


#pragma once

#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>

class ezStringView;
class ezLogInterface;

template<typename T> struct ezDelegate;

/// \brief Parses a math expression from a string and allows efficient evaluation with different variable configurations.
///
/// Valid is any functional, mathematical expression containing:
/// - Numbers (all numbers treated as doubles): 0123456789.
/// - Binary mathematical operators: * / - +
/// - Unary mathematical operators: - +
/// - Parenthesis: ( )
/// - Variables consisting of an arbitrary chain of: abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789 (s_szValidVariableCharacters) but musn't start with a number.
/// Hardcoded to double.
class EZ_FOUNDATION_DLL ezMathExpression
{
public:
  static const char* s_szValidVariableCharacters;

  /// \brief Inits using a given expression.
  ///
  /// If anything goes wrong it is logged and the math expression is in an invalid state.
  /// \param log
  ///   If null, default log interface will be used.
  ezMathExpression(const char* expression, ezLogInterface* log = nullptr);  // [tested]

  /// Whether the expression is valid and can be evaluated.
  bool IsValid() const { return m_bIsValid; }

  /// \brief Evaluates parsed expression with given variable configuration.
  ///
  /// Only way this function can fail is if the expression was not valid.
  /// \see IsValid
  double Evaluate(const ezDelegate<double(const ezStringView&)>& variableResolveFunction = [](const ezStringView&) { return 0.0; }) const; // [tested]


  // Parsing the expression - recursive parser.
private:

  /// \brief Entry point for parsing the math expression from a token stream.
  ///
  /// This function wraps the expression with the lowest priority.
  ezResult ParseExpression(const ezTokenParseUtils::TokenStream& tokens, ezUInt32& uiCurToken) { return ParseExpressionPlus(tokens, uiCurToken);  }

  ezResult ParseExpressionPlus(const ezTokenParseUtils::TokenStream& tokens, ezUInt32& uiCurToken);
  ezResult ParseExpressionMul(const ezTokenParseUtils::TokenStream& tokens, ezUInt32& uiCurToken);
  ezResult ParseFactor(const ezTokenParseUtils::TokenStream& tokens, ezUInt32& uiCurToken);


  ezLogInterface* const m_Log;
  const ezString m_OriginalExpression;

  // Instruction stream.
private:

  enum class InstructionType : ezUInt32
  {
    // Binary
    Add,      ///< Add last two elements of evaluation stack and push result back.
    Subtract, ///< Subtract last two elements of evaluation stack and push result back.
    Multiply, ///< Multiply last two elements of evaluation stack and push result back.
    Divide,   ///< Divide last two elements of evaluation stack and push result back.

    // Unary
    Negate,   ///< Negate top element of the evaluation stack.

    // Special
    PushConstant, ///< Instruction is followed by an integer that determines which constant should be pushed onto the evaluation stack.
    PushVariable, ///< Instruction is followed by two integers, giving begin and end of a variable name in m_OriginalExpression
  };

  ezDynamicArray<ezUInt32> m_InstructionStream;
  ezDynamicArray<double> m_Constants;

  bool m_bIsValid;
};

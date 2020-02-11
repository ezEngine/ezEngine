#include <FoundationPCH.h>

#include <Foundation/CodeUtils/MathExpression.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Utilities/ConversionUtils.h>

using namespace ezTokenParseUtils;

const char* ezMathExpression::s_szValidVariableCharacters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789";

ezMathExpression::ezMathExpression(ezLogInterface* pLog)
    : m_pLog(pLog ? pLog : ezLog::GetThreadLocalLogSystem()), m_bIsValid(false)
{
}


ezMathExpression::ezMathExpression(const char* szExpressionString, ezLogInterface* pLog)
    : m_pLog(pLog)
{
  Reset(szExpressionString);
}

void ezMathExpression::Reset(const char* szExpressionString)
{
  m_OriginalExpression = szExpressionString;
  m_InstructionStream.Clear();
  m_Constants.Clear();
  m_bIsValid = false;

  if (ezStringUtils::IsNullOrEmpty(szExpressionString))
    return;

  ezTokenizer tokenizer;
  tokenizer.Tokenize(ezMakeArrayPtr<ezUInt8>(const_cast<ezUInt8*>(reinterpret_cast<const ezUInt8*>(m_OriginalExpression.GetData())), m_OriginalExpression.GetElementCount()), m_pLog);

  ezUInt32 readTokens = 0;
  TokenStream tokenStream;
  tokenizer.GetNextLine(readTokens, tokenStream);

  ezUInt32 curToken = 0;
  m_bIsValid = ParseExpression(tokenStream, curToken).Succeeded();
  if (curToken != readTokens)
  {
    ezLog::Error(m_pLog, "Did not parse the entire math expression. This can happen if the last recognized token is followed by an unknown token.");
    m_bIsValid = false;
  }

  tokenStream.Clear();
  if (tokenizer.GetNextLine(readTokens, tokenStream).Succeeded() && !tokenStream.IsEmpty())
    ezLog::Warning(m_pLog, "Only the first line of a math expression is parsed, all following lines are ignored!");
}

double ezMathExpression::Evaluate(const ezDelegate<double(const ezStringView&)>& variableResolveFunction) const
{
  static const double errorOutput = ezMath::NaN<double>();

  if (!IsValid() || m_InstructionStream.IsEmpty())
  {
    ezLog::Error(m_pLog, "Can't evaluate invalid math expression '{0}'", m_OriginalExpression);
    return errorOutput;
  }

  ezHybridArray<double, 8> evaluationStack;

  for (ezUInt32 instructionIdx = 0; instructionIdx < m_InstructionStream.GetCount(); ++instructionIdx)
  {
    InstructionType::Enum instruction = static_cast<InstructionType::Enum>(m_InstructionStream[instructionIdx]);

    switch (instruction)
    {
      // Binary operations.
      case InstructionType::Add:
      case InstructionType::Subtract:
      case InstructionType::Multiply:
      case InstructionType::Divide:
      {
        if (evaluationStack.GetCount() < 2)
        {
          ezLog::Error(m_pLog, "Expected at least two operands on evaluation stack during evaluation of '{0}'.", m_OriginalExpression);
          return errorOutput;
        }

        double operand1 = evaluationStack.PeekBack();
        evaluationStack.PopBack();
        double operand0 = evaluationStack.PeekBack();
        //evaluationStack.PopBack();    // Don't pop, just overwrite directly.

        switch (instruction)
        {
          case InstructionType::Add:
            evaluationStack.PeekBack() = operand0 + operand1;
            break;
          case InstructionType::Subtract:
            evaluationStack.PeekBack() = operand0 - operand1;
            break;
          case InstructionType::Multiply:
            evaluationStack.PeekBack() = operand0 * operand1;
            break;
          case InstructionType::Divide:
            evaluationStack.PeekBack() = operand0 / operand1;
            break;

          default:
            break;
        }
      }
      break;

      // Unary operations.
      case InstructionType::Negate:
      {
        if (evaluationStack.GetCount() < 1)
        {
          ezLog::Error(m_pLog, "Expected at least operands on evaluation stack during evaluation of '{0}'.", m_OriginalExpression);
          return errorOutput;
        }

        evaluationStack.PeekBack() = -evaluationStack.PeekBack();
      }
      break;

      // Push Constant.
      case InstructionType::PushConstant:
      {
        EZ_ASSERT_DEBUG(m_InstructionStream.GetCount() > instructionIdx + 1, "ezMathExpression::InstructionType::PushConstant should always be followed by another integer in the instruction stream.");

        ++instructionIdx;
        ezUInt32 constantIndex = m_InstructionStream[instructionIdx];
        evaluationStack.PushBack(m_Constants[constantIndex]); // constantIndex is not a user input, so it should be impossible for it to be outside!
      }
      break;

      // Push Variable.
      case InstructionType::PushVariable:
      {
        EZ_ASSERT_DEBUG(m_InstructionStream.GetCount() > instructionIdx + 2, "ezMathExpression::InstructionType::PushVariable should always be followed by two more integers in the instruction stream.");

        ezUInt32 variableSubstringStart = m_InstructionStream[instructionIdx + 1];
        ezUInt32 variableSubstringEnd = m_InstructionStream[instructionIdx + 2];
        instructionIdx += 2;

        ezStringView variableName(m_OriginalExpression + variableSubstringStart, m_OriginalExpression + variableSubstringEnd);
        double variable = variableResolveFunction(variableName);
        evaluationStack.PushBack(variable);
      }
      break;

      default:
        EZ_REPORT_FAILURE("Unknown instruction in MathExpression!");
    }
  }

  // There should be just a single value left on the evaluation stack now.
  if (evaluationStack.GetCount() == 1)
  {
    return evaluationStack.PeekBack();
  }
  else
  {
    ezLog::Error(m_pLog, "Evaluation of '{0}' yielded more than one value on the evaluation stack.", m_OriginalExpression);
    return errorOutput;
  }
}

namespace
{
  const int s_operatorPrecedence[] =
      {
          // Binary
          1, // Add
          1, // Subtract
          2, // Multiply
          2, // Divide

          // Unary
          2, // Negate
  };

  // Accept/parses binary operator.
  // Does NOT advance the current token beyond the binary operator!
  bool AcceptBinaryOperator(const ezTokenParseUtils::TokenStream& tokens, ezUInt32& uiCurToken, ezMathExpression::InstructionType::Enum& outOperator)
  {
    SkipWhitespace(tokens, uiCurToken);

    if (uiCurToken >= tokens.GetCount())
      return false;

    if (tokens[uiCurToken]->m_DataView.GetElementCount() != 1)
      return false;

    ezUInt32 operatorChar = tokens[uiCurToken]->m_DataView.GetCharacter();

    switch (operatorChar)
    {
      case '+':
        outOperator = ezMathExpression::InstructionType::Add;
        break;
      case '-':
        outOperator = ezMathExpression::InstructionType::Subtract;
        break;
      case '*':
        outOperator = ezMathExpression::InstructionType::Multiply;
        break;
      case '/':
        outOperator = ezMathExpression::InstructionType::Divide;
        break;

      default:
        return false;
    }

    return true;
  }
}

ezResult ezMathExpression::ParseExpression(const ezTokenParseUtils::TokenStream& tokens, ezUInt32& uiCurToken, int precedence)
{
  if (ParseFactor(tokens, uiCurToken).Failed())
    return EZ_FAILURE;

  InstructionType::Enum binaryOp;
  while (AcceptBinaryOperator(tokens, uiCurToken, binaryOp) && precedence < s_operatorPrecedence[binaryOp])
  {
    // Consume token.
    ++uiCurToken;

    // Parse second operand.
    if (ParseExpression(tokens, uiCurToken, s_operatorPrecedence[binaryOp]).Failed())
      return EZ_FAILURE;

    // Perform operation on previous two operands.
    m_InstructionStream.PushBack(binaryOp);
  }

  return EZ_SUCCESS;
}

ezResult ezMathExpression::ParseFactor(const TokenStream& tokens, ezUInt32& uiCurToken)
{
  // Consume unary operators
  {
    while (Accept(tokens, uiCurToken, "+"))
    {
    }

    if (Accept(tokens, uiCurToken, "-"))
    {
      if (ParseExpression(tokens, uiCurToken, s_operatorPrecedence[InstructionType::Negate]).Failed())
        return EZ_FAILURE;

      m_InstructionStream.PushBack(InstructionType::Negate);
      return EZ_SUCCESS;
    }
  }

  // Consume constant or variable.
  ezUInt32 uiValueToken = uiCurToken;
  if (Accept(tokens, uiCurToken, ezTokenType::Integer) || Accept(tokens, uiCurToken, ezTokenType::Float))
  {
    const ezString sVal = tokens[uiValueToken]->m_DataView;

    double fConstant = 0;
    ezConversionUtils::StringToFloat(sVal, fConstant);
    
    m_InstructionStream.PushBack(InstructionType::PushConstant);
    m_InstructionStream.PushBack(m_Constants.GetCount());
    m_Constants.PushBack(fConstant);

    return EZ_SUCCESS;
  }
  else if (Accept(tokens, uiCurToken, ezTokenType::Identifier, &uiValueToken))
  {
    const ezString sVal = tokens[uiValueToken]->m_DataView;

    // Check if it really qualifies as variable!
    for (auto varChar : sVal)
    {
      const char* validChar = s_szValidVariableCharacters;
      for (; *validChar != '\0'; ++validChar)
      {
        if (*validChar == varChar)
          break;
      }
      if (*validChar == '\0') // Walked to the end, so the varChar was not any of the valid chars!
      {
        ezLog::Error(m_pLog, "Invalid character {0} in variable name: {1}", varChar, sVal);
        return EZ_FAILURE;
      }
    }

    m_InstructionStream.PushBack(InstructionType::PushVariable);
    m_InstructionStream.PushBack(tokens[uiValueToken]->m_uiColumn - 1);
    m_InstructionStream.PushBack(tokens[uiValueToken]->m_uiColumn - 1 + sVal.GetElementCount());

    return EZ_SUCCESS;
  }
  // Consume parenthesis.
  else if (Accept(tokens, uiCurToken, "("))
  {
    // A new expression!
    ParseExpression(tokens, uiCurToken);

    if (!Accept(tokens, uiCurToken, ")"))
    {
      if (uiCurToken >= tokens.GetCount())
      {
        ezLog::Error(m_pLog, "Syntax error, expected ')' after token '{0}' in column {1}.", tokens.PeekBack()->m_DataView, tokens.PeekBack()->m_uiColumn);
        return EZ_FAILURE;
      }

      ezLog::Error(m_pLog, "Syntax error, expected ')' after token '{0}' in column {1}.", tokens[uiCurToken]->m_DataView, tokens[uiCurToken]->m_uiColumn);
      return EZ_FAILURE;
    }
    else
      return EZ_SUCCESS;
  }

  if (uiCurToken >= tokens.GetCount())
  {
    ezLog::Error(m_pLog, "Syntax error, unexpected end of expression after token '{0}' in column {1}.", tokens.PeekBack()->m_DataView, tokens.PeekBack()->m_uiColumn);
    return EZ_FAILURE;
  }

  ezLog::Error(m_pLog, "Syntax error, expected identifier, number or '(' after token '{0}' in column {1}.", tokens[uiCurToken]->m_DataView, tokens[uiCurToken]->m_uiColumn);
  return EZ_FAILURE;
}

EZ_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_MathExpression);


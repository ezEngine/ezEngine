#include <PCH.h>
#include <Foundation/CodeUtils/MathExpression.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Utilities/ConversionUtils.h>

using namespace ezTokenParseUtils;

const char* ezMathExpression::s_szValidVariableCharacters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789";

ezMathExpression::ezMathExpression(const char* expressionString, ezLogInterface* log)
  : m_OriginalExpression(expressionString)
  , m_Log(log ? log : ezLog::GetThreadLocalLogSystem())
{
  ezTokenizer tokenizer;
  tokenizer.Tokenize(ezMakeArrayPtr<ezUInt8>(const_cast<ezUInt8*>(reinterpret_cast<const ezUInt8*>(m_OriginalExpression.GetData())), m_OriginalExpression.GetElementCount()), m_Log);

  ezUInt32 readTokens = 0;
  TokenStream tokenStream;
  tokenizer.GetNextLine(readTokens, tokenStream);

  ezUInt32 curToken = 0;
  m_bIsValid = ParseExpression(tokenStream, curToken).Succeeded();
  if (curToken != readTokens)
  {
    ezLog::Error(m_Log, "Did not parse the entire math expression. This can happen if the last recognized token is followed by an unknown token.");
    m_bIsValid = false;
  }


  tokenStream.Clear();
  if (tokenizer.GetNextLine(readTokens, tokenStream).Succeeded() && !tokenStream.IsEmpty())
    ezLog::Warning(m_Log, "Only the first line of a math expression is parsed, all following lines are ignored!");
}

double ezMathExpression::Evaluate(const ezDelegate<double(const ezStringView&)>& variableResolveFunction) const
{
  static const double errorOutput = ezMath::BasicType<double>::GetNaN();

  if (!IsValid())
  {
    ezLog::Error(m_Log, "Can't evaluate invalid math expression '{0}'", m_OriginalExpression);
    return errorOutput;
  }

  ezHybridArray<double, 8> evaluationStack;

  for (ezUInt32 instructionIdx=0; instructionIdx < m_InstructionStream.GetCount(); ++instructionIdx)
  {
    ezUInt32 instructionInt = m_InstructionStream[instructionIdx];
    InstructionType instruction = static_cast<InstructionType>(m_InstructionStream[instructionIdx]);

    // Binary operation.
    if (instructionInt <= static_cast<ezUInt32>(InstructionType::Divide))
    {
      if(evaluationStack.GetCount() < 2)
      {
        ezLog::Error(m_Log, "Expected at least two operands on evaluation stack during evaluation of '{0}'.", m_OriginalExpression);
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
      }
    }

    // Unary operation
    else if(instruction == InstructionType::Negate)
    {
      if (evaluationStack.GetCount() < 1)
      {
        ezLog::Error(m_Log, "Expected at least operands on evaluation stack during evaluation of '{0}'.", m_OriginalExpression);
        return errorOutput;
      }

      evaluationStack.PeekBack() = -evaluationStack.PeekBack();
    }

    // Push Constant
    else if(instruction == InstructionType::PushConstant)
    {
      EZ_ASSERT_DEBUG(m_InstructionStream.GetCount() > instructionIdx + 1, "ezMathExpression::InstructionType::PushConstant should always be followed by another integer in the instruction stream.");

      ++instructionIdx;
      ezUInt32 constantIndex = m_InstructionStream[instructionIdx];
      evaluationStack.PushBack(m_Constants[constantIndex]); // constantIndex is not a user input, so it should be impossible for it to be outside!
    }

    // Push Variable
    else if (instruction == InstructionType::PushVariable)
    {
      EZ_ASSERT_DEBUG(m_InstructionStream.GetCount() > instructionIdx + 2, "ezMathExpression::InstructionType::PushVariable should always be followed by two more integers in the instruction stream.");

      ezUInt32 variableSubstringStart = m_InstructionStream[instructionIdx+1];
      ezUInt32 variableSubstringEnd = m_InstructionStream[instructionIdx+2];
      instructionIdx += 2;

      ezStringView variableName(m_OriginalExpression + variableSubstringStart, m_OriginalExpression + variableSubstringEnd);
      double variable = variableResolveFunction(variableName);
      evaluationStack.PushBack(variable);
    }

    // Unknown
    else
    {
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
    ezLog::Error(m_Log, "Evaluation of '{0}' yielded more than one value on the evaluation stack.", m_OriginalExpression);
    return errorOutput;
  }
}

ezResult ezMathExpression::ParseExpressionPlus(const TokenStream& tokens, ezUInt32& uiCurToken)
{
  if (ParseExpressionMul(tokens, uiCurToken).Failed())
    return EZ_FAILURE;

  while (true)
  {
    if (Accept(tokens, uiCurToken, "+"))
    {
      ezInt64 iNextValue = 0;
      if (ParseExpressionMul(tokens, uiCurToken).Failed())
        return EZ_FAILURE;

      m_InstructionStream.PushBack(static_cast<ezUInt32>(InstructionType::Add));
    }
    else if (Accept(tokens, uiCurToken, "-"))
    {
      ezInt64 iNextValue = 0;
      if (ParseExpressionMul(tokens, uiCurToken).Failed())
        return EZ_FAILURE;

      m_InstructionStream.PushBack(static_cast<ezUInt32>(InstructionType::Subtract));
    }
    else
      break;
  }

  return EZ_SUCCESS;
}

ezResult ezMathExpression::ParseExpressionMul(const TokenStream& tokens, ezUInt32& uiCurToken)
{
  if (ParseFactor(tokens, uiCurToken).Failed())
    return EZ_FAILURE;

  while (true)
  {
    if (Accept(tokens, uiCurToken, "*"))
    {
      if (ParseFactor(tokens, uiCurToken).Failed())
        return EZ_FAILURE;

      m_InstructionStream.PushBack(static_cast<ezUInt32>(InstructionType::Multiply));
    }
    else if (Accept(tokens, uiCurToken, "/"))
    {
      if (ParseFactor(tokens, uiCurToken).Failed())
        return EZ_FAILURE;

      m_InstructionStream.PushBack(static_cast<ezUInt32>(InstructionType::Divide));
    }
    else
      break;
  }

  return EZ_SUCCESS;
}

ezResult ezMathExpression::ParseFactor(const TokenStream& tokens, ezUInt32& uiCurToken)
{
  while (Accept(tokens, uiCurToken, "+"))
  {
  }

  if (Accept(tokens, uiCurToken, "-"))
  {
    if (ParseFactor(tokens, uiCurToken).Failed())
      return EZ_FAILURE;

    m_InstructionStream.PushBack(static_cast<ezUInt32>(InstructionType::Negate));
    return EZ_SUCCESS;
  }

  ezUInt32 uiValueToken = uiCurToken;
  if (Accept(tokens, uiCurToken, ezTokenType::Identifier, &uiValueToken))
  {
    const ezString sVal = tokens[uiValueToken]->m_DataView;

    double fConstant = 0;

    // It's either a double or a variable
    if (ezConversionUtils::StringToFloat(sVal.GetData(), fConstant).Succeeded())
    {
      m_InstructionStream.PushBack(static_cast<ezUInt32>(InstructionType::PushConstant));
      m_InstructionStream.PushBack(m_Constants.GetCount());
      m_Constants.PushBack(fConstant);
    }
    else
    {
      // Check if it really qualifies as variable!
      for (char varChar : sVal)
      {
        const char* validChar = s_szValidVariableCharacters;
        for (; *validChar != '\0'; ++validChar)
        {
          if (*validChar == varChar)
            break;
        }
        if (*validChar == '\0')  // Walked to the end, so the varChar was not any of the valid chars!
        {
          ezLog::Error(m_Log, "Invalid character {1} in variable name: {0}", sVal, varChar);
          return EZ_FAILURE;
        }
      }

      m_InstructionStream.PushBack(static_cast<ezUInt32>(InstructionType::PushVariable));
      m_InstructionStream.PushBack(tokens[uiValueToken]->m_uiColumn - 1);
      m_InstructionStream.PushBack(tokens[uiValueToken]->m_uiColumn - 1 + sVal.GetElementCount());
    }

    return EZ_SUCCESS;
  }
  else if (Accept(tokens, uiCurToken, "("))
  {
    // A new expression!
    ParseExpression(tokens, uiCurToken);

    if (!Accept(tokens, uiCurToken, ")"))
    {
      ezLog::Error(m_Log, "Syntax error, ')' after token '{0}' in column {0}.", tokens[uiCurToken - 1]->m_DataView, tokens[uiCurToken - 1]->m_uiColumn);
      return EZ_FAILURE;
    }
    else
      return EZ_SUCCESS;
  }

  ezLog::Error(m_Log, "Syntax error, expected identifier, number or '(' after token '{0}' in column {0}.", tokens[uiCurToken - 1]->m_DataView, tokens[uiCurToken - 1]->m_uiColumn);
  return EZ_FAILURE;
}

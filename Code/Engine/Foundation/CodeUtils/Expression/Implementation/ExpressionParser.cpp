#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Tokenizer.h>

using namespace ezTokenParseUtils;

ezExpressionParser::ezExpressionParser() = default;
ezExpressionParser::~ezExpressionParser() = default;

ezResult ezExpressionParser::Parse(ezStringView code, ezArrayPtr<Stream> inputs, ezArrayPtr<Stream> outputs, ezExpressionAST& out_ast)
{
  if (code.IsEmpty())
    return EZ_FAILURE;

  m_pAST = &out_ast;
  SetupInAndOutputs(inputs, outputs);

  ezTokenizer tokenizer;
  tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)code.GetStartPointer(), code.GetElementCount()), ezLog::GetThreadLocalLogSystem());

  ezUInt32 readTokens = 0;
  while (tokenizer.GetNextLine(readTokens, m_TokenStream).Succeeded())
  {
    m_uiCurrentToken = 0;
    EZ_SUCCEED_OR_RETURN(ParseAssignment());
  }

  EZ_SUCCEED_OR_RETURN(CheckOutputs());

  return EZ_SUCCESS;
}

void ezExpressionParser::SetupInAndOutputs(ezArrayPtr<Stream> inputs, ezArrayPtr<Stream> outputs)
{
  for (auto& input : inputs)
  {
    auto pInputNode = m_pAST->CreateInput(input.m_sName, input.m_DataType);
    m_KnownVariables.Insert(input.m_sName, pInputNode);
  }

  for (auto& output : outputs)
  {
    auto pOutputNode = m_pAST->CreateOutput(output.m_sName, output.m_DataType, nullptr);
    m_KnownVariables.Insert(output.m_sName, pOutputNode);

    m_pAST->m_OutputNodes.PushBack(pOutputNode);
  }
}

ezResult ezExpressionParser::ParseAssignment()
{
  const ezToken* pIdentifierToken = nullptr;
  EZ_SUCCEED_OR_RETURN(Expect(ezTokenType::Identifier, &pIdentifierToken));

  const ezStringView sIdentifier = pIdentifierToken->m_DataView;
  ezExpressionAST::Node* pVarNode = GetVariable(sIdentifier);
  if (pVarNode == nullptr)
  {
    ReportError(m_TokenStream[m_uiCurrentToken], "Syntax error, expected a valid variable");
    return EZ_FAILURE;
  }

  EZ_SUCCEED_OR_RETURN(Expect("="));
  ezExpressionAST::Node* pExpression = ParseExpression();

  if (ezExpressionAST::NodeType::IsOutput(pVarNode->m_Type))
  {
    auto pOutput = static_cast<ezExpressionAST::Output*>(pVarNode);
    pOutput->m_pExpression = pExpression;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezExpressionAST::Node* ezExpressionParser::ParseFactor()
{
  ezUInt32 uiIdentifierToken = 0;
  if (Accept(m_TokenStream, m_uiCurrentToken, ezTokenType::Identifier, &uiIdentifierToken))
  {
    auto pIdentifierToken = m_TokenStream[uiIdentifierToken];
    const ezStringView sIdentifier = pIdentifierToken->m_DataView;

    if (Accept(m_TokenStream, m_uiCurrentToken, "("))
    {
      // function call
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
    else
    {
      return GetVariable(sIdentifier);
    }
  }

  ezUInt32 uiValueToken = 0;
  if (Accept(m_TokenStream, m_uiCurrentToken, ezTokenType::Integer, &uiValueToken) ||
      Accept(m_TokenStream, m_uiCurrentToken, ezTokenType::Float, &uiValueToken))
  {
    const ezString sVal = m_TokenStream[uiValueToken]->m_DataView;

    double fConstant = 0;
    ezConversionUtils::StringToFloat(sVal, fConstant).IgnoreResult();

    return m_pAST->CreateConstant((float)fConstant);
  }

  if (Accept(m_TokenStream, m_uiCurrentToken, "("))
  {
    auto pExpression = ParseExpression();
    if (Expect(")").Failed())
      return nullptr;

    return pExpression;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

// Parsing the expression - recursive parser using "precedence climbing".
// http://www.engr.mun.ca/~theo/Misc/exp_parsing.htm
ezExpressionAST::Node* ezExpressionParser::ParseExpression(int iPrecedence /*= 0*/)
{
  auto pExpression = ParseUnaryExpression();
  if (pExpression == nullptr)
    return nullptr;

  ezExpressionAST::NodeType::Enum binaryOp;
  int iBinaryOpPrecedence = 0;
  while (AcceptBinaryOperator(binaryOp, iBinaryOpPrecedence) && iBinaryOpPrecedence >= iPrecedence)
  {
    // Consume token.
    ++m_uiCurrentToken;

    auto pRightOperand = ParseExpression(iBinaryOpPrecedence);
    if (pRightOperand == nullptr)
      return nullptr;

    pExpression = m_pAST->CreateBinaryOperator(binaryOp, pExpression, pRightOperand);
  }

  return pExpression;
}

ezExpressionAST::Node* ezExpressionParser::ParseUnaryExpression()
{
  while (Accept(m_TokenStream, m_uiCurrentToken, "+"))
  {
  }

  if (Accept(m_TokenStream, m_uiCurrentToken, "-"))
  {
    auto pOperand = ParseUnaryExpression();
    return m_pAST->CreateUnaryOperator(ezExpressionAST::NodeType::Negate, pOperand);
  }

  return ParseFactor();
}

// Does NOT advance the current token beyond the binary operator!
// Operator precedence according to https://en.cppreference.com/w/cpp/language/operator_precedence,
// but inverted so that higher values mean higher precedence
static constexpr int s_iMaxPrecedence = 20;

bool ezExpressionParser::AcceptBinaryOperator(ezExpressionAST::NodeType::Enum& out_binaryOp, int& out_iOperatorPrecedence)
{
  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  if (m_uiCurrentToken >= m_TokenStream.GetCount())
    return false;

  auto pCurrentToken = m_TokenStream[m_uiCurrentToken];
  if (pCurrentToken->m_DataView.GetElementCount() != 1)
    return false;

  ezUInt32 operatorChar = pCurrentToken->m_DataView.GetCharacter();

  switch (operatorChar)
  {
    case '+':
      out_binaryOp = ezExpressionAST::NodeType::Add;
      out_iOperatorPrecedence = s_iMaxPrecedence - 6;
      break;
    case '-':
      out_binaryOp = ezExpressionAST::NodeType::Subtract;
      out_iOperatorPrecedence = s_iMaxPrecedence - 6;
      break;
    case '*':
      out_binaryOp = ezExpressionAST::NodeType::Multiply;
      out_iOperatorPrecedence = s_iMaxPrecedence - 5;
      break;
    case '/':
      out_binaryOp = ezExpressionAST::NodeType::Divide;
      out_iOperatorPrecedence = s_iMaxPrecedence - 5;
      break;
      // Currently not supported
      /*case '%':
      out_binaryOp = ezExpressionAST::NodeType::Modulo;
      out_iOperatorPrecedence = s_iMaxPrecedence-5;
      break;*/

    default:
      return false;
  }

  return true;
}

ezExpressionAST::Node* ezExpressionParser::GetVariable(ezStringView sVarName)
{
  ezHashedString sHashedVarName;
  sHashedVarName.Assign(sVarName);

  ezExpressionAST::Node* pNode = nullptr;
  m_KnownVariables.TryGetValue(sHashedVarName, pNode);

  return pNode;
}

ezResult ezExpressionParser::CheckOutputs()
{
  for (auto pOutputNode : m_pAST->m_OutputNodes)
  {
    if (pOutputNode->m_pExpression == nullptr)
    {
      ezLog::Error("Output '{}' was never written", pOutputNode->m_sName);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

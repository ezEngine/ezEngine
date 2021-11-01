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
  ezExpressionAST::Node* pVarNode = ParseVariable();
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

ezExpressionAST::Node* ezExpressionParser::ParseVariable()
{
  ezUInt32 uiVarToken = 0;
  if (Accept(m_TokenStream, m_uiCurrentToken, ezTokenType::Identifier, &uiVarToken) == false)
    return nullptr;

  auto pVarToken = m_TokenStream[uiVarToken];
  const ezStringView sVarName = pVarToken->m_DataView;
  if (ezStringUtils::IsValidIdentifierName(sVarName.GetStartPointer(), sVarName.GetEndPointer()) == false)
  {
    ReportError(pVarToken, ezFmt("Syntax error, '{}' is not a valid identifier", pVarToken->m_DataView));
    return nullptr;
  }

  ezHashedString sHashedVarName;
  sHashedVarName.Assign(sVarName);

  ezExpressionAST::Node* pNode = nullptr;
  m_KnownVariables.TryGetValue(sHashedVarName, pNode);

  return pNode;
}

ezExpressionAST::Node* ezExpressionParser::ParseExpression(int iPrecedence /*= 0*/)
{
  // TODO
  return m_pAST->CreateConstant(0.5f);
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

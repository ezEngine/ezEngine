#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Tokenizer.h>

using namespace ezTokenParseUtils;

ezExpressionParser::ezExpressionParser()
{
  RegisterBuiltinFunctions();
}

ezExpressionParser::~ezExpressionParser() = default;

ezResult ezExpressionParser::Parse(ezStringView code, ezArrayPtr<Stream> inputs, ezArrayPtr<Stream> outputs, const Options& options, ezExpressionAST& out_ast)
{
  if (code.IsEmpty())
    return EZ_FAILURE;

  m_Options = options;

  m_pAST = &out_ast;
  SetupInAndOutputs(inputs, outputs);

  ezTokenizer tokenizer;
  tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)code.GetStartPointer(), code.GetElementCount()), ezLog::GetThreadLocalLogSystem());

  ezUInt32 readTokens = 0;
  while (tokenizer.GetNextLine(readTokens, m_TokenStream).Succeeded())
  {
    m_uiCurrentToken = 0;

    while (m_uiCurrentToken < readTokens)
    {
      EZ_SUCCEED_OR_RETURN(ParseStatement());

      if (m_uiCurrentToken < readTokens && Accept(m_TokenStream, m_uiCurrentToken, ";") == false)
      {
        auto pCurrentToken = m_TokenStream[m_uiCurrentToken];
        ReportError(pCurrentToken, ezFmt("Syntax error, unexpected token '{}'", pCurrentToken->m_DataView));
        return EZ_FAILURE;
      }
    }
  }

  EZ_SUCCEED_OR_RETURN(CheckOutputs());

  return EZ_SUCCESS;
}

void ezExpressionParser::RegisterBuiltinFunctions()
{
  // Unary
  m_BuiltinFunctions.Insert(ezMakeHashedString("abs"), ezExpressionAST::NodeType::Absolute);
  m_BuiltinFunctions.Insert(ezMakeHashedString("sqrt"), ezExpressionAST::NodeType::Sqrt);
  m_BuiltinFunctions.Insert(ezMakeHashedString("sin"), ezExpressionAST::NodeType::Sin);
  m_BuiltinFunctions.Insert(ezMakeHashedString("cos"), ezExpressionAST::NodeType::Cos);
  m_BuiltinFunctions.Insert(ezMakeHashedString("tan"), ezExpressionAST::NodeType::Tan);
  m_BuiltinFunctions.Insert(ezMakeHashedString("asin"), ezExpressionAST::NodeType::ASin);
  m_BuiltinFunctions.Insert(ezMakeHashedString("acos"), ezExpressionAST::NodeType::ACos);
  m_BuiltinFunctions.Insert(ezMakeHashedString("atan"), ezExpressionAST::NodeType::ATan);

  //Binary
  m_BuiltinFunctions.Insert(ezMakeHashedString("min"), ezExpressionAST::NodeType::Min);
  m_BuiltinFunctions.Insert(ezMakeHashedString("max"), ezExpressionAST::NodeType::Max);
}

void ezExpressionParser::SetupInAndOutputs(ezArrayPtr<Stream> inputs, ezArrayPtr<Stream> outputs)
{
  m_KnownVariables.Clear();

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

ezResult ezExpressionParser::ParseStatement()
{
  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  if (m_uiCurrentToken >= m_TokenStream.GetCount())
    return EZ_FAILURE;

  const ezToken* pIdentifierToken = m_TokenStream[m_uiCurrentToken];
  if (pIdentifierToken->m_iType != ezTokenType::Identifier)
  {
    ReportError(pIdentifierToken, "Syntax error, expected type or variable");
  }

  if (ParseType(pIdentifierToken->m_DataView).Succeeded())
  {
    return ParseVariableDefinition();
  }

  return ParseAssignment();
}

ezResult ezExpressionParser::ParseType(ezStringView sTypeName)
{
  if (sTypeName == "var" || sTypeName == "float")
  {
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezExpressionParser::ParseVariableDefinition()
{
  // skip type
  EZ_SUCCEED_OR_RETURN(Expect(ezTokenType::Identifier));

  const ezToken* pIdentifierToken = nullptr;
  EZ_SUCCEED_OR_RETURN(Expect(ezTokenType::Identifier, &pIdentifierToken));

  ezHashedString sHashedVarName;
  sHashedVarName.Assign(pIdentifierToken->m_DataView);

  ezExpressionAST::Node* pNode = nullptr;
  if (m_KnownVariables.TryGetValue(sHashedVarName, pNode))
  {
    const char* szExisting = "a variable";
    if (ezExpressionAST::NodeType::IsInput(pNode->m_Type))
    {
      szExisting = "an input";
    }
    else if (ezExpressionAST::NodeType::IsOutput(pNode->m_Type))
    {
      szExisting = "an output";
    }

    ReportError(pIdentifierToken, ezFmt("Local variable '{}' cannot be defined because {} of the same name already exists", pIdentifierToken->m_DataView, szExisting));
    return EZ_FAILURE;
  }

  EZ_SUCCEED_OR_RETURN(Expect("="));
  ezExpressionAST::Node* pExpression = ParseExpression();
  if (pExpression == nullptr)
    return EZ_FAILURE;

  m_KnownVariables.Insert(sHashedVarName, pExpression);
  return EZ_SUCCESS;
}

ezResult ezExpressionParser::ParseAssignment()
{
  const ezToken* pIdentifierToken = nullptr;
  EZ_SUCCEED_OR_RETURN(Expect(ezTokenType::Identifier, &pIdentifierToken));

  const ezStringView sIdentifier = pIdentifierToken->m_DataView;
  ezExpressionAST::Node* pVarNode = GetVariable(sIdentifier);
  if (pVarNode == nullptr)
  {
    ReportError(pIdentifierToken, "Syntax error, expected a valid variable");
    return EZ_FAILURE;
  }

  ezExpressionAST::NodeType::Enum assignOperator = ezExpressionAST::NodeType::Invalid;
  if (Accept(m_TokenStream, m_uiCurrentToken, "+", "="))
  {
    assignOperator = ezExpressionAST::NodeType::Add;
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "-", "="))
  {
    assignOperator = ezExpressionAST::NodeType::Subtract;
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "*", "="))
  {
    assignOperator = ezExpressionAST::NodeType::Multiply;
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "/", "="))
  {
    assignOperator = ezExpressionAST::NodeType::Divide;
  }
  else
  {
    EZ_SUCCEED_OR_RETURN(Expect("="));
  }
  
  ezExpressionAST::Node* pExpression = ParseExpression();
  if (pExpression == nullptr)
    return EZ_FAILURE;

  if (assignOperator != ezExpressionAST::NodeType::Invalid)
  {
    pExpression = m_pAST->CreateBinaryOperator(assignOperator, pVarNode, pExpression);
  }

  if (ezExpressionAST::NodeType::IsInput(pVarNode->m_Type))
  {
    ReportError(pIdentifierToken, ezFmt("Input '{}' is not assignable", sIdentifier));
    return EZ_FAILURE;
  }
  else if (ezExpressionAST::NodeType::IsOutput(pVarNode->m_Type))
  {
    auto pOutput = static_cast<ezExpressionAST::Output*>(pVarNode);
    pOutput->m_pExpression = pExpression;
    return EZ_SUCCESS;
  }

  ezHashedString sHashedVarName;
  sHashedVarName.Assign(sIdentifier);
  m_KnownVariables.Insert(sHashedVarName, pExpression);
  return EZ_SUCCESS;
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
      return ParseFunctionCall(sIdentifier);
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

  return nullptr;
}

// Parsing the expression - recursive parser using "precedence climbing".
// http://www.engr.mun.ca/~theo/Misc/exp_parsing.htm
ezExpressionAST::Node* ezExpressionParser::ParseExpression(int iPrecedence /* = s_iLowestPrecedence*/)
{
  auto pExpression = ParseUnaryExpression();
  if (pExpression == nullptr)
    return nullptr;

  ezExpressionAST::NodeType::Enum binaryOp;
  int iBinaryOpPrecedence = 0;
  while (AcceptBinaryOperator(binaryOp, iBinaryOpPrecedence) && iBinaryOpPrecedence < iPrecedence)
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

ezExpressionAST::Node* ezExpressionParser::ParseFunctionCall(ezStringView sFunctionName)
{
  // "(" of the function call
  const ezToken* pFunctionToken = m_TokenStream[m_uiCurrentToken - 1];

  ezHybridArray<ezExpressionAST::Node*, 8> arguments;
  if (Accept(m_TokenStream, m_uiCurrentToken, ")") == false)
  {
    do
    {
      arguments.PushBack(ParseExpression());
    } while (Accept(m_TokenStream, m_uiCurrentToken, ","));
  }
  if (Expect(")").Failed())
    return nullptr;

  ezHashedString sHashedFuncName;
  sHashedFuncName.Assign(sFunctionName);

  ezEnum<ezExpressionAST::NodeType> builtinType;
  if (m_BuiltinFunctions.TryGetValue(sHashedFuncName, builtinType))
  {
    if (ezExpressionAST::NodeType::IsUnary(builtinType))
    {
      if (arguments.GetCount() != 1)
      {
        ReportError(pFunctionToken, ezFmt("Invalid argument count for '{}'. Expected 1 but got {}", sFunctionName, arguments.GetCount()));
        return nullptr;
      }
      return m_pAST->CreateUnaryOperator(builtinType, arguments[0]);
    }
    else if (ezExpressionAST::NodeType::IsBinary(builtinType))
    {
      if (arguments.GetCount() != 2)
      {
        ReportError(pFunctionToken, ezFmt("Invalid argument count for '{}'. Expected 2 but got {}", sFunctionName, arguments.GetCount()));
        return nullptr;
      }
      return m_pAST->CreateBinaryOperator(builtinType, arguments[0], arguments[1]);
    }

    EZ_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }

  // external function
  auto pFunctionCall = m_pAST->CreateFunctionCall(sHashedFuncName);
  pFunctionCall->m_Arguments = std::move(arguments);
  return pFunctionCall;
}

// Does NOT advance the current token beyond the binary operator!
// Operator precedence according to https://en.cppreference.com/w/cpp/language/operator_precedence,
// lower value means higher precedence
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
      out_iOperatorPrecedence = 6;
      break;
    case '-':
      out_binaryOp = ezExpressionAST::NodeType::Subtract;
      out_iOperatorPrecedence = 6;
      break;
    case '*':
      out_binaryOp = ezExpressionAST::NodeType::Multiply;
      out_iOperatorPrecedence = 5;
      break;
    case '/':
      out_binaryOp = ezExpressionAST::NodeType::Divide;
      out_iOperatorPrecedence = 5;
      break;
      // Currently not supported
      /*case '%':
      out_binaryOp = ezExpressionAST::NodeType::Modulo;
      out_iOperatorPrecedence = 5;
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
  if (m_KnownVariables.TryGetValue(sHashedVarName, pNode) == false && m_Options.m_bTreatUnknownVariablesAsInputs)
  {
    pNode = m_pAST->CreateInput(sHashedVarName, ezProcessingStream::DataType::Float);
    m_KnownVariables.Insert(sHashedVarName, pNode);
  }

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

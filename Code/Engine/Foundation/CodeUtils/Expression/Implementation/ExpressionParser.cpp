#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Tokenizer.h>

namespace
{
  struct AssignOperator
  {
    ezStringView m_sName;
    ezExpressionAST::NodeType::Enum m_NodeType;
  };

  static constexpr AssignOperator s_assignOperators[] = {
    {"+="_ezsv, ezExpressionAST::NodeType::Add},
    {"-="_ezsv, ezExpressionAST::NodeType::Subtract},
    {"*="_ezsv, ezExpressionAST::NodeType::Multiply},
    {"/="_ezsv, ezExpressionAST::NodeType::Divide},
    {"%="_ezsv, ezExpressionAST::NodeType::Modulo},
    {"<<="_ezsv, ezExpressionAST::NodeType::BitshiftLeft},
    {">>="_ezsv, ezExpressionAST::NodeType::BitshiftRight},
    {"&="_ezsv, ezExpressionAST::NodeType::BitwiseAnd},
    {"^="_ezsv, ezExpressionAST::NodeType::BitwiseXor},
    {"|="_ezsv, ezExpressionAST::NodeType::BitwiseOr},
  };

  struct BinaryOperator
  {
    ezStringView m_sName;
    ezExpressionAST::NodeType::Enum m_NodeType;
    int m_iPrecedence;
  };

  // Operator precedence according to https://en.cppreference.com/w/cpp/language/operator_precedence,
  // lower value means higher precedence
  // sorted by string length to simplify the test against a token stream
  static constexpr BinaryOperator s_binaryOperators[] = {
    {"&&"_ezsv, ezExpressionAST::NodeType::LogicalAnd, 14},
    {"||"_ezsv, ezExpressionAST::NodeType::LogicalOr, 15},
    {"<<"_ezsv, ezExpressionAST::NodeType::BitshiftLeft, 7},
    {">>"_ezsv, ezExpressionAST::NodeType::BitshiftRight, 7},
    {"=="_ezsv, ezExpressionAST::NodeType::Equal, 10},
    {"!="_ezsv, ezExpressionAST::NodeType::NotEqual, 10},
    {"<="_ezsv, ezExpressionAST::NodeType::LessEqual, 9},
    {">="_ezsv, ezExpressionAST::NodeType::GreaterEqual, 9},
    {"<"_ezsv, ezExpressionAST::NodeType::Less, 9},
    {">"_ezsv, ezExpressionAST::NodeType::Greater, 9},
    {"&"_ezsv, ezExpressionAST::NodeType::BitwiseAnd, 11},
    {"^"_ezsv, ezExpressionAST::NodeType::BitwiseXor, 12},
    {"|"_ezsv, ezExpressionAST::NodeType::BitwiseOr, 13},
    {"?"_ezsv, ezExpressionAST::NodeType::Select, 16},
    {"+"_ezsv, ezExpressionAST::NodeType::Add, 6},
    {"-"_ezsv, ezExpressionAST::NodeType::Subtract, 6},
    {"*"_ezsv, ezExpressionAST::NodeType::Multiply, 5},
    {"/"_ezsv, ezExpressionAST::NodeType::Divide, 5},
    {"%"_ezsv, ezExpressionAST::NodeType::Modulo, 5},
  };

  static ezHashTable<ezHashedString, ezEnum<ezExpressionAST::DataType>> s_KnownTypes;
  static ezHashTable<ezHashedString, ezEnum<ezExpressionAST::NodeType>> s_BuiltinFunctions;

} // namespace

using namespace ezTokenParseUtils;

ezExpressionParser::ezExpressionParser()
{
  RegisterKnownTypes();
  RegisterBuiltinFunctions();
}

ezExpressionParser::~ezExpressionParser() = default;

// static
const ezHashTable<ezHashedString, ezEnum<ezExpressionAST::DataType>>& ezExpressionParser::GetKnownTypes()
{
  RegisterKnownTypes();

  return s_KnownTypes;
}

// static
const ezHashTable<ezHashedString, ezEnum<ezExpressionAST::NodeType>>& ezExpressionParser::GetBuiltinFunctions()
{
  RegisterBuiltinFunctions();

  return s_BuiltinFunctions;
}

void ezExpressionParser::RegisterFunction(const ezExpression::FunctionDesc& funcDesc)
{
  EZ_ASSERT_DEV(funcDesc.m_uiNumRequiredInputs <= funcDesc.m_InputTypes.GetCount(), "Not enough input types defined. {} inputs are required but only {} types given.", funcDesc.m_uiNumRequiredInputs, funcDesc.m_InputTypes.GetCount());

  auto& functionDescs = m_FunctionDescs[funcDesc.m_sName];
  if (functionDescs.Contains(funcDesc) == false)
  {
    functionDescs.PushBack(funcDesc);
  }
}

void ezExpressionParser::UnregisterFunction(const ezExpression::FunctionDesc& funcDesc)
{
  if (auto pFunctionDescs = m_FunctionDescs.GetValue(funcDesc.m_sName))
  {
    pFunctionDescs->RemoveAndCopy(funcDesc);
  }
}

ezResult ezExpressionParser::Parse(ezStringView sCode, ezArrayPtr<ezExpression::StreamDesc> inputs, ezArrayPtr<ezExpression::StreamDesc> outputs, const Options& options, ezExpressionAST& out_ast)
{
  if (sCode.IsEmpty())
    return EZ_FAILURE;

  m_Options = options;

  m_pAST = &out_ast;
  SetupInAndOutputs(inputs, outputs);

  ezTokenizer tokenizer;
  tokenizer.Tokenize(ezArrayPtr<const ezUInt8>((const ezUInt8*)sCode.GetStartPointer(), sCode.GetElementCount()), ezLog::GetThreadLocalLogSystem());

  ezUInt32 readTokens = 0;
  while (tokenizer.GetNextLine(readTokens, m_TokenStream).Succeeded())
  {
    m_uiCurrentToken = 0;

    while (m_uiCurrentToken < m_TokenStream.GetCount())
    {
      EZ_SUCCEED_OR_RETURN(ParseStatement());

      if (m_uiCurrentToken < m_TokenStream.GetCount() && AcceptStatementTerminator() == false)
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

// static
void ezExpressionParser::RegisterKnownTypes()
{
  if (s_KnownTypes.IsEmpty() == false)
    return;

  s_KnownTypes.Insert(ezMakeHashedString("var"), ezExpressionAST::DataType::Unknown);

  s_KnownTypes.Insert(ezMakeHashedString("vec2"), ezExpressionAST::DataType::Float2);
  s_KnownTypes.Insert(ezMakeHashedString("vec3"), ezExpressionAST::DataType::Float3);
  s_KnownTypes.Insert(ezMakeHashedString("vec4"), ezExpressionAST::DataType::Float4);

  s_KnownTypes.Insert(ezMakeHashedString("vec2i"), ezExpressionAST::DataType::Int2);
  s_KnownTypes.Insert(ezMakeHashedString("vec3i"), ezExpressionAST::DataType::Int3);
  s_KnownTypes.Insert(ezMakeHashedString("vec4i"), ezExpressionAST::DataType::Int4);

  ezStringBuilder sTypeName;
  for (ezUInt32 type = ezExpressionAST::DataType::Bool; type < ezExpressionAST::DataType::Count; ++type)
  {
    sTypeName = ezExpressionAST::DataType::GetName(static_cast<ezExpressionAST::DataType::Enum>(type));
    sTypeName.ToLower();

    ezHashedString sTypeNameHashed;
    sTypeNameHashed.Assign(sTypeName);

    s_KnownTypes.Insert(sTypeNameHashed, static_cast<ezExpressionAST::DataType::Enum>(type));
  }
}

void ezExpressionParser::RegisterBuiltinFunctions()
{
  if (s_BuiltinFunctions.IsEmpty() == false)
    return;

  // Unary
  s_BuiltinFunctions.Insert(ezMakeHashedString("abs"), ezExpressionAST::NodeType::Absolute);
  s_BuiltinFunctions.Insert(ezMakeHashedString("saturate"), ezExpressionAST::NodeType::Saturate);
  s_BuiltinFunctions.Insert(ezMakeHashedString("sqrt"), ezExpressionAST::NodeType::Sqrt);
  s_BuiltinFunctions.Insert(ezMakeHashedString("exp"), ezExpressionAST::NodeType::Exp);
  s_BuiltinFunctions.Insert(ezMakeHashedString("ln"), ezExpressionAST::NodeType::Ln);
  s_BuiltinFunctions.Insert(ezMakeHashedString("log2"), ezExpressionAST::NodeType::Log2);
  s_BuiltinFunctions.Insert(ezMakeHashedString("log10"), ezExpressionAST::NodeType::Log10);
  s_BuiltinFunctions.Insert(ezMakeHashedString("pow2"), ezExpressionAST::NodeType::Pow2);
  s_BuiltinFunctions.Insert(ezMakeHashedString("sin"), ezExpressionAST::NodeType::Sin);
  s_BuiltinFunctions.Insert(ezMakeHashedString("cos"), ezExpressionAST::NodeType::Cos);
  s_BuiltinFunctions.Insert(ezMakeHashedString("tan"), ezExpressionAST::NodeType::Tan);
  s_BuiltinFunctions.Insert(ezMakeHashedString("asin"), ezExpressionAST::NodeType::ASin);
  s_BuiltinFunctions.Insert(ezMakeHashedString("acos"), ezExpressionAST::NodeType::ACos);
  s_BuiltinFunctions.Insert(ezMakeHashedString("atan"), ezExpressionAST::NodeType::ATan);
  s_BuiltinFunctions.Insert(ezMakeHashedString("radToDeg"), ezExpressionAST::NodeType::RadToDeg);
  s_BuiltinFunctions.Insert(ezMakeHashedString("rad_to_deg"), ezExpressionAST::NodeType::RadToDeg);
  s_BuiltinFunctions.Insert(ezMakeHashedString("degToRad"), ezExpressionAST::NodeType::DegToRad);
  s_BuiltinFunctions.Insert(ezMakeHashedString("deg_to_rad"), ezExpressionAST::NodeType::DegToRad);
  s_BuiltinFunctions.Insert(ezMakeHashedString("round"), ezExpressionAST::NodeType::Round);
  s_BuiltinFunctions.Insert(ezMakeHashedString("floor"), ezExpressionAST::NodeType::Floor);
  s_BuiltinFunctions.Insert(ezMakeHashedString("ceil"), ezExpressionAST::NodeType::Ceil);
  s_BuiltinFunctions.Insert(ezMakeHashedString("trunc"), ezExpressionAST::NodeType::Trunc);
  s_BuiltinFunctions.Insert(ezMakeHashedString("frac"), ezExpressionAST::NodeType::Frac);
  s_BuiltinFunctions.Insert(ezMakeHashedString("length"), ezExpressionAST::NodeType::Length);
  s_BuiltinFunctions.Insert(ezMakeHashedString("normalize"), ezExpressionAST::NodeType::Normalize);
  s_BuiltinFunctions.Insert(ezMakeHashedString("all"), ezExpressionAST::NodeType::All);
  s_BuiltinFunctions.Insert(ezMakeHashedString("any"), ezExpressionAST::NodeType::Any);

  // Binary
  s_BuiltinFunctions.Insert(ezMakeHashedString("mod"), ezExpressionAST::NodeType::Modulo);
  s_BuiltinFunctions.Insert(ezMakeHashedString("log"), ezExpressionAST::NodeType::Log);
  s_BuiltinFunctions.Insert(ezMakeHashedString("pow"), ezExpressionAST::NodeType::Pow);
  s_BuiltinFunctions.Insert(ezMakeHashedString("min"), ezExpressionAST::NodeType::Min);
  s_BuiltinFunctions.Insert(ezMakeHashedString("max"), ezExpressionAST::NodeType::Max);
  s_BuiltinFunctions.Insert(ezMakeHashedString("dot"), ezExpressionAST::NodeType::Dot);
  s_BuiltinFunctions.Insert(ezMakeHashedString("cross"), ezExpressionAST::NodeType::Cross);
  s_BuiltinFunctions.Insert(ezMakeHashedString("reflect"), ezExpressionAST::NodeType::Reflect);

  // Ternary
  s_BuiltinFunctions.Insert(ezMakeHashedString("clamp"), ezExpressionAST::NodeType::Clamp);
  s_BuiltinFunctions.Insert(ezMakeHashedString("lerp"), ezExpressionAST::NodeType::Lerp);
  s_BuiltinFunctions.Insert(ezMakeHashedString("smoothstep"), ezExpressionAST::NodeType::SmoothStep);
  s_BuiltinFunctions.Insert(ezMakeHashedString("smootherstep"), ezExpressionAST::NodeType::SmootherStep);
}

void ezExpressionParser::SetupInAndOutputs(ezArrayPtr<ezExpression::StreamDesc> inputs, ezArrayPtr<ezExpression::StreamDesc> outputs)
{
  m_KnownVariables.Clear();

  for (auto& inputDesc : inputs)
  {
    auto pInput = m_pAST->CreateInput(inputDesc);
    m_pAST->m_InputNodes.PushBack(pInput);
    m_KnownVariables.Insert(inputDesc.m_sName, pInput);
  }

  for (auto& outputDesc : outputs)
  {
    auto pOutputNode = m_pAST->CreateOutput(outputDesc, nullptr);
    m_pAST->m_OutputNodes.PushBack(pOutputNode);
    m_KnownVariables.Insert(outputDesc.m_sName, pOutputNode);
  }
}

ezResult ezExpressionParser::ParseStatement()
{
  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  if (AcceptStatementTerminator())
  {
    // empty statement
    return EZ_SUCCESS;
  }

  if (m_uiCurrentToken >= m_TokenStream.GetCount())
    return EZ_FAILURE;

  const ezToken* pIdentifierToken = m_TokenStream[m_uiCurrentToken];
  if (pIdentifierToken->m_iType != ezTokenType::Identifier)
  {
    ReportError(pIdentifierToken, "Syntax error, expected type or variable");
  }

  ezEnum<ezExpressionAST::DataType> type;
  if (ParseType(pIdentifierToken->m_DataView, type).Succeeded())
  {
    return ParseVariableDefinition(type);
  }

  return ParseAssignment();
}

ezResult ezExpressionParser::ParseType(ezStringView sTypeName, ezEnum<ezExpressionAST::DataType>& out_type)
{
  ezTempHashedString sTypeNameHashed(sTypeName);
  if (s_KnownTypes.TryGetValue(sTypeNameHashed, out_type))
  {
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezExpressionParser::ParseVariableDefinition(ezEnum<ezExpressionAST::DataType> type)
{
  // skip type
  EZ_SUCCEED_OR_RETURN(Expect(ezTokenType::Identifier));

  const ezToken* pIdentifierToken = nullptr;
  EZ_SUCCEED_OR_RETURN(Expect(ezTokenType::Identifier, &pIdentifierToken));

  ezHashedString sHashedVarName;
  sHashedVarName.Assign(pIdentifierToken->m_DataView);

  ezExpressionAST::Node* pVariableNode;
  if (m_KnownVariables.TryGetValue(sHashedVarName, pVariableNode))
  {
    const char* szExisting = "a variable";
    if (ezExpressionAST::NodeType::IsInput(pVariableNode->m_Type))
    {
      szExisting = "an input";
    }
    else if (ezExpressionAST::NodeType::IsOutput(pVariableNode->m_Type))
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

  m_KnownVariables.Insert(sHashedVarName, EnsureExpectedType(pExpression, type));
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

  ezStringView sPartialAssignmentMask;
  if (Accept(m_TokenStream, m_uiCurrentToken, "."))
  {
    const ezToken* pSwizzleToken = nullptr;
    if (Expect(ezTokenType::Identifier, &pSwizzleToken).Failed())
    {
      ReportError(m_TokenStream[m_uiCurrentToken], "Invalid partial assignment");
      return EZ_FAILURE;
    }

    sPartialAssignmentMask = pSwizzleToken->m_DataView;
  }

  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  ezExpressionAST::NodeType::Enum assignOperator = ezExpressionAST::NodeType::Invalid;
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_assignOperators); ++i)
  {
    auto& op = s_assignOperators[i];
    if (AcceptOperator(op.m_sName))
    {
      assignOperator = op.m_NodeType;
      m_uiCurrentToken += op.m_sName.GetElementCount();
      break;
    }
  }

  if (assignOperator == ezExpressionAST::NodeType::Invalid)
  {
    EZ_SUCCEED_OR_RETURN(Expect("="));
  }

  ezExpressionAST::Node* pExpression = ParseExpression();
  if (pExpression == nullptr)
    return EZ_FAILURE;

  if (assignOperator != ezExpressionAST::NodeType::Invalid)
  {
    pExpression = m_pAST->CreateBinaryOperator(assignOperator, Unpack(pVarNode), pExpression);
  }

  if (sPartialAssignmentMask.IsEmpty() == false)
  {
    auto pConstructor = m_pAST->CreateConstructorCall(Unpack(pVarNode, false), pExpression, sPartialAssignmentMask);
    if (pConstructor == nullptr)
    {
      ReportError(pIdentifierToken, ezFmt("Invalid partial assignment .{} = {}", sPartialAssignmentMask, ezExpressionAST::DataType::GetName(pExpression->m_ReturnType)));
      return EZ_FAILURE;
    }

    pExpression = pConstructor;
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
  m_KnownVariables[sHashedVarName] = EnsureExpectedType(pExpression, pVarNode->m_ReturnType);
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
      return ParseSwizzle(ParseFunctionCall(sIdentifier));
    }
    else if (sIdentifier == "true")
    {
      return m_pAST->CreateConstant(true, ezExpressionAST::DataType::Bool);
    }
    else if (sIdentifier == "false")
    {
      return m_pAST->CreateConstant(false, ezExpressionAST::DataType::Bool);
    }
    else if (sIdentifier == "PI")
    {
      return m_pAST->CreateConstant(ezMath::Pi<float>(), ezExpressionAST::DataType::Float);
    }
    else
    {
      auto pVariable = GetVariable(sIdentifier);
      if (pVariable == nullptr)
      {
        ReportError(pIdentifierToken, ezFmt("Undeclared identifier '{}'", sIdentifier));
        return nullptr;
      }
      return ParseSwizzle(Unpack(pVariable));
    }
  }

  ezUInt32 uiValueToken = 0;
  if (Accept(m_TokenStream, m_uiCurrentToken, ezTokenType::Integer, &uiValueToken))
  {
    const ezString sVal = m_TokenStream[uiValueToken]->m_DataView;

    ezInt64 iConstant = 0;
    if (sVal.StartsWith_NoCase("0x"))
    {
      ezUInt64 uiHexConstant = 0;
      ezConversionUtils::ConvertHexStringToUInt64(sVal, uiHexConstant).IgnoreResult();
      iConstant = uiHexConstant;
    }
    else
    {
      ezConversionUtils::StringToInt64(sVal, iConstant).IgnoreResult();
    }

    return m_pAST->CreateConstant((int)iConstant, ezExpressionAST::DataType::Int);
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, ezTokenType::Float, &uiValueToken))
  {
    const ezString sVal = m_TokenStream[uiValueToken]->m_DataView;

    double fConstant = 0;
    ezConversionUtils::StringToFloat(sVal, fConstant).IgnoreResult();

    return m_pAST->CreateConstant((float)fConstant, ezExpressionAST::DataType::Float);
  }

  if (Accept(m_TokenStream, m_uiCurrentToken, "("))
  {
    auto pExpression = ParseExpression();
    if (Expect(")").Failed())
      return nullptr;

    return ParseSwizzle(pExpression);
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
  ezUInt32 uiOperatorLength = 0;
  while (AcceptBinaryOperator(binaryOp, iBinaryOpPrecedence, uiOperatorLength) && iBinaryOpPrecedence < iPrecedence)
  {
    // Consume token.
    m_uiCurrentToken += uiOperatorLength;

    auto pSecondOperand = ParseExpression(iBinaryOpPrecedence);
    if (pSecondOperand == nullptr)
      return nullptr;

    if (binaryOp == ezExpressionAST::NodeType::Select)
    {
      if (Expect(":").Failed())
        return nullptr;

      auto pThirdOperand = ParseExpression(iBinaryOpPrecedence);
      if (pThirdOperand == nullptr)
        return nullptr;

      pExpression = m_pAST->CreateTernaryOperator(ezExpressionAST::NodeType::Select, pExpression, pSecondOperand, pThirdOperand);
    }
    else
    {
      pExpression = m_pAST->CreateBinaryOperator(binaryOp, pExpression, pSecondOperand);
    }
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
    if (pOperand == nullptr)
      return nullptr;

    return m_pAST->CreateUnaryOperator(ezExpressionAST::NodeType::Negate, pOperand);
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "~"))
  {
    auto pOperand = ParseUnaryExpression();
    if (pOperand == nullptr)
      return nullptr;

    return m_pAST->CreateUnaryOperator(ezExpressionAST::NodeType::BitwiseNot, pOperand);
  }
  else if (Accept(m_TokenStream, m_uiCurrentToken, "!"))
  {
    auto pOperand = ParseUnaryExpression();
    if (pOperand == nullptr)
      return nullptr;

    return m_pAST->CreateUnaryOperator(ezExpressionAST::NodeType::LogicalNot, pOperand);
  }

  return ParseFactor();
}

ezExpressionAST::Node* ezExpressionParser::ParseFunctionCall(ezStringView sFunctionName)
{
  // "(" of the function call
  const ezToken* pFunctionToken = m_TokenStream[m_uiCurrentToken - 1];

  ezSmallArray<ezExpressionAST::Node*, 8> arguments;
  if (Accept(m_TokenStream, m_uiCurrentToken, ")") == false)
  {
    do
    {
      arguments.PushBack(ParseExpression());
    } while (Accept(m_TokenStream, m_uiCurrentToken, ","));

    if (Expect(")").Failed())
      return nullptr;
  }

  auto CheckArgumentCount = [&](ezUInt32 uiExpectedArgumentCount) -> ezResult
  {
    if (arguments.GetCount() != uiExpectedArgumentCount)
    {
      ReportError(pFunctionToken, ezFmt("Invalid argument count for '{}'. Expected {} but got {}", sFunctionName, uiExpectedArgumentCount, arguments.GetCount()));
      return EZ_FAILURE;
    }
    return EZ_SUCCESS;
  };

  ezHashedString sHashedFuncName;
  sHashedFuncName.Assign(sFunctionName);

  ezEnum<ezExpressionAST::DataType> dataType;
  if (s_KnownTypes.TryGetValue(sHashedFuncName, dataType))
  {
    ezUInt32 uiElementCount = ezExpressionAST::DataType::GetElementCount(dataType);
    if (arguments.GetCount() > uiElementCount)
    {
      ReportError(pFunctionToken, ezFmt("Invalid argument count for '{}'. Expected 0 - {} but got {}", sFunctionName, uiElementCount, arguments.GetCount()));
      return nullptr;
    }

    return m_pAST->CreateConstructorCall(dataType, arguments);
  }

  ezEnum<ezExpressionAST::NodeType> builtinType;
  if (s_BuiltinFunctions.TryGetValue(sHashedFuncName, builtinType))
  {
    if (ezExpressionAST::NodeType::IsUnary(builtinType))
    {
      if (CheckArgumentCount(1).Failed())
        return nullptr;

      return m_pAST->CreateUnaryOperator(builtinType, arguments[0]);
    }
    else if (ezExpressionAST::NodeType::IsBinary(builtinType))
    {
      if (CheckArgumentCount(2).Failed())
        return nullptr;

      return m_pAST->CreateBinaryOperator(builtinType, arguments[0], arguments[1]);
    }
    else if (ezExpressionAST::NodeType::IsTernary(builtinType))
    {
      if (CheckArgumentCount(3).Failed())
        return nullptr;

      return m_pAST->CreateTernaryOperator(builtinType, arguments[0], arguments[1], arguments[2]);
    }

    EZ_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }

  // external function
  const ezHybridArray<ezExpression::FunctionDesc, 1>* pFunctionDescs = nullptr;
  if (m_FunctionDescs.TryGetValue(sHashedFuncName, pFunctionDescs))
  {
    ezUInt32 uiMinArgumentCount = ezInvalidIndex;
    for (auto& funcDesc : *pFunctionDescs)
    {
      uiMinArgumentCount = ezMath::Min<ezUInt32>(uiMinArgumentCount, funcDesc.m_uiNumRequiredInputs);
    }

    if (arguments.GetCount() < uiMinArgumentCount)
    {
      ReportError(pFunctionToken, ezFmt("Invalid argument count for '{}'. Expected at least {} but got {}", sFunctionName, uiMinArgumentCount, arguments.GetCount()));
      return nullptr;
    }

    return m_pAST->CreateFunctionCall(*pFunctionDescs, arguments);
  }

  ReportError(pFunctionToken, ezFmt("Undeclared function '{}'", sFunctionName));
  return nullptr;
}

ezExpressionAST::Node* ezExpressionParser::ParseSwizzle(ezExpressionAST::Node* pExpression)
{
  if (Accept(m_TokenStream, m_uiCurrentToken, "."))
  {
    const ezToken* pSwizzleToken = nullptr;
    if (Expect(ezTokenType::Identifier, &pSwizzleToken).Failed())
      return nullptr;

    pExpression = m_pAST->CreateSwizzle(pSwizzleToken->m_DataView, pExpression);
    if (pExpression == nullptr)
    {
      ReportError(pSwizzleToken, ezFmt("Invalid swizzle '{}'", pSwizzleToken->m_DataView));
    }
  }

  return pExpression;
}

// Does NOT advance the current token beyond the operator!
bool ezExpressionParser::AcceptOperator(ezStringView sName)
{
  const ezUInt32 uiOperatorLength = sName.GetElementCount();

  if (m_uiCurrentToken + uiOperatorLength - 1 >= m_TokenStream.GetCount())
    return false;

  for (ezUInt32 charIndex = 0; charIndex < uiOperatorLength; ++charIndex)
  {
    const ezUInt32 c = sName.GetStartPointer()[charIndex];
    if (m_TokenStream[m_uiCurrentToken + charIndex]->m_DataView.GetCharacter() != c)
    {
      return false;
    }
  }

  return true;
}

// Does NOT advance the current token beyond the binary operator!
bool ezExpressionParser::AcceptBinaryOperator(ezExpressionAST::NodeType::Enum& out_binaryOp, int& out_iOperatorPrecedence, ezUInt32& out_uiOperatorLength)
{
  SkipWhitespace(m_TokenStream, m_uiCurrentToken);

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(s_binaryOperators); ++i)
  {
    auto& op = s_binaryOperators[i];
    if (AcceptOperator(op.m_sName))
    {
      out_binaryOp = op.m_NodeType;
      out_iOperatorPrecedence = op.m_iPrecedence;
      out_uiOperatorLength = op.m_sName.GetElementCount();
      return true;
    }
  }

  return false;
}

ezExpressionAST::Node* ezExpressionParser::GetVariable(ezStringView sVarName)
{
  ezHashedString sHashedVarName;
  sHashedVarName.Assign(sVarName);

  ezExpressionAST::Node* pVariableNode = nullptr;
  if (m_KnownVariables.TryGetValue(sHashedVarName, pVariableNode) == false && m_Options.m_bTreatUnknownVariablesAsInputs)
  {
    pVariableNode = m_pAST->CreateInput({sHashedVarName, ezProcessingStream::DataType::Float});
    m_KnownVariables.Insert(sHashedVarName, pVariableNode);
  }

  return pVariableNode;
}

ezExpressionAST::Node* ezExpressionParser::EnsureExpectedType(ezExpressionAST::Node* pNode, ezExpressionAST::DataType::Enum expectedType)
{
  if (expectedType != ezExpressionAST::DataType::Unknown)
  {
    const auto nodeRegisterType = ezExpressionAST::DataType::GetRegisterType(pNode->m_ReturnType);
    const auto expectedRegisterType = ezExpressionAST::DataType::GetRegisterType(expectedType);
    if (nodeRegisterType != expectedRegisterType)
    {
      pNode = m_pAST->CreateUnaryOperator(ezExpressionAST::NodeType::TypeConversion, pNode, expectedType);
    }

    const ezUInt32 nodeElementCount = ezExpressionAST::DataType::GetElementCount(pNode->m_ReturnType);
    const ezUInt32 expectedElementCount = ezExpressionAST::DataType::GetElementCount(expectedType);
    if (nodeElementCount < expectedElementCount)
    {
      pNode = m_pAST->CreateConstructorCall(expectedType, ezMakeArrayPtr(&pNode, 1));
    }
  }

  return pNode;
}

ezExpressionAST::Node* ezExpressionParser::Unpack(ezExpressionAST::Node* pNode, bool bUnassignedError /*= true*/)
{
  if (ezExpressionAST::NodeType::IsOutput(pNode->m_Type))
  {
    auto pOutput = static_cast<ezExpressionAST::Output*>(pNode);
    if (pOutput->m_pExpression == nullptr && bUnassignedError)
    {
      ReportError(m_TokenStream[m_uiCurrentToken], ezFmt("Output '{}' has not been assigned yet", pOutput->m_Desc.m_sName));
    }

    return pOutput->m_pExpression;
  }

  return pNode;
}

ezResult ezExpressionParser::CheckOutputs()
{
  for (auto pOutputNode : m_pAST->m_OutputNodes)
  {
    if (pOutputNode->m_pExpression == nullptr)
    {
      ezLog::Error("Output '{}' was never written", pOutputNode->m_Desc.m_sName);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

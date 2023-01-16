#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Utilities/DGMLWriter.h>

// static
bool ezExpressionAST::NodeType::IsUnary(Enum nodeType)
{
  return nodeType > FirstUnary && nodeType < LastUnary;
}

// static
bool ezExpressionAST::NodeType::IsBinary(Enum nodeType)
{
  return nodeType > FirstBinary && nodeType < LastBinary;
}

// static
bool ezExpressionAST::NodeType::IsTernary(Enum nodeType)
{
  return nodeType > FirstTernary && nodeType < LastTernary;
}

// static
bool ezExpressionAST::NodeType::IsConstant(Enum nodeType)
{
  return nodeType == Constant;
}

// static
bool ezExpressionAST::NodeType::IsInput(Enum nodeType)
{
  return nodeType == Input;
}

// static
bool ezExpressionAST::NodeType::IsOutput(Enum nodeType)
{
  return nodeType == Output;
}

// static
bool ezExpressionAST::NodeType::IsFunctionCall(Enum nodeType)
{
  return nodeType == FunctionCall;
}

// static
bool ezExpressionAST::NodeType::IsConstructorCall(Enum nodeType)
{
  return nodeType == ConstructorCall;
}

namespace
{
  static const char* s_szNodeTypeNames[] = {
    "Invalid",

    // Unary
    "",
    "Negate",
    "Absolute",
    "Saturate",
    "Sqrt",
    "Exp",
    "Ln",
    "Log2",
    "Log10",
    "Pow2",
    "Sin",
    "Cos",
    "Tan",
    "ASin",
    "ACos",
    "ATan",
    "RadToDeg",
    "DegToRad",
    "Round",
    "Floor",
    "Ceil",
    "Trunc",
    "Frac",
    "BitwiseNot",
    "LogicalNot",
    "TypeConversion",
    "",

    // Binary
    "",
    "Add",
    "Subtract",
    "Multiply",
    "Divide",
    "Modulo",
    "Log",
    "Pow",
    "Min",
    "Max",
    "BitshiftLeft",
    "BitshiftRight",
    "BitwiseAnd",
    "BitwiseXor",
    "BitwiseOr",
    "Equal",
    "NotEqual",
    "Less",
    "LessEqual",
    "Greater",
    "GreaterEqual",
    "LogicalAnd",
    "LogicalOr",
    "",

    // Ternary
    "",
    "Clamp",
    "Select",
    "Lerp",
    "",

    "Constant",
    "Input",
    "Output",

    "FunctionCall",
    "ConstructorCall",
  };

  static_assert(EZ_ARRAY_SIZE(s_szNodeTypeNames) == ezExpressionAST::NodeType::Count);

  static constexpr ezUInt16 BuildSignature(ezExpression::RegisterType::Enum returnType, ezExpression::RegisterType::Enum a, ezExpression::RegisterType::Enum b = ezExpression::RegisterType::Unknown, ezExpression::RegisterType::Enum c = ezExpression::RegisterType::Unknown)
  {
    ezUInt32 signature = static_cast<ezUInt32>(returnType);
    signature |= a << ezExpression::RegisterType::MaxNumBits * 1;
    signature |= b << ezExpression::RegisterType::MaxNumBits * 2;
    signature |= c << ezExpression::RegisterType::MaxNumBits * 3;
    return static_cast<ezUInt16>(signature);
  }

  static constexpr ezExpression::RegisterType::Enum GetReturnTypeFromSignature(ezUInt16 uiSignature)
  {
    ezUInt32 uiMask = EZ_BIT(ezExpression::RegisterType::MaxNumBits) - 1;
    return static_cast<ezExpression::RegisterType::Enum>(uiSignature & uiMask);
  }

  static constexpr ezExpression::RegisterType::Enum GetArgumentTypeFromSignature(ezUInt16 uiSignature, ezUInt32 uiArgumentIndex)
  {
    ezUInt32 uiShift = ezExpression::RegisterType::MaxNumBits * (uiArgumentIndex + 1);
    ezUInt32 uiMask = EZ_BIT(ezExpression::RegisterType::MaxNumBits) - 1;
    return static_cast<ezExpression::RegisterType::Enum>((uiSignature >> uiShift) & uiMask);
  }

#define SIG1(r, a) BuildSignature(ezExpression::RegisterType::r, ezExpression::RegisterType::a)
#define SIG2(r, a, b) BuildSignature(ezExpression::RegisterType::r, ezExpression::RegisterType::a, ezExpression::RegisterType::b)
#define SIG3(r, a, b, c) BuildSignature(ezExpression::RegisterType::r, ezExpression::RegisterType::a, ezExpression::RegisterType::b, ezExpression::RegisterType::c)

  struct Overloads
  {
    ezUInt16 m_Signatures[4] = {};
  };

  static Overloads s_NodeTypeOverloads[] = {
    {}, // Invalid,

    // Unary
    {},                                   // FirstUnary,
    {SIG1(Float, Float), SIG1(Int, Int)}, // Negate,
    {SIG1(Float, Float), SIG1(Int, Int)}, // Absolute,
    {SIG1(Float, Float), SIG1(Int, Int)}, // Saturate,
    {SIG1(Float, Float)},                 // Sqrt,
    {SIG1(Float, Float)},                 // Exp,
    {SIG1(Float, Float)},                 // Ln,
    {SIG1(Float, Float), SIG1(Int, Int)}, // Log2,
    {SIG1(Float, Float)},                 // Log10,
    {SIG1(Float, Float), SIG1(Int, Int)}, // Pow2,
    {SIG1(Float, Float)},                 // Sin,
    {SIG1(Float, Float)},                 // Cos,
    {SIG1(Float, Float)},                 // Tan,
    {SIG1(Float, Float)},                 // ASin,
    {SIG1(Float, Float)},                 // ACos,
    {SIG1(Float, Float)},                 // ATan,
    {SIG1(Float, Float)},                 // RadToDeg,
    {SIG1(Float, Float)},                 // DegToRad,
    {SIG1(Float, Float)},                 // Round,
    {SIG1(Float, Float)},                 // Floor,
    {SIG1(Float, Float)},                 // Ceil,
    {SIG1(Float, Float)},                 // Trunc,
    {SIG1(Float, Float)},                 // Frac,
    {SIG1(Int, Int)},                     // BitwiseNot,
    {SIG1(Bool, Bool)},                   // LogicalNot,
    {},                                   // TypeConversion,
    {},                                   // LastUnary,

    // Binary
    {},                                                                       // FirstBinary,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Add,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Subtract,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Multiply,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Divide,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                          // Modulo,
    {SIG2(Float, Float, Float)},                                              // Log,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Pow,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Min,
    {SIG2(Float, Float, Float), SIG2(Int, Int, Int)},                         // Max,
    {SIG2(Int, Int, Int)},                                                    // BitshiftLeft,
    {SIG2(Int, Int, Int)},                                                    // BitshiftRight,
    {SIG2(Int, Int, Int)},                                                    // BitwiseAnd,
    {SIG2(Int, Int, Int)},                                                    // BitwiseXor,
    {SIG2(Int, Int, Int)},                                                    // BitwiseOr,
    {SIG2(Bool, Float, Float), SIG2(Bool, Int, Int), SIG2(Bool, Bool, Bool)}, // Equal,
    {SIG2(Bool, Float, Float), SIG2(Bool, Int, Int), SIG2(Bool, Bool, Bool)}, // NotEqual,
    {SIG2(Bool, Float, Float), SIG2(Bool, Int, Int)},                         // Less,
    {SIG2(Bool, Float, Float), SIG2(Bool, Int, Int)},                         // LessEqual,
    {SIG2(Bool, Float, Float), SIG2(Bool, Int, Int)},                         // Greater,
    {SIG2(Bool, Float, Float), SIG2(Bool, Int, Int)},                         // GreaterEqual,
    {SIG2(Bool, Bool, Bool)},                                                 // LogicalAnd,
    {SIG2(Bool, Bool, Bool)},                                                 // LogicalOr,
    {},                                                                       // LastBinary,

    // Ternary
    {},                                                                                         // FirstTernary,
    {SIG3(Float, Float, Float, Float), SIG3(Int, Int, Int, Int)},                               // Clamp,
    {SIG3(Float, Bool, Float, Float), SIG3(Int, Bool, Int, Int), SIG3(Bool, Bool, Bool, Bool)}, // Select,
    {SIG3(Float, Float, Float, Float)},                                                         // Lerp,
    {},                                                                                         // LastTernary,

    {}, // Constant,
    {}, // Input,
    {}, // Output,

    {}, // FunctionCall,
    {}, // ConstructorCall,
  };

  static_assert(EZ_ARRAY_SIZE(s_NodeTypeOverloads) == ezExpressionAST::NodeType::Count);
} // namespace

#undef SIG1
#undef SIG2
#undef SIG3

// static
const char* ezExpressionAST::NodeType::GetName(Enum nodeType)
{
  EZ_ASSERT_DEBUG(nodeType >= 0 && nodeType < EZ_ARRAY_SIZE(s_szNodeTypeNames), "Out of bounds access");
  return s_szNodeTypeNames[nodeType];
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  static ezVariantType::Enum s_DataTypeVariantTypes[] = {
    ezVariantType::Invalid, // Unknown,
    ezVariantType::Invalid, // Unknown2,
    ezVariantType::Invalid, // Unknown3,
    ezVariantType::Invalid, // Unknown4,

    ezVariantType::Bool,    // Bool,
    ezVariantType::Invalid, // Bool2,
    ezVariantType::Invalid, // Bool3,
    ezVariantType::Invalid, // Bool4,

    ezVariantType::Int32,    // Int,
    ezVariantType::Vector2I, // Int2,
    ezVariantType::Vector3I, // Int3,
    ezVariantType::Vector4I, // Int4,

    ezVariantType::Float,   // Float,
    ezVariantType::Vector2, // Float2,
    ezVariantType::Vector3, // Float3,
    ezVariantType::Vector4, // Float4,
  };
  static_assert(EZ_ARRAY_SIZE(s_DataTypeVariantTypes) == (size_t)ezExpressionAST::DataType::Count);

  static ezExpressionAST::DataType::Enum s_DataTypeFromStreamType[] = {
    ezExpressionAST::DataType::Float,  // Half,
    ezExpressionAST::DataType::Float2, // Half2,
    ezExpressionAST::DataType::Float3, // Half3,
    ezExpressionAST::DataType::Float4, // Half4,

    ezExpressionAST::DataType::Float,  // Float,
    ezExpressionAST::DataType::Float2, // Float2,
    ezExpressionAST::DataType::Float3, // Float3,
    ezExpressionAST::DataType::Float4, // Float4,

    ezExpressionAST::DataType::Int,  // Byte,
    ezExpressionAST::DataType::Int2, // Byte2,
    ezExpressionAST::DataType::Int3, // Byte3,
    ezExpressionAST::DataType::Int4, // Byte4,

    ezExpressionAST::DataType::Int,  // Short,
    ezExpressionAST::DataType::Int2, // Short2,
    ezExpressionAST::DataType::Int3, // Short3,
    ezExpressionAST::DataType::Int4, // Short4,

    ezExpressionAST::DataType::Int,  // Int,
    ezExpressionAST::DataType::Int2, // Int2,
    ezExpressionAST::DataType::Int3, // Int3,
    ezExpressionAST::DataType::Int4, // Int4,
  };
  static_assert(EZ_ARRAY_SIZE(s_DataTypeFromStreamType) == (size_t)ezProcessingStream::DataType::Count);

  static_assert(ezExpressionAST::DataType::Float >> 2 == ezExpression::RegisterType::Float);
  static_assert(ezExpressionAST::DataType::Int >> 2 == ezExpression::RegisterType::Int);
  static_assert(ezExpressionAST::DataType::Bool >> 2 == ezExpression::RegisterType::Bool);
  static_assert(ezExpressionAST::DataType::Unknown >> 2 == ezExpression::RegisterType::Unknown);

  static const char* s_szDataTypeNames[] = {
    "Unknown",  // Unknown,
    "Unknown2", // Unknown2,
    "Unknown3", // Unknown3,
    "Unknown4", // Unknown4,

    "Bool",  // Bool,
    "Bool2", // Bool2,
    "Bool3", // Bool3,
    "Bool4", // Bool4,

    "Int",  // Int,
    "Int2", // Int2,
    "Int3", // Int3,
    "Int4", // Int4,

    "Float",  // Float,
    "Float2", // Float2,
    "Float3", // Float3,
    "Float4", // Float4,
  };

  static_assert(EZ_ARRAY_SIZE(s_szDataTypeNames) == ezExpressionAST::DataType::Count);
} // namespace


// static
ezVariantType::Enum ezExpressionAST::DataType::GetVariantType(Enum dataType)
{
  EZ_ASSERT_DEBUG(dataType >= 0 && dataType < EZ_ARRAY_SIZE(s_DataTypeVariantTypes), "Out of bounds access");
  return s_DataTypeVariantTypes[dataType];
}

// static
ezExpressionAST::DataType::Enum ezExpressionAST::DataType::FromStreamType(ezProcessingStream::DataType dataType)
{
  EZ_ASSERT_DEBUG(static_cast<ezUInt32>(dataType) >= 0 && static_cast<ezUInt32>(dataType) < EZ_ARRAY_SIZE(s_DataTypeFromStreamType), "Out of bounds access");
  return s_DataTypeFromStreamType[static_cast<ezUInt32>(dataType)];
}

// static
const char* ezExpressionAST::DataType::GetName(Enum dataType)
{
  EZ_ASSERT_DEBUG(dataType >= 0 && dataType < EZ_ARRAY_SIZE(s_szDataTypeNames), "Out of bounds access");
  return s_szDataTypeNames[dataType];
}

//////////////////////////////////////////////////////////////////////////

ezExpressionAST::ezExpressionAST()
  : m_Allocator("Expression AST", ezFoundation::GetAlignedAllocator())
{
  static_assert(sizeof(Node) == 8);
  static_assert(sizeof(UnaryOperator) == 16);
  static_assert(sizeof(BinaryOperator) == 24);
  static_assert(sizeof(FunctionCall) == 96);
}

ezExpressionAST::~ezExpressionAST() = default;

ezExpressionAST::UnaryOperator* ezExpressionAST::CreateUnaryOperator(NodeType::Enum type, Node* pOperand, DataType::Enum returnType /*= DataType::Unknown*/)
{
  EZ_ASSERT_DEBUG(NodeType::IsUnary(type), "Type '{}' is not an unary operator", NodeType::GetName(type));

  auto pUnaryOperator = EZ_NEW(&m_Allocator, UnaryOperator);
  pUnaryOperator->m_Type = type;
  pUnaryOperator->m_ReturnType = returnType;
  pUnaryOperator->m_pOperand = pOperand;

  ResolveOverloads(pUnaryOperator);

  return pUnaryOperator;
}

ezExpressionAST::BinaryOperator* ezExpressionAST::CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand)
{
  EZ_ASSERT_DEBUG(NodeType::IsBinary(type), "Type '{}' is not a binary operator", NodeType::GetName(type));

  auto pBinaryOperator = EZ_NEW(&m_Allocator, BinaryOperator);
  pBinaryOperator->m_Type = type;
  pBinaryOperator->m_ReturnType = DataType::Unknown;
  pBinaryOperator->m_pLeftOperand = pLeftOperand;
  pBinaryOperator->m_pRightOperand = pRightOperand;

  ResolveOverloads(pBinaryOperator);

  return pBinaryOperator;
}

ezExpressionAST::TernaryOperator* ezExpressionAST::CreateTernaryOperator(NodeType::Enum type, Node* pFirstOperand, Node* pSecondOperand, Node* pThirdOperand)
{
  EZ_ASSERT_DEBUG(NodeType::IsTernary(type), "Type '{}' is not a ternary operator", NodeType::GetName(type));

  auto pTernaryOperator = EZ_NEW(&m_Allocator, TernaryOperator);
  pTernaryOperator->m_Type = type;
  pTernaryOperator->m_ReturnType = DataType::Unknown;
  pTernaryOperator->m_pFirstOperand = pFirstOperand;
  pTernaryOperator->m_pSecondOperand = pSecondOperand;
  pTernaryOperator->m_pThirdOperand = pThirdOperand;

  ResolveOverloads(pTernaryOperator);

  return pTernaryOperator;
}

ezExpressionAST::Constant* ezExpressionAST::CreateConstant(const ezVariant& value, DataType::Enum dataType /*= DataType::Float*/)
{
  ezVariantType::Enum variantType = DataType::GetVariantType(dataType);
  EZ_ASSERT_DEV(variantType != ezVariantType::Invalid, "Invalid constant type '{}'", DataType::GetName(dataType));

  auto pConstant = EZ_NEW(&m_Allocator, Constant);
  pConstant->m_Type = NodeType::Constant;
  pConstant->m_ReturnType = dataType;
  pConstant->m_Value = value.ConvertTo(DataType::GetVariantType(dataType));

  EZ_ASSERT_DEV(pConstant->m_Value.IsValid(), "Invalid constant value or conversion to target data type failed");

  return pConstant;
}

ezExpressionAST::Input* ezExpressionAST::CreateInput(const ezExpression::StreamDesc& desc)
{
  auto pInput = EZ_NEW(&m_Allocator, Input);
  pInput->m_Type = NodeType::Input;
  pInput->m_ReturnType = DataType::FromStreamType(desc.m_DataType);
  pInput->m_Desc = desc;

  return pInput;
}

ezExpressionAST::Output* ezExpressionAST::CreateOutput(const ezExpression::StreamDesc& desc, Node* pExpression)
{
  auto pOutput = EZ_NEW(&m_Allocator, Output);
  pOutput->m_Type = NodeType::Output;
  pOutput->m_ReturnType = DataType::FromStreamType(desc.m_DataType);
  pOutput->m_Desc = desc;
  pOutput->m_pExpression = pExpression;

  return pOutput;
}

ezExpressionAST::FunctionCall* ezExpressionAST::CreateFunctionCall(const ezExpression::FunctionDesc& desc)
{
  return CreateFunctionCall(ezMakeArrayPtr(&desc, 1));
}

ezExpressionAST::FunctionCall* ezExpressionAST::CreateFunctionCall(ezArrayPtr<const ezExpression::FunctionDesc> descs)
{
  auto pFunctionCall = EZ_NEW(&m_Allocator, FunctionCall);
  pFunctionCall->m_Type = NodeType::FunctionCall;
  pFunctionCall->m_ReturnType = DataType::Unknown;

  for (auto& desc : descs)
  {
    auto it = m_FunctionDescs.Insert(desc);

    pFunctionCall->m_Descs.PushBack(&it.Key());
  }

  return pFunctionCall;
}

ezExpressionAST::ConstructorCall* ezExpressionAST::CreateConstructorCall(DataType::Enum dataType)
{
  EZ_ASSERT_DEV(dataType >= DataType::Bool, "Invalid data type for constructor");

  auto pConstructorCall = EZ_NEW(&m_Allocator, ConstructorCall);
  pConstructorCall->m_Type = NodeType::ConstructorCall;
  pConstructorCall->m_ReturnType = dataType;

  return pConstructorCall;
}

// static
ezArrayPtr<ezExpressionAST::Node*> ezExpressionAST::GetChildren(Node* pNode)
{
  NodeType::Enum nodeType = pNode->m_Type;
  if (NodeType::IsUnary(nodeType))
  {
    auto& pChild = static_cast<UnaryOperator*>(pNode)->m_pOperand;
    return ezMakeArrayPtr(&pChild, 1);
  }
  else if (NodeType::IsBinary(nodeType))
  {
    auto& pChildren = static_cast<BinaryOperator*>(pNode)->m_pLeftOperand;
    return ezMakeArrayPtr(&pChildren, 2);
  }
  else if (NodeType::IsTernary(nodeType))
  {
    auto& pChildren = static_cast<TernaryOperator*>(pNode)->m_pFirstOperand;
    return ezMakeArrayPtr(&pChildren, 3);
  }
  else if (NodeType::IsOutput(nodeType))
  {
    auto& pChild = static_cast<Output*>(pNode)->m_pExpression;
    return ezMakeArrayPtr(&pChild, 1);
  }
  else if (NodeType::IsFunctionCall(nodeType))
  {
    auto& args = static_cast<FunctionCall*>(pNode)->m_Arguments;
    return args;
  }
  else if (NodeType::IsConstructorCall(nodeType))
  {
    auto& args = static_cast<ConstructorCall*>(pNode)->m_Arguments;
    return args;
  }

  EZ_ASSERT_DEV(NodeType::IsInput(nodeType) || NodeType::IsConstant(nodeType), "Unknown node type");
  return ezArrayPtr<Node*>();
}

// static
ezArrayPtr<const ezExpressionAST::Node*> ezExpressionAST::GetChildren(const Node* pNode)
{
  NodeType::Enum nodeType = pNode->m_Type;
  if (NodeType::IsUnary(nodeType))
  {
    auto& pChild = static_cast<const UnaryOperator*>(pNode)->m_pOperand;
    return ezMakeArrayPtr((const Node**)&pChild, 1);
  }
  else if (NodeType::IsBinary(nodeType))
  {
    auto& pChildren = static_cast<const BinaryOperator*>(pNode)->m_pLeftOperand;
    return ezMakeArrayPtr((const Node**)&pChildren, 2);
  }
  else if (NodeType::IsTernary(nodeType))
  {
    auto& pChildren = static_cast<const TernaryOperator*>(pNode)->m_pFirstOperand;
    return ezMakeArrayPtr((const Node**)&pChildren, 3);
  }
  else if (NodeType::IsOutput(nodeType))
  {
    auto& pChild = static_cast<const Output*>(pNode)->m_pExpression;
    return ezMakeArrayPtr((const Node**)&pChild, 1);
  }
  else if (NodeType::IsFunctionCall(nodeType))
  {
    auto& args = static_cast<const FunctionCall*>(pNode)->m_Arguments;
    return ezArrayPtr<const Node*>((const Node**)args.GetData(), args.GetCount());
  }
  else if (NodeType::IsConstructorCall(nodeType))
  {
    auto& args = static_cast<const ConstructorCall*>(pNode)->m_Arguments;
    return ezArrayPtr<const Node*>((const Node**)args.GetData(), args.GetCount());
  }

  EZ_ASSERT_DEV(NodeType::IsInput(nodeType) || NodeType::IsConstant(nodeType), "Unknown node type");
  return ezArrayPtr<const Node*>();
}

void ezExpressionAST::ResolveOverloads(Node* pNode)
{
  if (pNode->m_uiOverloadIndex != 0xFF)
  {
    // already resolved
    return;
  }

  const NodeType::Enum nodeType = pNode->m_Type;
  if (nodeType == NodeType::TypeConversion)
  {
    EZ_ASSERT_DEV(pNode->m_ReturnType != DataType::Unknown, "Return type must be specified for conversion nodes");
    pNode->m_uiOverloadIndex = 0;
    return;
  }

  auto CalculateMatchDistance = [](ezArrayPtr<Node*> children, ezArrayPtr<const ezEnum<ezExpression::RegisterType>> expectedTypes, ezUInt32 uiNumRequiredArgs)
  {
    if (children.GetCount() < uiNumRequiredArgs)
    {
      return ezInvalidIndex;
    }

    ezUInt32 uiMatchDistance = 0;
    for (ezUInt32 i = 0; i < ezMath::Min(children.GetCount(), expectedTypes.GetCount()); ++i)
    {
      auto& pChildNode = children[i];
      if (pChildNode == nullptr || pChildNode->m_ReturnType == DataType::Unknown)
      {
        // Can't resolve yet
        return ezInvalidIndex;
      }

      auto childType = DataType::GetRegisterType(pChildNode->m_ReturnType);
      int iDistance = expectedTypes[i] - childType;
      if (iDistance < 0)
      {
        // Penalty to prevent 'narrowing' conversions
        iDistance *= -ezExpression::RegisterType::Count;
      }
      uiMatchDistance += iDistance;
    }
    return uiMatchDistance;
  };

  if (NodeType::IsUnary(nodeType) || NodeType::IsBinary(nodeType) || NodeType::IsTernary(nodeType))
  {
    auto children = GetChildren(pNode);
    ezSmallArray < ezEnum<ezExpression::RegisterType>, 4> expectedTypes;
    ezUInt32 uiBestMatchDistance = ezInvalidIndex;

    for (ezUInt32 uiSigIndex = 0; uiSigIndex < EZ_ARRAY_SIZE(Overloads::m_Signatures); ++uiSigIndex)
    {
      const ezUInt16 uiSignature = s_NodeTypeOverloads[nodeType].m_Signatures[uiSigIndex];
      if (uiSignature == 0)
        break;

      expectedTypes.Clear();
      for (ezUInt32 i = 0; i< children.GetCount(); ++i)
      {
        expectedTypes.PushBack(GetArgumentTypeFromSignature(uiSignature, i));
      }

      ezUInt32 uiMatchDistance = CalculateMatchDistance(children, expectedTypes, expectedTypes.GetCount());
      if (uiMatchDistance < uiBestMatchDistance)
      {
        pNode->m_ReturnType = DataType::FromRegisterType(GetReturnTypeFromSignature(uiSignature));
        pNode->m_uiOverloadIndex = static_cast<ezUInt8>(uiSigIndex);
        uiBestMatchDistance = uiMatchDistance;
      }
    }
  }
  else if (NodeType::IsFunctionCall(nodeType))
  {
    auto pFunctionCall = static_cast<FunctionCall*>(pNode);
    ezUInt32 uiBestMatchDistance = ezInvalidIndex;

    for (ezUInt32 uiOverloadIndex = 0; uiOverloadIndex < pFunctionCall->m_Descs.GetCount(); ++uiOverloadIndex)
    {
      auto pFuncDesc = pFunctionCall->m_Descs[uiOverloadIndex];

      ezUInt32 uiMatchDistance = CalculateMatchDistance(pFunctionCall->m_Arguments, pFuncDesc->m_InputTypes, pFuncDesc->m_uiNumRequiredInputs);
      if (uiMatchDistance < uiBestMatchDistance)
      {
        pNode->m_ReturnType = DataType::FromRegisterType(pFuncDesc->m_OutputType);
        pNode->m_uiOverloadIndex = static_cast<ezUInt8>(uiOverloadIndex);
        uiBestMatchDistance = uiMatchDistance;
      }
    }

    if (pNode->m_ReturnType != DataType::Unknown)
    {
      auto pFuncDesc = pFunctionCall->m_Descs[pNode->m_uiOverloadIndex];

      // Trim arguments array to number of inputs
      if (pFunctionCall->m_Arguments.GetCount() > pFuncDesc->m_InputTypes.GetCount())
      {
        pFunctionCall->m_Arguments.SetCount(static_cast<ezUInt16>(pFuncDesc->m_InputTypes.GetCount()));
      }
    }
  }
}

ezExpressionAST::DataType::Enum ezExpressionAST::GetExpectedChildDataType(Node* pNode, ezUInt32 uiChildIndex)
{
  const NodeType::Enum nodeType = pNode->m_Type;
  const DataType::Enum returnType = pNode->m_ReturnType;
  const ezUInt32 uiOverloadIndex = pNode->m_uiOverloadIndex;
  EZ_ASSERT_DEV(returnType != DataType::Unknown, "Return type must not be unknown");

  if (nodeType == NodeType::TypeConversion)
  {
    return DataType::Unknown;
  }
  else if (NodeType::IsUnary(nodeType) || NodeType::IsBinary(nodeType) || NodeType::IsTernary(nodeType))
  {
    EZ_ASSERT_DEV(uiOverloadIndex != 0xFF, "Unresolved overload");
    ezUInt16 uiSignature = s_NodeTypeOverloads[nodeType].m_Signatures[uiOverloadIndex];
    return DataType::FromRegisterType(GetArgumentTypeFromSignature(uiSignature, uiChildIndex));
  }
  else if (NodeType::IsOutput(nodeType))
  {
    return returnType;
  }
  else if (NodeType::IsFunctionCall(nodeType))
  {
    EZ_ASSERT_DEV(uiOverloadIndex != 0xFF, "Unresolved overload");

    auto pDesc = static_cast<const FunctionCall*>(pNode)->m_Descs[uiOverloadIndex];
    return DataType::FromRegisterType(pDesc->m_InputTypes[uiChildIndex]);
  }
  else if (NodeType::IsConstructorCall(nodeType))
  {
    // EZ_ASSERT_DEV(uiOverloadIndex != 0xFF, "Unresolved overload");

    return DataType::FromRegisterType(DataType::GetRegisterType(returnType));
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return DataType::Unknown;
}

namespace
{
  struct NodeInfo
  {
    EZ_DECLARE_POD_TYPE();

    const ezExpressionAST::Node* m_pNode;
    ezUInt32 m_uiParentGraphNode;
  };
} // namespace

void ezExpressionAST::PrintGraph(ezDGMLGraph& graph) const
{
  ezHybridArray<NodeInfo, 64> nodeStack;

  ezStringBuilder sTmp;
  for (auto pOutputNode : m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      continue;

    sTmp = NodeType::GetName(pOutputNode->m_Type);
    sTmp.Append("(", DataType::GetName(pOutputNode->m_ReturnType), ")");
    sTmp.Append(": ", pOutputNode->m_Desc.m_sName);

    ezDGMLGraph::NodeDesc nd;
    nd.m_Color = ezColorScheme::LightUI(ezColorScheme::Blue);
    ezUInt32 uiGraphNode = graph.AddNode(sTmp, &nd);

    nodeStack.PushBack({pOutputNode->m_pExpression, uiGraphNode});
  }

  ezHashTable<const Node*, ezUInt32> nodeCache;

  while (!nodeStack.IsEmpty())
  {
    NodeInfo currentNodeInfo = nodeStack.PeekBack();
    nodeStack.PopBack();

    ezUInt32 uiGraphNode = 0;
    if (currentNodeInfo.m_pNode != nullptr)
    {
      if (!nodeCache.TryGetValue(currentNodeInfo.m_pNode, uiGraphNode))
      {
        NodeType::Enum nodeType = currentNodeInfo.m_pNode->m_Type;
        sTmp = NodeType::GetName(nodeType);
        sTmp.Append("(", DataType::GetName(currentNodeInfo.m_pNode->m_ReturnType), ")");
        ezColor color = ezColor::White;

        if (NodeType::IsConstant(nodeType))
        {
          sTmp.AppendFormat(": {0}", static_cast<const Constant*>(currentNodeInfo.m_pNode)->m_Value.ConvertTo<ezString>());
        }
        else if (NodeType::IsInput(nodeType))
        {
          auto pInputNode = static_cast<const Input*>(currentNodeInfo.m_pNode);
          sTmp.Append(": ", pInputNode->m_Desc.m_sName);
          color = ezColorScheme::LightUI(ezColorScheme::Green);
        }
        else if (NodeType::IsFunctionCall(nodeType))
        {
          auto pFunctionCall = static_cast<const FunctionCall*>(currentNodeInfo.m_pNode);
          if (pFunctionCall->m_uiOverloadIndex != 0xFF)
          {
            auto pDesc = pFunctionCall->m_Descs[currentNodeInfo.m_pNode->m_uiOverloadIndex];
            sTmp.Append(": ", pDesc->GetMangledName());
          }
          else
          {
            sTmp.Append(": ", pFunctionCall->m_Descs[0]->m_sName);
          }
          color = ezColorScheme::LightUI(ezColorScheme::Yellow);
        }

        ezDGMLGraph::NodeDesc nd;
        nd.m_Color = color;
        uiGraphNode = graph.AddNode(sTmp, &nd);
        nodeCache.Insert(currentNodeInfo.m_pNode, uiGraphNode);

        // push children
        auto children = GetChildren(currentNodeInfo.m_pNode);
        for (auto pChild : children)
        {
          nodeStack.PushBack({pChild, uiGraphNode});
        }
      }
    }
    else
    {
      ezDGMLGraph::NodeDesc nd;
      nd.m_Color = ezColor::OrangeRed;
      uiGraphNode = graph.AddNode("Invalid", &nd);
    }

    graph.AddConnection(uiGraphNode, currentNodeInfo.m_uiParentGraphNode);
  }
}


EZ_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionAST);

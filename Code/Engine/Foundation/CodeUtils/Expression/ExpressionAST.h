#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionDeclarations.h>
#include <Foundation/Memory/LinearAllocator.h>

class ezDGMLGraph;

class EZ_FOUNDATION_DLL ezExpressionAST
{
public:
  struct NodeType
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Invalid,
      Default = Invalid,

      // Unary
      FirstUnary,
      Negate,
      Absolute,
      Saturate,
      Sqrt,
      Exp,
      Ln,
      Log2,
      Log10,
      Pow2,
      Sin,
      Cos,
      Tan,
      ASin,
      ACos,
      ATan,
      RadToDeg,
      DegToRad,
      Round,
      Floor,
      Ceil,
      Trunc,
      Frac,
      Length,
      Normalize,
      BitwiseNot,
      LogicalNot,
      All,
      Any,
      TypeConversion,
      LastUnary,

      // Binary
      FirstBinary,
      Add,
      Subtract,
      Multiply,
      Divide,
      Modulo,
      Log,
      Pow,
      Min,
      Max,
      Dot,
      Cross,
      Reflect,
      BitshiftLeft,
      BitshiftRight,
      BitwiseAnd,
      BitwiseXor,
      BitwiseOr,
      Equal,
      NotEqual,
      Less,
      LessEqual,
      Greater,
      GreaterEqual,
      LogicalAnd,
      LogicalOr,
      LastBinary,

      // Ternary
      FirstTernary,
      Clamp,
      Select,
      Lerp,
      SmoothStep,
      SmootherStep,
      LastTernary,

      Constant,
      Swizzle,
      Input,
      Output,

      FunctionCall,
      ConstructorCall,

      Count
    };

    static bool IsUnary(Enum nodeType);
    static bool IsBinary(Enum nodeType);
    static bool IsTernary(Enum nodeType);
    static bool IsConstant(Enum nodeType);
    static bool IsSwizzle(Enum nodeType);
    static bool IsInput(Enum nodeType);
    static bool IsOutput(Enum nodeType);
    static bool IsFunctionCall(Enum nodeType);
    static bool IsConstructorCall(Enum nodeType);

    static bool IsCommutative(Enum nodeType);
    static bool AlwaysReturnsSingleElement(Enum nodeType);

    static const char* GetName(Enum nodeType);
  };

  struct DataType
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      Unknown,
      Unknown2,
      Unknown3,
      Unknown4,

      Bool,
      Bool2,
      Bool3,
      Bool4,

      Int,
      Int2,
      Int3,
      Int4,

      Float,
      Float2,
      Float3,
      Float4,

      Count,

      Default = Unknown,
    };

    static ezVariantType::Enum GetVariantType(Enum dataType);

    static Enum FromStreamType(ezProcessingStream::DataType dataType);

    EZ_ALWAYS_INLINE static ezExpression::RegisterType::Enum GetRegisterType(Enum dataType)
    {
      return static_cast<ezExpression::RegisterType::Enum>(dataType >> 2);
    }

    EZ_ALWAYS_INLINE static Enum FromRegisterType(ezExpression::RegisterType::Enum registerType, ezUInt32 uiElementCount = 1)
    {
      return static_cast<ezExpressionAST::DataType::Enum>((registerType << 2) + uiElementCount - 1);
    }

    EZ_ALWAYS_INLINE static ezUInt32 GetElementCount(Enum dataType) { return (dataType & 0x3) + 1; }

    static const char* GetName(Enum dataType);
  };

  struct VectorComponent
  {
    using StorageType = ezUInt8;

    enum Enum
    {
      X,
      Y,
      Z,
      W,

      R = X,
      G = Y,
      B = Z,
      A = W,

      Count,

      Default = X
    };

    static const char* GetName(Enum vectorComponent);

    static Enum FromChar(ezUInt32 uiChar);
  };

  struct Node
  {
    ezEnum<NodeType> m_Type;
    ezEnum<DataType> m_ReturnType;
    ezUInt8 m_uiOverloadIndex = 0xFF;
    ezUInt8 m_uiNumInputElements = 0;

    ezUInt32 m_uiHash = 0;
  };

  struct UnaryOperator : public Node
  {
    Node* m_pOperand = nullptr;
  };

  struct BinaryOperator : public Node
  {
    Node* m_pLeftOperand = nullptr;
    Node* m_pRightOperand = nullptr;
  };

  struct TernaryOperator : public Node
  {
    Node* m_pFirstOperand = nullptr;
    Node* m_pSecondOperand = nullptr;
    Node* m_pThirdOperand = nullptr;
  };

  struct Constant : public Node
  {
    ezVariant m_Value;
  };

  struct Swizzle : public Node
  {
    ezEnum<VectorComponent> m_Components[4];
    ezUInt32 m_NumComponents = 0;
    Node* m_pExpression = nullptr;
  };

  struct Input : public Node
  {
    ezExpression::StreamDesc m_Desc;
  };

  struct Output : public Node
  {
    ezExpression::StreamDesc m_Desc;
    Node* m_pExpression = nullptr;
  };

  struct FunctionCall : public Node
  {
    ezSmallArray<const ezExpression::FunctionDesc*, 1> m_Descs;
    ezSmallArray<Node*, 8> m_Arguments;
  };

  struct ConstructorCall : public Node
  {
    ezSmallArray<Node*, 4> m_Arguments;
  };

public:
  ezExpressionAST();
  ~ezExpressionAST();

  UnaryOperator* CreateUnaryOperator(NodeType::Enum type, Node* pOperand, DataType::Enum returnType = DataType::Unknown);
  BinaryOperator* CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand);
  TernaryOperator* CreateTernaryOperator(NodeType::Enum type, Node* pFirstOperand, Node* pSecondOperand, Node* pThirdOperand);
  Constant* CreateConstant(const ezVariant& value, DataType::Enum dataType = DataType::Float);
  Swizzle* CreateSwizzle(ezStringView sSwizzle, Node* pExpression);
  Swizzle* CreateSwizzle(ezEnum<VectorComponent> component, Node* pExpression);
  Swizzle* CreateSwizzle(ezArrayPtr<ezEnum<VectorComponent>> swizzle, Node* pExpression);
  Input* CreateInput(const ezExpression::StreamDesc& desc);
  Output* CreateOutput(const ezExpression::StreamDesc& desc, Node* pExpression);
  FunctionCall* CreateFunctionCall(const ezExpression::FunctionDesc& desc, ezArrayPtr<Node*> arguments);
  FunctionCall* CreateFunctionCall(ezArrayPtr<const ezExpression::FunctionDesc> descs, ezArrayPtr<Node*> arguments);
  ConstructorCall* CreateConstructorCall(DataType::Enum dataType, ezArrayPtr<Node*> arguments);
  ConstructorCall* CreateConstructorCall(Node* pOldValue, Node* pNewValue, ezStringView sPartialAssignmentMask);

  static ezArrayPtr<Node*> GetChildren(Node* pNode);
  static ezArrayPtr<const Node*> GetChildren(const Node* pNode);

  void PrintGraph(ezDGMLGraph& inout_graph) const;

  ezSmallArray<Input*, 8> m_InputNodes;
  ezSmallArray<Output*, 8> m_OutputNodes;

  // Transforms
  Node* TypeDeductionAndConversion(Node* pNode);
  Node* ReplaceVectorInstructions(Node* pNode);
  Node* ScalarizeVectorInstructions(Node* pNode);
  Node* ReplaceUnsupportedInstructions(Node* pNode);
  Node* FoldConstants(Node* pNode);
  Node* CommonSubexpressionElimination(Node* pNode);
  Node* Validate(Node* pNode);

  ezResult ScalarizeInputs();
  ezResult ScalarizeOutputs();

private:
  void ResolveOverloads(Node* pNode);

  static DataType::Enum GetExpectedChildDataType(const Node* pNode, ezUInt32 uiChildIndex);

  static void UpdateHash(Node* pNode);
  static bool IsEqual(const Node* pNodeA, const Node* pNodeB);

  ezLinearAllocator<> m_Allocator;

  ezSet<ezExpression::FunctionDesc> m_FunctionDescs;

  ezHashTable<ezUInt32, ezSmallArray<Node*, 1>> m_NodeDeduplicationTable;
};

#pragma once

#include <ProcGenPlugin/Declarations.h>
#include <Foundation/Memory/StackAllocator.h>
#include <Foundation/Types/Variant.h>

class ezDGMLGraph;

class EZ_PROCGENPLUGIN_DLL ezExpressionAST
{
public:
  struct NodeType
  {
    typedef ezUInt32 StorageType;

    enum Enum
    {
      Invalid,
      Default = Invalid,

      // Unary
      FirstUnary,
      Negate,
      Absolute,
      Sqrt,
      Sin,
      Cos,
      Tan,
      ASin,
      ACos,
      ATan,
      LastUnary,

      // Binary
      FirstBinary,
      Add,
      Subtract,
      Multiply,
      Divide,
      Min,
      Max,
      LastBinary,

      // Constant
      FloatConstant,

      // Input
      FloatInput,

      // Output
      FloatOutput,

      FunctionCall,

      Count
    };

    static bool IsUnary(Enum nodeType);
    static bool IsBinary(Enum nodeType);
    static bool IsConstant(Enum nodeType);
    static bool IsInput(Enum nodeType);
    static bool IsOutput(Enum nodeType);

    static const char* GetName(Enum nodeType);
  };

  struct Node
  {
    EZ_FORCE_INLINE Node(NodeType::Enum type)
      : m_Type(type)
    {
    }

    ezEnum<NodeType> m_Type;
  };

  struct UnaryOperator : public Node
  {
    EZ_FORCE_INLINE UnaryOperator(NodeType::Enum type)
      : Node(type)
      , m_pOperand(nullptr)
    {
    }

    Node* m_pOperand;
  };

  struct BinaryOperator : public Node
  {
    EZ_FORCE_INLINE BinaryOperator(NodeType::Enum type)
      : Node(type)
      , m_pLeftOperand(nullptr)
      , m_pRightOperand(nullptr)
    {
    }

    Node* m_pLeftOperand;
    Node* m_pRightOperand;
  };

  struct Constant : public Node
  {
    EZ_FORCE_INLINE Constant(NodeType::Enum type)
      : Node(type)
    {
    }

    ezVariant m_Value;
  };

  struct Input : public Node
  {
    EZ_FORCE_INLINE Input(NodeType::Enum type)
      : Node(type)
    {
    }

    ezHashedString m_sName;
  };

  struct Output : public Node
  {
    EZ_FORCE_INLINE Output(NodeType::Enum type)
      : Node(type)
    {
    }

    ezHashedString m_sName;
    Node* m_pExpression;
  };

  struct FunctionCall : public Node
  {
    EZ_FORCE_INLINE FunctionCall(NodeType::Enum type)
      : Node(type)
    {
    }

    ezHashedString m_sName;
    ezHybridArray<Node*, 8> m_Arguments;
  };

public:
  ezExpressionAST();
  ~ezExpressionAST();

  UnaryOperator* CreateUnaryOperator(NodeType::Enum type, Node* pOperand);
  BinaryOperator* CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand);
  Constant* CreateConstant(const ezVariant& value);
  Input* CreateInput(const ezHashedString& sName);
  Output* CreateOutput(const ezHashedString& sName, Node* pExpression);
  FunctionCall* CreateFunctionCall(const ezHashedString& sName);

  static ezArrayPtr<Node*> GetChildren(Node* pNode);
  static ezArrayPtr<const Node*> GetChildren(const Node* pNode);

  void PrintGraph(ezDGMLGraph& graph) const;

  ezHybridArray<Output*, 8> m_OutputNodes;

private:
  ezStackAllocator<> m_Allocator;
};

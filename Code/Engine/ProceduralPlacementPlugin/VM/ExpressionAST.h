#pragma once

#include <ProceduralPlacementPlugin/Basics.h>
#include <Foundation/Memory/StackAllocator.h>
#include <Foundation/Types/Variant.h>

class ezDGMLGraph;

class EZ_PROCEDURALPLACEMENTPLUGIN_DLL ezExpressionAST
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

      FunctionCall,

      Count
    };

    static bool IsUnary(Enum nodeType);
    static bool IsBinary(Enum nodeType);
    static bool IsConstant(Enum nodeType);
    static bool IsInput(Enum nodeType);

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

    ezUInt32 m_uiIndex;
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

  ezExpressionAST::UnaryOperator* CreateUnaryOperator(NodeType::Enum type, Node* pOperand);
  ezExpressionAST::BinaryOperator* CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand);
  ezExpressionAST::Constant* CreateConstant(const ezVariant& value);
  ezExpressionAST::Input* CreateInput(ezUInt32 uiIndex);
  ezExpressionAST::FunctionCall* CreateFunctionCall(const char* szName);

  void PrintGraph(ezDGMLGraph& graph) const;

  ezHybridArray<Node*, 8> m_OutputNodes;

private:
  ezStackAllocator<> m_Allocator;
};

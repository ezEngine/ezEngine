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

      // Ternary
      Select,

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
    ezEnum<NodeType> m_Type;
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

  struct Select : public Node
  {
    Node* m_pCondition = nullptr;
    Node* m_pTrueOperand = nullptr;
    Node* m_pFalseOperand = nullptr;
  };

  struct Constant : public Node
  {
    ezVariant m_Value;
  };

  struct Input : public Node
  {
    ezHashedString m_sName;
  };

  struct Output : public Node
  {
    ezHashedString m_sName;
    Node* m_pExpression = nullptr;
  };

  struct FunctionCall : public Node
  {
    ezHashedString m_sName;
    ezHybridArray<Node*, 8> m_Arguments;
  };

public:
  ezExpressionAST();
  ~ezExpressionAST();

  UnaryOperator* CreateUnaryOperator(NodeType::Enum type, Node* pOperand);
  BinaryOperator* CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand);
  Select* CreateSelect(Node* pCondition, Node* pTrueOperand, Node* pFalseOperand);
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

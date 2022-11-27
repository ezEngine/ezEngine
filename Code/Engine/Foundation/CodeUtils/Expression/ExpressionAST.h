#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Memory/StackAllocator.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Variant.h>

class ezDGMLGraph;

class EZ_FOUNDATION_DLL ezExpressionAST
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
      Saturate,
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
      FirstTernary,
      Clamp,
      Select,
      LastTernary,

      // Constant
      Constant,

      // Input
      Input,

      // Output
      Output,

      FunctionCall,

      Count
    };

    static bool IsUnary(Enum nodeType);
    static bool IsBinary(Enum nodeType);
    static bool IsTernary(Enum nodeType);
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

  struct TernaryOperator : public Node
  {
    Node* m_pFirstOperand = nullptr;
    Node* m_pSecondOperand = nullptr;
    Node* m_pThirdOperand = nullptr;
  };

  struct Constant : public Node
  {
    ezVariant m_Value;
    ezProcessingStream::DataType m_DataType;
  };

  struct Input : public Node
  {
    ezHashedString m_sName;
    ezProcessingStream::DataType m_DataType;
  };

  struct Output : public Node
  {
    ezHashedString m_sName;
    ezProcessingStream::DataType m_DataType;
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
  TernaryOperator* CreateTernaryOperator(NodeType::Enum type, Node* pFirstOperand, Node* pSecondOperand, Node* pThirdOperand);
  Constant* CreateConstant(const ezVariant& value);
  Input* CreateInput(const ezHashedString& sName, ezProcessingStream::DataType dataType);
  Output* CreateOutput(const ezHashedString& sName, ezProcessingStream::DataType dataType, Node* pExpression);
  FunctionCall* CreateFunctionCall(const ezHashedString& sName);

  static ezArrayPtr<Node*> GetChildren(Node* pNode);
  static ezArrayPtr<const Node*> GetChildren(const Node* pNode);

  void PrintGraph(ezDGMLGraph& graph) const;

  ezHybridArray<Output*, 8> m_OutputNodes;

  // Transforms
  Node* ReplaceUnsupportedInstructions(Node* pNode);
  Node* FoldConstants(Node* pNode);

private:
  ezStackAllocator<> m_Allocator;
};

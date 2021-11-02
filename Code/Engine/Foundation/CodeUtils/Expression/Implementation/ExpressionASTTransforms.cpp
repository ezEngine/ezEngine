#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>

ezExpressionAST::Node* ezExpressionAST::ReplaceUnsupportedInstructions(Node* pNode)
{
  NodeType::Enum nodeType = pNode->m_Type;
  if (nodeType == NodeType::Negate)
  {
    auto pZero = CreateConstant(0.0f);
    auto pOperand = static_cast<UnaryOperator*>(pNode)->m_pOperand;
    return CreateBinaryOperator(NodeType::Subtract, pZero, pOperand);
  }

  return pNode;
}

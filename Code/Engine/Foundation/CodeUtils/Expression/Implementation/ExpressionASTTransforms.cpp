#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>

ezExpressionAST::Node* ezExpressionAST::ReplaceUnsupportedInstructions(Node* pNode)
{
  NodeType::Enum nodeType = pNode->m_Type;
  if (nodeType == NodeType::Negate)
  {
    auto pZero = CreateConstant(0.0f);
    auto pValue = static_cast<UnaryOperator*>(pNode)->m_pOperand;
    return CreateBinaryOperator(NodeType::Subtract, pZero, pValue);
  }
  else if (nodeType == NodeType::Saturate)
  {
    auto pZero = CreateConstant(0.0f);
    auto pOne = CreateConstant(1.0f);
    auto pValue = static_cast<UnaryOperator*>(pNode)->m_pOperand;
    return CreateBinaryOperator(NodeType::Max, pZero, CreateBinaryOperator(NodeType::Min, pOne, pValue));
  }
  else if (nodeType == NodeType::Clamp)
  {
    auto pTernaryNode = static_cast<TernaryOperator*>(pNode);
    auto pValue = pTernaryNode->m_pFirstOperand;
    auto pMinValue = pTernaryNode->m_pSecondOperand;
    auto pMaxValue = pTernaryNode->m_pThirdOperand;    
    return CreateBinaryOperator(NodeType::Max, pMinValue, CreateBinaryOperator(NodeType::Min, pMaxValue, pValue));
  }

  return pNode;
}

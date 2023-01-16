#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>

ezExpressionAST::Node* ezExpressionAST::ReplaceUnsupportedInstructions(Node* pNode)
{
  NodeType::Enum nodeType = pNode->m_Type;
  if (nodeType == NodeType::Negate)
  {
    auto pUnaryNode = static_cast<const UnaryOperator*>(pNode);
    if (NodeType::IsConstant(pUnaryNode->m_pOperand->m_Type))
    {
      auto pConstantNode = static_cast<Constant*>(pUnaryNode->m_pOperand);
      const float fValue = pConstantNode->m_Value.Get<float>();
      return CreateConstant(-fValue);
    }
    else
    {
      auto pZero = CreateConstant(0.0f);
      auto pValue = pUnaryNode->m_pOperand;
      return CreateBinaryOperator(NodeType::Subtract, pZero, pValue);
    }
  }
  else if (nodeType == NodeType::Saturate)
  {
    auto pUnaryNode = static_cast<const UnaryOperator*>(pNode);
    if (NodeType::IsConstant(pUnaryNode->m_pOperand->m_Type))
    {
      auto pConstantNode = static_cast<Constant*>(pUnaryNode->m_pOperand);
      const float fValue = pConstantNode->m_Value.Get<float>();
      return CreateConstant(ezMath::Saturate(fValue));
    }
    else
    {
      auto pZero = CreateConstant(0.0f);
      auto pOne = CreateConstant(1.0f);
      auto pValue = static_cast<const UnaryOperator*>(pNode)->m_pOperand;
      return CreateBinaryOperator(NodeType::Max, pZero, CreateBinaryOperator(NodeType::Min, pOne, pValue));
    }
  }
  else if (nodeType == NodeType::Clamp)
  {
    auto pTernaryNode = static_cast<const TernaryOperator*>(pNode);
    auto pValue = pTernaryNode->m_pFirstOperand;
    auto pMinValue = pTernaryNode->m_pSecondOperand;
    auto pMaxValue = pTernaryNode->m_pThirdOperand;
    return CreateBinaryOperator(NodeType::Max, pMinValue, CreateBinaryOperator(NodeType::Min, pMaxValue, pValue));
  }

  return pNode;
}

//////////////////////////////////////////////////////////////////////////

ezExpressionAST::Node* ezExpressionAST::FoldConstants(Node* pNode)
{
  NodeType::Enum nodeType = pNode->m_Type;
  if (NodeType::IsUnary(nodeType))
  {
    auto pUnaryNode = static_cast<const UnaryOperator*>(pNode);
    if (NodeType::IsConstant(pUnaryNode->m_pOperand->m_Type))
    {
      auto pConstantNode = static_cast<Constant*>(pUnaryNode->m_pOperand);
      const float fValue = pConstantNode->m_Value.Get<float>();

      switch (nodeType)
      {
        case NodeType::Negate:
          return CreateConstant(-fValue);
        case NodeType::Absolute:
          return CreateConstant(ezMath::Abs(fValue));
        case NodeType::Saturate:
          return CreateConstant(ezMath::Saturate(fValue));
        case NodeType::Sqrt:
          return CreateConstant(ezMath::Sqrt(fValue));
        case NodeType::Sin:
          return CreateConstant(ezMath::Sin(ezAngle::Radian(fValue)));
        case NodeType::Cos:
          return CreateConstant(ezMath::Cos(ezAngle::Radian(fValue)));
        case NodeType::Tan:
          return CreateConstant(ezMath::Tan(ezAngle::Radian(fValue)));
        case NodeType::ASin:
          return CreateConstant(ezMath::ASin(fValue).GetRadian());
        case NodeType::ACos:
          return CreateConstant(ezMath::ACos(fValue).GetRadian());
        case NodeType::ATan:
          return CreateConstant(ezMath::ATan(fValue).GetRadian());

        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
          return pNode;
      }
    }
  }
  else if (NodeType::IsBinary(nodeType))
  {
    auto pBinaryNode = static_cast<const BinaryOperator*>(pNode);
    const bool bLeftIsConstant = NodeType::IsConstant(pBinaryNode->m_pLeftOperand->m_Type);
    const bool bRightIsConstant = NodeType::IsConstant(pBinaryNode->m_pRightOperand->m_Type);
    if (bLeftIsConstant && bRightIsConstant)
    {
      auto pLeftConstant = static_cast<const Constant*>(pBinaryNode->m_pLeftOperand);
      auto pRightConstant = static_cast<const Constant*>(pBinaryNode->m_pRightOperand);
      const float fLeftValue = pLeftConstant->m_Value.Get<float>();
      const float fRightValue = pRightConstant->m_Value.Get<float>();

      switch (nodeType)
      {
        case NodeType::Add:
          return CreateConstant(fLeftValue + fRightValue);
        case NodeType::Subtract:
          return CreateConstant(fLeftValue - fRightValue);
        case NodeType::Multiply:
          return CreateConstant(fLeftValue * fRightValue);
        case NodeType::Divide:
          return CreateConstant(fLeftValue / fRightValue);
        case NodeType::Min:
          return CreateConstant(ezMath::Min(fLeftValue, fRightValue));
        case NodeType::Max:
          return CreateConstant(ezMath::Max(fLeftValue, fRightValue));

        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
          return pNode;
      }
    }
    else if (bRightIsConstant)
    {
      auto pOperand = pBinaryNode->m_pLeftOperand;
      auto pConstantNode = static_cast<Constant*>(pBinaryNode->m_pRightOperand);
      const float fValue = pConstantNode->m_Value.Get<float>();

      switch (nodeType)
      {
        case NodeType::Add:
          return CreateBinaryOperator(ezExpressionAST::NodeType::Add, pConstantNode, pOperand);
        case NodeType::Subtract:
          return CreateBinaryOperator(ezExpressionAST::NodeType::Add, CreateConstant(-fValue), pOperand);
        case NodeType::Multiply:
          return CreateBinaryOperator(ezExpressionAST::NodeType::Multiply, pConstantNode, pOperand);
        case NodeType::Divide:
          return CreateBinaryOperator(ezExpressionAST::NodeType::Multiply, CreateConstant(1.0f / fValue), pOperand);
        case NodeType::Min:
          return CreateBinaryOperator(ezExpressionAST::NodeType::Min, pConstantNode, pOperand);
        case NodeType::Max:
          return CreateBinaryOperator(ezExpressionAST::NodeType::Max, pConstantNode, pOperand);

        default:
          EZ_ASSERT_NOT_IMPLEMENTED;
          return pNode;
      }
    }
  }
  else if (NodeType::IsTernary(nodeType))
  {
    auto pTernaryNode = static_cast<const TernaryOperator*>(pNode);
    const bool bFirstIsConstant = NodeType::IsConstant(pTernaryNode->m_pFirstOperand->m_Type);
    const bool bSecondIsConstant = NodeType::IsConstant(pTernaryNode->m_pSecondOperand->m_Type);
    const bool bThirdIsConstant = NodeType::IsConstant(pTernaryNode->m_pThirdOperand->m_Type);
  }

  return pNode;
}


EZ_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionASTTransforms);

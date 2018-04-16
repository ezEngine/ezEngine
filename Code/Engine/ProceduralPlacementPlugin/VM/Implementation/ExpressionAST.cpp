#include <PCH.h>
#include <ProceduralPlacementPlugin/VM/ExpressionAST.h>
#include <Utilities/DGML/DGMLWriter.h>

//static
bool ezExpressionAST::NodeType::IsUnary(Enum nodeType)
{
  return nodeType > FirstUnary && nodeType < LastUnary;
}

//static
bool ezExpressionAST::NodeType::IsBinary(Enum nodeType)
{
  return nodeType > FirstBinary && nodeType < LastBinary;
}

//static
bool ezExpressionAST::NodeType::IsConstant(Enum nodeType)
{
  return nodeType == FloatConstant;
}

//static
bool ezExpressionAST::NodeType::IsInput(Enum nodeType)
{
  return nodeType == FloatInput;
}

namespace
{
  static const char* s_szNodeTypeNames[] =
  {
    "Invalid",

    // Unary
    "",
    "Negate",
    "Absolute",
    "Sqrt",
    "",

    // Binary
    "",
    "Add",
    "Subtract",
    "Multiply",
    "Divide",
    "Min",
    "Max",
    "",

    // Constant
    "FloatConstant",

    // Input
    "FloatInput",

    "FunctionCall"
  };

  EZ_CHECK_AT_COMPILETIME_MSG(EZ_ARRAY_SIZE(s_szNodeTypeNames) == ezExpressionAST::NodeType::Count, "Node name array size does not match node type count");
}

//static
const char* ezExpressionAST::NodeType::GetName(Enum nodeType)
{
  EZ_ASSERT_DEBUG(nodeType >= 0 && nodeType < EZ_ARRAY_SIZE(s_szNodeTypeNames), "Out of bounds access");
  return s_szNodeTypeNames[nodeType];
}

//////////////////////////////////////////////////////////////////////////

ezExpressionAST::ezExpressionAST()
  : m_Allocator("Expression AST", ezFoundation::GetAlignedAllocator())
{
}

ezExpressionAST::~ezExpressionAST()
{
}

ezExpressionAST::ezExpressionAST::UnaryOperator* ezExpressionAST::CreateUnaryOperator(NodeType::Enum type, Node* pOperand)
{
  auto pUnaryOperator = EZ_NEW(&m_Allocator, UnaryOperator, type);
  pUnaryOperator->m_pOperand = pOperand;

  return pUnaryOperator;
}

ezExpressionAST::ezExpressionAST::BinaryOperator* ezExpressionAST::CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand)
{
  auto pBinaryOperator = EZ_NEW(&m_Allocator, BinaryOperator, type);
  pBinaryOperator->m_pLeftOperand = pLeftOperand;
  pBinaryOperator->m_pRightOperand = pRightOperand;

  return pBinaryOperator;
}

ezExpressionAST::Constant* ezExpressionAST::CreateConstant(const ezVariant& value)
{
  auto pConstant = EZ_NEW(&m_Allocator, Constant, NodeType::FloatConstant);
  pConstant->m_Value = value;

  return pConstant;
}

ezExpressionAST::ezExpressionAST::Input* ezExpressionAST::CreateInput(ezUInt32 uiIndex)
{
  auto pInput = EZ_NEW(&m_Allocator, Input, NodeType::FloatInput);
  pInput->m_uiIndex = uiIndex;

  return pInput;
}

ezExpressionAST::ezExpressionAST::FunctionCall* ezExpressionAST::CreateFunctionCall(const char* szName)
{
  auto pFunctionCall = EZ_NEW(&m_Allocator, FunctionCall, NodeType::FunctionCall);
  pFunctionCall->m_sName.Assign(szName);

  return pFunctionCall;
}

namespace
{
  struct NodeInfo
  {
    EZ_DECLARE_POD_TYPE();

    const ezExpressionAST::Node* m_pNode;
    ezUInt32 m_uiParentGraphNode;
  };
}

void ezExpressionAST::PrintGraph(ezDGMLGraph& graph) const
{
  ezHybridArray<NodeInfo, 64> nodeStack;

  ezStringBuilder sTmp;
  for (ezUInt32 i = 0; i < m_OutputNodes.GetCount(); ++i)
  {
    Node* pOutputNode = m_OutputNodes[i];
    if (pOutputNode == nullptr)
      continue;

    sTmp.Format("Output{0}", i);

    ezUInt32 uiGraphNode = graph.AddNode(sTmp, ezColor::CornflowerBlue);

    nodeStack.PushBack({ pOutputNode, uiGraphNode });
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
        if (NodeType::IsConstant(nodeType))
        {
          sTmp.AppendFormat(": {0}", static_cast<const Constant*>(currentNodeInfo.m_pNode)->m_Value.ConvertTo<ezString>());
        }
        else if (NodeType::IsInput(nodeType))
        {
          sTmp.AppendFormat(": {0}", static_cast<const Input*>(currentNodeInfo.m_pNode)->m_uiIndex);
        }
        else if (nodeType == NodeType::FunctionCall)
        {
          sTmp.Append(": ", static_cast<const FunctionCall*>(currentNodeInfo.m_pNode)->m_sName);
        }

        uiGraphNode = graph.AddNode(sTmp);
        nodeCache.Insert(currentNodeInfo.m_pNode, uiGraphNode);

        // push children
        if (NodeType::IsUnary(nodeType))
        {
          auto pUnaryOperator = static_cast<const UnaryOperator*>(currentNodeInfo.m_pNode);
          nodeStack.PushBack({ pUnaryOperator->m_pOperand, uiGraphNode });
        }
        else if (NodeType::IsBinary(nodeType))
        {
          auto pBinaryOperator = static_cast<const BinaryOperator*>(currentNodeInfo.m_pNode);
          nodeStack.PushBack({ pBinaryOperator->m_pLeftOperand, uiGraphNode });
          nodeStack.PushBack({ pBinaryOperator->m_pRightOperand, uiGraphNode });
        }
        else if (nodeType == NodeType::FunctionCall)
        {
          auto pFunctionCall = static_cast<const FunctionCall*>(currentNodeInfo.m_pNode);
          for (auto arg : pFunctionCall->m_Arguments)
          {
            nodeStack.PushBack({ arg, uiGraphNode });
          }
        }
      }
    }
    else
    {
      uiGraphNode = graph.AddNode("Invalid", ezColor::OrangeRed);
    }

    graph.AddConnection(uiGraphNode, currentNodeInfo.m_uiParentGraphNode);
  }
}

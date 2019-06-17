#include <ProcGenPluginPCH.h>

#include <Foundation/Utilities/DGMLWriter.h>
#include <ProcGenPlugin/VM/ExpressionAST.h>

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
bool ezExpressionAST::NodeType::IsConstant(Enum nodeType)
{
  return nodeType == FloatConstant;
}

// static
bool ezExpressionAST::NodeType::IsInput(Enum nodeType)
{
  return nodeType == FloatInput;
}

// static
bool ezExpressionAST::NodeType::IsOutput(Enum nodeType)
{
  return nodeType == FloatOutput;
}

namespace
{
  static const char* s_szNodeTypeNames[] = {"Invalid",

    // Unary
    "", "Negate", "Absolute", "Sqrt", "Sin", "Cos", "Tan", "ASin", "ACos", "ATan", "",

    // Binary
    "", "Add", "Subtract", "Multiply", "Divide", "Min", "Max", "",

    // Constant
    "FloatConstant",

    // Input
    "FloatInput",

    // Output
    "FloatOutput",

    "FunctionCall"};

  EZ_CHECK_AT_COMPILETIME_MSG(
    EZ_ARRAY_SIZE(s_szNodeTypeNames) == ezExpressionAST::NodeType::Count, "Node name array size does not match node type count");
} // namespace

// static
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

ezExpressionAST::~ezExpressionAST() {}

ezExpressionAST::UnaryOperator* ezExpressionAST::CreateUnaryOperator(NodeType::Enum type, Node* pOperand)
{
  auto pUnaryOperator = EZ_NEW(&m_Allocator, UnaryOperator, type);
  pUnaryOperator->m_pOperand = pOperand;

  return pUnaryOperator;
}

ezExpressionAST::BinaryOperator* ezExpressionAST::CreateBinaryOperator(NodeType::Enum type, Node* pLeftOperand, Node* pRightOperand)
{
  auto pBinaryOperator = EZ_NEW(&m_Allocator, BinaryOperator, type);
  pBinaryOperator->m_pLeftOperand = pLeftOperand;
  pBinaryOperator->m_pRightOperand = pRightOperand;

  return pBinaryOperator;
}

ezExpressionAST::Constant* ezExpressionAST::CreateConstant(const ezVariant& value)
{
  EZ_ASSERT_DEV(value.IsA<float>(), "value needs to be float");

  auto pConstant = EZ_NEW(&m_Allocator, Constant, NodeType::FloatConstant);
  pConstant->m_Value = value;

  return pConstant;
}

ezExpressionAST::Input* ezExpressionAST::CreateInput(const ezHashedString& sName)
{
  auto pInput = EZ_NEW(&m_Allocator, Input, NodeType::FloatInput);
  pInput->m_sName = sName;

  return pInput;
}

ezExpressionAST::Output* ezExpressionAST::CreateOutput(const ezHashedString& sName, Node* pExpression)
{
  auto pOutput = EZ_NEW(&m_Allocator, Output, NodeType::FloatOutput);
  pOutput->m_sName = sName;
  pOutput->m_pExpression = pExpression;

  return pOutput;
}

ezExpressionAST::FunctionCall* ezExpressionAST::CreateFunctionCall(const ezHashedString& sName)
{
  auto pFunctionCall = EZ_NEW(&m_Allocator, FunctionCall, NodeType::FunctionCall);
  pFunctionCall->m_sName = sName;

  return pFunctionCall;
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
  else if (NodeType::IsOutput(nodeType))
  {
    auto& pChild = static_cast<Output*>(pNode)->m_pExpression;
    return ezMakeArrayPtr(&pChild, 1);
  }
  else if (nodeType == NodeType::FunctionCall)
  {
    auto& args = static_cast<FunctionCall*>(pNode)->m_Arguments;
    return args;
  }

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
  else if (NodeType::IsOutput(nodeType))
  {
    auto& pChild = static_cast<const Output*>(pNode)->m_pExpression;
    return ezMakeArrayPtr((const Node**)&pChild, 1);
  }
  else if (nodeType == NodeType::FunctionCall)
  {
    auto& args = static_cast<const FunctionCall*>(pNode)->m_Arguments;
    return ezArrayPtr<const Node*>((const Node**)args.GetData(), args.GetCount());
  }

  return ezArrayPtr<const Node*>();
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

    sTmp.Format("{0}: {1}", NodeType::GetName(pOutputNode->m_Type), pOutputNode->m_sName);

    ezDGMLGraph::NodeDesc nd;
    nd.m_Color = ezColor::LightBlue;
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
        ezColor color = ezColor::White;

        if (NodeType::IsConstant(nodeType))
        {
          sTmp.AppendFormat(": {0}", static_cast<const Constant*>(currentNodeInfo.m_pNode)->m_Value.ConvertTo<ezString>());
        }
        else if (NodeType::IsInput(nodeType))
        {
          sTmp.Append(": ", static_cast<const Input*>(currentNodeInfo.m_pNode)->m_sName);
          color = ezColor::LightGreen;
        }
        else if (nodeType == NodeType::FunctionCall)
        {
          sTmp.Append(": ", static_cast<const FunctionCall*>(currentNodeInfo.m_pNode)->m_sName);
          color = ezColor::LightGoldenRodYellow;
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

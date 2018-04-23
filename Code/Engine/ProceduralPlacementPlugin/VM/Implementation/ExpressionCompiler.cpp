#include <PCH.h>
#include <ProceduralPlacementPlugin/VM/ExpressionCompiler.h>

namespace
{
  static ezExpressionByteCode::OpCode::Enum NodeTypeToOpCode(ezExpressionAST::NodeType::Enum nodeType)
  {
    switch (nodeType)
    {
    case ezExpressionAST::NodeType::Absolute:
      return ezExpressionByteCode::OpCode::Abs_R;
    case ezExpressionAST::NodeType::Sqrt:
      return ezExpressionByteCode::OpCode::Sqrt_R;
    case ezExpressionAST::NodeType::Add:
      return ezExpressionByteCode::OpCode::Add_RR;
    case ezExpressionAST::NodeType::Subtract:
      return ezExpressionByteCode::OpCode::Sub_RR;
    case ezExpressionAST::NodeType::Multiply:
      return ezExpressionByteCode::OpCode::Mul_RR;
    case ezExpressionAST::NodeType::Divide:
      return ezExpressionByteCode::OpCode::Div_RR;
    case ezExpressionAST::NodeType::Min:
      return ezExpressionByteCode::OpCode::Min_RR;
    case ezExpressionAST::NodeType::Max:
      return ezExpressionByteCode::OpCode::Max_RR;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return ezExpressionByteCode::OpCode::FirstUnary;
    }
  }
}

ezExpressionCompiler::ezExpressionCompiler()
{

}

ezExpressionCompiler::~ezExpressionCompiler()
{

}

ezResult ezExpressionCompiler::Compile(ezExpressionAST& ast, ezExpressionByteCode& out_byteCode)
{
  if (BuildNodeInstructions(ast).Failed())
    return EZ_FAILURE;

  if (UpdateRegisterLifetime(ast).Failed())
    return EZ_FAILURE;

  if (AssignRegisters().Failed())
    return EZ_FAILURE;

  if (GenerateByteCode(ast, out_byteCode).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezExpressionCompiler::BuildNodeInstructions(const ezExpressionAST& ast)
{
  m_NodeInstructions.Clear();
  m_NodeToRegisterIndex.Clear();
  m_LiveIntervals.Clear();
  ezUInt32 uiNextRegisterIndex = 0;

  // Build node instruction order aka post order tree traversal and assign virtual register indices and
  // determine their lifetime start.
  for (const ezExpressionAST::Node* pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      continue;

    m_NodeStack.Clear();

    m_NodeStack.PushBack(pOutputNode);

    while (!m_NodeStack.IsEmpty())
    {
      auto pCurrentNode = m_NodeStack.PeekBack();
      m_NodeStack.PopBack();

      if (pCurrentNode == nullptr)
      {
        return EZ_FAILURE;
      }

      if (!m_NodeToRegisterIndex.Contains(pCurrentNode))
      {
        m_NodeInstructions.PushBack(pCurrentNode);

        m_NodeToRegisterIndex.Insert(pCurrentNode, uiNextRegisterIndex);
        ++uiNextRegisterIndex;

        ezUInt32 uiCurrentInstructionIndex = m_NodeInstructions.GetCount() - 1;
        m_LiveIntervals.PushBack({ uiCurrentInstructionIndex, uiCurrentInstructionIndex, pCurrentNode });
        EZ_ASSERT_DEV(m_LiveIntervals.GetCount() == uiNextRegisterIndex, "Implementation error");

        if (ezExpressionAST::NodeType::IsBinary(pCurrentNode->m_Type))
        {
          // Do not push the left operand if it is a constant, we don't want a separate mov instruction for it
          // since all binary operators can take a constant as left operand in place.
          auto pBinary = static_cast<const ezExpressionAST::BinaryOperator*>(pCurrentNode);
          bool bLeftIsConstant = ezExpressionAST::NodeType::IsConstant(pBinary->m_pLeftOperand->m_Type);
          if (!bLeftIsConstant)
          {
            m_NodeStack.PushBack(pBinary->m_pLeftOperand);
          }

          m_NodeStack.PushBack(pBinary->m_pRightOperand);
        }
        else
        {
          auto children = ezExpressionAST::GetChildren(pCurrentNode);
          for (auto pChild : children)
          {
            m_NodeStack.PushBack(pChild);
          }
        }
      }
    }
  }

  if (m_NodeInstructions.IsEmpty())
  {
    // Nothing to compile
    return EZ_FAILURE;
  }

  // Reverse the order of instructions, they have been written in a stack based fashion.
  // Strictly speaking this would not be necessary but makes the rest easier to understand.
  {
    const ezExpressionAST::Node** pLast = &m_NodeInstructions.PeekBack();
    const ezExpressionAST::Node** pFirst = &m_NodeInstructions[0];
    while (pLast > pFirst)
    {
      ezMath::Swap(*pFirst, *pLast);
      --pLast;
      ++pFirst;
    }

    // Also reverse lifetime
    ezUInt32 uiLastInstructionIndex = m_NodeInstructions.GetCount() - 1;
    for (auto& liveRegister : m_LiveIntervals)
    {
      liveRegister.m_uiStart = uiLastInstructionIndex - liveRegister.m_uiStart;
      liveRegister.m_uiEnd = uiLastInstructionIndex - liveRegister.m_uiEnd;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezExpressionCompiler::UpdateRegisterLifetime(const ezExpressionAST& ast)
{
  ezUInt32 uiInstructionIndex = 0;
  ezUInt32 uiNumInstructions = m_NodeInstructions.GetCount();
  for (; uiInstructionIndex < uiNumInstructions; ++uiInstructionIndex)
  {
    auto pCurrentNode = m_NodeInstructions[uiInstructionIndex];

    auto children = ezExpressionAST::GetChildren(pCurrentNode);
    for (auto pChild : children)
    {
      ezUInt32 uiRegisterIndex = m_NodeToRegisterIndex[pChild];
      auto& liveRegister = m_LiveIntervals[uiRegisterIndex];

      EZ_ASSERT_DEV(liveRegister.m_uiEnd <= uiInstructionIndex, "Implementation error");
      liveRegister.m_uiEnd = uiInstructionIndex;
    }
  }

  for (auto pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      continue;

    ezUInt32 uiRegisterIndex = m_NodeToRegisterIndex[pOutputNode];
    auto& liveRegister = m_LiveIntervals[uiRegisterIndex];

    EZ_ASSERT_DEV(liveRegister.m_uiEnd <= uiInstructionIndex, "Implementation error");
    liveRegister.m_uiEnd = uiInstructionIndex;

    uiInstructionIndex++;
  }

  return EZ_SUCCESS;
}

ezResult ezExpressionCompiler::AssignRegisters()
{
  // This is an implementation of the linear scan register allocation algorithm without spilling
  // https://www2.seas.gwu.edu/~hchoi/teaching/cs160d/linearscan.pdf

  // Sort register lifetime by start index
  m_LiveIntervals.Sort([](const LiveInterval& a, const LiveInterval& b) { return a.m_uiStart < b.m_uiStart; });

  // Assign registers
  ezHybridArray<LiveInterval, 64> activeIntervals;
  ezHybridArray<ezUInt32, 64> freeRegisters;

  for (auto& liveInterval : m_LiveIntervals)
  {
    // Expire old intervals
    for (ezUInt32 uiActiveIndex = activeIntervals.GetCount(); uiActiveIndex-- > 0; )
    {
      auto& activeInterval = activeIntervals[uiActiveIndex];
      if (activeInterval.m_uiEnd <= liveInterval.m_uiStart)
      {
        ezUInt32 uiRegisterIndex = m_NodeToRegisterIndex[activeInterval.m_pNode];
        freeRegisters.PushBack(uiRegisterIndex);

        activeIntervals.RemoveAt(uiActiveIndex);
      }
    }

    // Allocate register
    ezUInt32 uiNewRegister = 0;
    if (!freeRegisters.IsEmpty())
    {
      uiNewRegister = freeRegisters.PeekBack();
      freeRegisters.PopBack();
    }
    else
    {
      uiNewRegister = activeIntervals.GetCount();
    }
    m_NodeToRegisterIndex[liveInterval.m_pNode] = uiNewRegister;

    activeIntervals.PushBack(liveInterval);
  }

  return EZ_SUCCESS;
}

ezResult ezExpressionCompiler::GenerateByteCode(const ezExpressionAST& ast, ezExpressionByteCode& out_byteCode)
{
  auto& byteCode = out_byteCode.m_ByteCode;

  ezUInt32 uiMaxRegisterIndex = 0;

  for (auto pCurrentNode : m_NodeInstructions)
  {
    ezUInt32 uiTargetRegister = m_NodeToRegisterIndex[pCurrentNode];
    uiMaxRegisterIndex = ezMath::Max(uiMaxRegisterIndex, uiTargetRegister);

    ezExpressionAST::NodeType::Enum nodeType = pCurrentNode->m_Type;
    if (ezExpressionAST::NodeType::IsUnary(nodeType))
    {
      auto pUnary = static_cast<const ezExpressionAST::UnaryOperator*>(pCurrentNode);

      byteCode.PushBack(NodeTypeToOpCode(nodeType));
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(m_NodeToRegisterIndex[pUnary->m_pOperand]);
    }
    else if (ezExpressionAST::NodeType::IsBinary(nodeType))
    {
      auto pBinary = static_cast<const ezExpressionAST::BinaryOperator*>(pCurrentNode);
      bool bLeftIsConstant = ezExpressionAST::NodeType::IsConstant(pBinary->m_pLeftOperand->m_Type);
      ezExpressionByteCode::OpCode::Enum opCode = NodeTypeToOpCode(nodeType);
      ezUInt32 uiConstantValue = 0;

      if (bLeftIsConstant)
      {
        // Op code for constant register combination is always +1 of regular op code.
        opCode = static_cast<ezExpressionByteCode::OpCode::Enum>(opCode + 1);

        auto pConstant = static_cast<const ezExpressionAST::Constant*>(pBinary->m_pLeftOperand);
        uiConstantValue = *reinterpret_cast<const ezUInt32*>(&pConstant->m_Value.Get<float>());
      }

      byteCode.PushBack(opCode);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(bLeftIsConstant ? uiConstantValue : m_NodeToRegisterIndex[pBinary->m_pLeftOperand]);
      byteCode.PushBack(m_NodeToRegisterIndex[pBinary->m_pRightOperand]);
    }
    else if (ezExpressionAST::NodeType::IsConstant(nodeType))
    {
      EZ_ASSERT_DEV(nodeType == ezExpressionAST::NodeType::FloatConstant, "Only floats are supported");
      auto pConstant = static_cast<const ezExpressionAST::Constant*>(pCurrentNode);
      float fValue = pConstant->m_Value.Get<float>();

      byteCode.PushBack(ezExpressionByteCode::OpCode::Mov_C);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(*reinterpret_cast<ezUInt32*>(&fValue));
    }
    else if (ezExpressionAST::NodeType::IsInput(nodeType))
    {
      byteCode.PushBack(ezExpressionByteCode::OpCode::Mov_I);
      byteCode.PushBack(uiTargetRegister);
      byteCode.PushBack(static_cast<const ezExpressionAST::Input*>(pCurrentNode)->m_uiIndex);
    }
    else if (nodeType == ezExpressionAST::NodeType::FunctionCall)
    {
      auto pFunctionCall = static_cast<const ezExpressionAST::FunctionCall*>(pCurrentNode);

      byteCode.PushBack(ezExpressionByteCode::OpCode::Call);
      byteCode.PushBack(pFunctionCall->m_sName.GetHash());
      byteCode.PushBack(uiTargetRegister);

      byteCode.PushBack(pFunctionCall->m_Arguments.GetCount());
      for (auto pArg : pFunctionCall->m_Arguments)
      {
        ezUInt32 uiArgRegister = m_NodeToRegisterIndex[pArg];
        byteCode.PushBack(uiArgRegister);
      }
    }
  }

  ezUInt32 uiNumInstructions = m_NodeInstructions.GetCount();

  // Move to outputs
  for (ezUInt32 uiOutputIndex = 0; uiOutputIndex < ast.m_OutputNodes.GetCount(); ++uiOutputIndex)
  {
    auto pOutputNode = ast.m_OutputNodes[uiOutputIndex];
    if (pOutputNode == nullptr)
      continue;

    byteCode.PushBack(ezExpressionByteCode::OpCode::Mov_O);
    byteCode.PushBack(uiOutputIndex);
    byteCode.PushBack(m_NodeToRegisterIndex[pOutputNode]);

    ++uiNumInstructions;
  }

  out_byteCode.m_uiNumInstructions = uiNumInstructions;
  out_byteCode.m_uiNumTempRegisters = uiMaxRegisterIndex + 1;

  return EZ_SUCCESS;
}

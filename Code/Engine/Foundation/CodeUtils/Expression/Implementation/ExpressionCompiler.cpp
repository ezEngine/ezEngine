#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
#define ADD_OFFSET(opCode) static_cast<ezExpressionByteCode::OpCode::Enum>((opCode) + uiOffset)

  static ezExpressionByteCode::OpCode::Enum NodeTypeToOpCode(ezExpressionAST::NodeType::Enum nodeType, ezExpressionAST::DataType::Enum dataType, bool bRightIsConstant)
  {
    const ezExpression::RegisterType::Enum registerType = ezExpressionAST::DataType::GetRegisterType(dataType);
    const bool bFloat = registerType == ezExpression::RegisterType::Float;
    const bool bInt = registerType == ezExpression::RegisterType::Int;
    const ezUInt32 uiOffset = bRightIsConstant ? ezExpressionByteCode::OpCode::FirstBinaryWithConstant - ezExpressionByteCode::OpCode::FirstBinary : 0;

    switch (nodeType)
    {
      case ezExpressionAST::NodeType::Absolute:
        return bFloat ? ezExpressionByteCode::OpCode::AbsF_R : ezExpressionByteCode::OpCode::AbsI_R;
      case ezExpressionAST::NodeType::Sqrt:
        return ezExpressionByteCode::OpCode::SqrtF_R;

      case ezExpressionAST::NodeType::Exp:
        return ezExpressionByteCode::OpCode::ExpF_R;
      case ezExpressionAST::NodeType::Ln:
        return ezExpressionByteCode::OpCode::LnF_R;
      case ezExpressionAST::NodeType::Log2:
        return bFloat ? ezExpressionByteCode::OpCode::Log2F_R : ezExpressionByteCode::OpCode::Log2I_R;
      case ezExpressionAST::NodeType::Log10:
        return ezExpressionByteCode::OpCode::Log10F_R;
      case ezExpressionAST::NodeType::Pow2:
        return ezExpressionByteCode::OpCode::Pow2F_R;

      case ezExpressionAST::NodeType::Sin:
        return ezExpressionByteCode::OpCode::SinF_R;
      case ezExpressionAST::NodeType::Cos:
        return ezExpressionByteCode::OpCode::CosF_R;
      case ezExpressionAST::NodeType::Tan:
        return ezExpressionByteCode::OpCode::TanF_R;

      case ezExpressionAST::NodeType::ASin:
        return ezExpressionByteCode::OpCode::ASinF_R;
      case ezExpressionAST::NodeType::ACos:
        return ezExpressionByteCode::OpCode::ACosF_R;
      case ezExpressionAST::NodeType::ATan:
        return ezExpressionByteCode::OpCode::ATanF_R;

      case ezExpressionAST::NodeType::Round:
        return ezExpressionByteCode::OpCode::RoundF_R;
      case ezExpressionAST::NodeType::Floor:
        return ezExpressionByteCode::OpCode::FloorF_R;
      case ezExpressionAST::NodeType::Ceil:
        return ezExpressionByteCode::OpCode::CeilF_R;
      case ezExpressionAST::NodeType::Trunc:
        return ezExpressionByteCode::OpCode::TruncF_R;

      case ezExpressionAST::NodeType::BitwiseNot:
        return ezExpressionByteCode::OpCode::NotI_R;
      case ezExpressionAST::NodeType::LogicalNot:
        return ezExpressionByteCode::OpCode::NotB_R;

      case ezExpressionAST::NodeType::TypeConversion:
        return bFloat ? ezExpressionByteCode::OpCode::IToF_R : ezExpressionByteCode::OpCode::FToI_R;

      case ezExpressionAST::NodeType::Add:
        return ADD_OFFSET(bFloat ? ezExpressionByteCode::OpCode::AddF_RR : ezExpressionByteCode::OpCode::AddI_RR);
      case ezExpressionAST::NodeType::Subtract:
        return ADD_OFFSET(bFloat ? ezExpressionByteCode::OpCode::SubF_RR : ezExpressionByteCode::OpCode::SubI_RR);
      case ezExpressionAST::NodeType::Multiply:
        return ADD_OFFSET(bFloat ? ezExpressionByteCode::OpCode::MulF_RR : ezExpressionByteCode::OpCode::MulI_RR);
      case ezExpressionAST::NodeType::Divide:
        return ADD_OFFSET(bFloat ? ezExpressionByteCode::OpCode::DivF_RR : ezExpressionByteCode::OpCode::DivI_RR);
      case ezExpressionAST::NodeType::Min:
        return ADD_OFFSET(bFloat ? ezExpressionByteCode::OpCode::MinF_RR : ezExpressionByteCode::OpCode::MinI_RR);
      case ezExpressionAST::NodeType::Max:
        return ADD_OFFSET(bFloat ? ezExpressionByteCode::OpCode::MaxF_RR : ezExpressionByteCode::OpCode::MaxI_RR);

      case ezExpressionAST::NodeType::BitshiftLeft:
        return ADD_OFFSET(ezExpressionByteCode::OpCode::ShlI_RR);
      case ezExpressionAST::NodeType::BitshiftRight:
        return ADD_OFFSET(ezExpressionByteCode::OpCode::ShrI_RR);
      case ezExpressionAST::NodeType::BitwiseAnd:
        return ADD_OFFSET(ezExpressionByteCode::OpCode::AndI_RR);
      case ezExpressionAST::NodeType::BitwiseXor:
        return ADD_OFFSET(ezExpressionByteCode::OpCode::XorI_RR);
      case ezExpressionAST::NodeType::BitwiseOr:
        return ADD_OFFSET(ezExpressionByteCode::OpCode::OrI_RR);

      case ezExpressionAST::NodeType::Equal:
        if (bFloat)
          return ADD_OFFSET(ezExpressionByteCode::OpCode::EqF_RR);
        else if (bInt)
          return ADD_OFFSET(ezExpressionByteCode::OpCode::EqI_RR);
        else
          return ADD_OFFSET(ezExpressionByteCode::OpCode::EqB_RR);
      case ezExpressionAST::NodeType::NotEqual:
        if (bFloat)
          return ADD_OFFSET(ezExpressionByteCode::OpCode::NEqF_RR);
        else if (bInt)
          return ADD_OFFSET(ezExpressionByteCode::OpCode::NEqI_RR);
        else
          return ADD_OFFSET(ezExpressionByteCode::OpCode::NEqB_RR);
      case ezExpressionAST::NodeType::Less:
        return ADD_OFFSET(bFloat ? ezExpressionByteCode::OpCode::LtF_RR : ezExpressionByteCode::OpCode::LtI_RR);
      case ezExpressionAST::NodeType::LessEqual:
        return ADD_OFFSET(bFloat ? ezExpressionByteCode::OpCode::LEqF_RR : ezExpressionByteCode::OpCode::LEqI_RR);
      case ezExpressionAST::NodeType::Greater:
        return ADD_OFFSET(bFloat ? ezExpressionByteCode::OpCode::GtF_RR : ezExpressionByteCode::OpCode::GtI_RR);
      case ezExpressionAST::NodeType::GreaterEqual:
        return ADD_OFFSET(bFloat ? ezExpressionByteCode::OpCode::GEqF_RR : ezExpressionByteCode::OpCode::GEqI_RR);

      case ezExpressionAST::NodeType::LogicalAnd:
        return ADD_OFFSET(ezExpressionByteCode::OpCode::AndB_RR);
      case ezExpressionAST::NodeType::LogicalOr:
        return ADD_OFFSET(ezExpressionByteCode::OpCode::OrB_RR);

      case ezExpressionAST::NodeType::Select:
        if (bFloat)
          return ezExpressionByteCode::OpCode::SelF_RRR;
        else if (bInt)
          return ezExpressionByteCode::OpCode::SelI_RRR;
        else
          return ezExpressionByteCode::OpCode::SelB_RRR;

      case ezExpressionAST::NodeType::Constant:
        return ezExpressionByteCode::OpCode::MovX_C;
      case ezExpressionAST::NodeType::Input:
        return bFloat ? ezExpressionByteCode::OpCode::LoadF : ezExpressionByteCode::OpCode::LoadI;
      case ezExpressionAST::NodeType::Output:
        return bFloat ? ezExpressionByteCode::OpCode::StoreF : ezExpressionByteCode::OpCode::StoreI;
      case ezExpressionAST::NodeType::FunctionCall:
        return ezExpressionByteCode::OpCode::Call;
      case ezExpressionAST::NodeType::ConstructorCall:
        EZ_REPORT_FAILURE("Constructor calls should not exist anymore after AST transformations");
        return ezExpressionByteCode::OpCode::Nop;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        return ezExpressionByteCode::OpCode::Nop;
    }
  }

#undef ADD_OFFSET
} // namespace

ezExpressionCompiler::ezExpressionCompiler() = default;
ezExpressionCompiler::~ezExpressionCompiler() = default;

ezResult ezExpressionCompiler::Compile(ezExpressionAST& ref_ast, ezExpressionByteCode& out_byteCode, ezStringView sDebugAstOutputPath /*= ezStringView()*/)
{
  out_byteCode.Clear();

  EZ_SUCCEED_OR_RETURN(TransformAndOptimizeAST(ref_ast, sDebugAstOutputPath));
  EZ_SUCCEED_OR_RETURN(BuildNodeInstructions(ref_ast));
  EZ_SUCCEED_OR_RETURN(UpdateRegisterLifetime());
  EZ_SUCCEED_OR_RETURN(AssignRegisters());
  EZ_SUCCEED_OR_RETURN(GenerateByteCode(ref_ast, out_byteCode));

  return EZ_SUCCESS;
}

ezResult ezExpressionCompiler::TransformAndOptimizeAST(ezExpressionAST& ast, ezStringView sDebugAstOutputPath)
{
  DumpAST(ast, sDebugAstOutputPath, "_00");

  EZ_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, ezMakeDelegate(&ezExpressionAST::TypeDeductionAndConversion, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_01_TypeConv");

  EZ_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, ezMakeDelegate(&ezExpressionAST::ReplaceVectorInstructions, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_02_ReplacedVectorInst");

  EZ_SUCCEED_OR_RETURN(ast.ScalarizeInputs());
  EZ_SUCCEED_OR_RETURN(ast.ScalarizeOutputs());
  EZ_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, ezMakeDelegate(&ezExpressionAST::ScalarizeVectorInstructions, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_03_Scalarized");

  EZ_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, ezMakeDelegate(&ezExpressionAST::FoldConstants, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_04_ConstantFolded1");

  EZ_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, ezMakeDelegate(&ezExpressionAST::ReplaceUnsupportedInstructions, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_05_ReplacedUnsupportedInst");

  EZ_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, ezMakeDelegate(&ezExpressionAST::FoldConstants, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_06_ConstantFolded2");

  EZ_SUCCEED_OR_RETURN(TransformASTPostOrder(ast, ezMakeDelegate(&ezExpressionAST::CommonSubexpressionElimination, &ast)));
  EZ_SUCCEED_OR_RETURN(TransformASTPreOrder(ast, ezMakeDelegate(&ezExpressionAST::Validate, &ast)));
  DumpAST(ast, sDebugAstOutputPath, "_07_Optimized");

  return EZ_SUCCESS;
}

ezResult ezExpressionCompiler::BuildNodeInstructions(const ezExpressionAST& ast)
{
  m_NodeStack.Clear();
  m_NodeInstructions.Clear();
  auto& nodeStackTemp = m_NodeInstructions;

  // Build node instruction order aka post order tree traversal
  for (ezExpressionAST::Node* pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      return EZ_FAILURE;

    EZ_ASSERT_DEV(nodeStackTemp.IsEmpty(), "Implementation error");

    nodeStackTemp.PushBack(pOutputNode);

    while (!nodeStackTemp.IsEmpty())
    {
      auto pCurrentNode = nodeStackTemp.PeekBack();
      nodeStackTemp.PopBack();

      if (pCurrentNode == nullptr)
      {
        return EZ_FAILURE;
      }

      m_NodeStack.PushBack(pCurrentNode);

      if (ezExpressionAST::NodeType::IsBinary(pCurrentNode->m_Type))
      {
        auto pBinary = static_cast<const ezExpressionAST::BinaryOperator*>(pCurrentNode);
        nodeStackTemp.PushBack(pBinary->m_pLeftOperand);

        // Do not push the right operand if it is a constant, we don't want a separate mov instruction for it
        // since all binary operators can take a constant as right operand in place.
        const bool bRightIsConstant = ezExpressionAST::NodeType::IsConstant(pBinary->m_pRightOperand->m_Type);
        if (!bRightIsConstant)
        {
          nodeStackTemp.PushBack(pBinary->m_pRightOperand);
        }
      }
      else
      {
        auto children = ezExpressionAST::GetChildren(pCurrentNode);
        for (auto pChild : children)
        {
          nodeStackTemp.PushBack(pChild);
        }
      }
    }
  }

  if (m_NodeStack.IsEmpty())
  {
    // Nothing to compile
    return EZ_FAILURE;
  }

  EZ_ASSERT_DEV(m_NodeInstructions.IsEmpty(), "Implementation error");

  m_NodeToRegisterIndex.Clear();
  m_LiveIntervals.Clear();
  ezUInt32 uiNextRegisterIndex = 0;

  // De-duplicate nodes, build final instruction list and assign virtual register indices. Also determine their lifetime start.
  while (!m_NodeStack.IsEmpty())
  {
    auto pCurrentNode = m_NodeStack.PeekBack();
    m_NodeStack.PopBack();

    if (!m_NodeToRegisterIndex.Contains(pCurrentNode))
    {
      m_NodeInstructions.PushBack(pCurrentNode);

      if (ezExpressionAST::NodeType::IsOutput(pCurrentNode->m_Type))
        continue;

      m_NodeToRegisterIndex.Insert(pCurrentNode, uiNextRegisterIndex);
      ++uiNextRegisterIndex;

      ezUInt32 uiCurrentInstructionIndex = m_NodeInstructions.GetCount() - 1;
      m_LiveIntervals.PushBack({uiCurrentInstructionIndex, uiCurrentInstructionIndex, pCurrentNode});
      EZ_ASSERT_DEV(m_LiveIntervals.GetCount() == uiNextRegisterIndex, "Implementation error");
    }
  }

  return EZ_SUCCESS;
}

ezResult ezExpressionCompiler::UpdateRegisterLifetime()
{
  ezUInt32 uiNumInstructions = m_NodeInstructions.GetCount();
  for (ezUInt32 uiInstructionIndex = 0; uiInstructionIndex < uiNumInstructions; ++uiInstructionIndex)
  {
    auto pCurrentNode = m_NodeInstructions[uiInstructionIndex];

    auto children = ezExpressionAST::GetChildren(pCurrentNode);
    for (auto pChild : children)
    {
      ezUInt32 uiRegisterIndex = ezInvalidIndex;
      if (m_NodeToRegisterIndex.TryGetValue(pChild, uiRegisterIndex))
      {
        auto& liveRegister = m_LiveIntervals[uiRegisterIndex];

        liveRegister.m_uiStart = ezMath::Min(liveRegister.m_uiStart, uiInstructionIndex);
        liveRegister.m_uiEnd = ezMath::Max(liveRegister.m_uiEnd, uiInstructionIndex);
      }
      else
      {
        EZ_ASSERT_DEV(ezExpressionAST::NodeType::IsConstant(pChild->m_Type), "Must have a valid register for nodes that are not constants");
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezExpressionCompiler::AssignRegisters()
{
  // This is an implementation of the linear scan register allocation algorithm without spilling
  // https://www2.seas.gwu.edu/~hchoi/teaching/cs160d/linearscan.pdf

  // Sort register lifetime by start index
  m_LiveIntervals.Sort([](const LiveInterval& a, const LiveInterval& b)
    { return a.m_uiStart < b.m_uiStart; });

  // Assign registers
  ezHybridArray<LiveInterval, 64> activeIntervals;
  ezHybridArray<ezUInt32, 64> freeRegisters;

  for (auto& liveInterval : m_LiveIntervals)
  {
    // Expire old intervals
    for (ezUInt32 uiActiveIndex = activeIntervals.GetCount(); uiActiveIndex-- > 0;)
    {
      auto& activeInterval = activeIntervals[uiActiveIndex];
      if (activeInterval.m_uiEnd <= liveInterval.m_uiStart)
      {
        ezUInt32 uiRegisterIndex = m_NodeToRegisterIndex[activeInterval.m_pNode];
        freeRegisters.PushBack(uiRegisterIndex);

        activeIntervals.RemoveAtAndCopy(uiActiveIndex);
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
  ezHybridArray<ezExpression::StreamDesc, 8> inputs;
  ezHybridArray<ezExpression::StreamDesc, 8> outputs;
  ezHybridArray<ezExpression::FunctionDesc, 4> functions;

  m_ByteCode.Clear();

  ezUInt32 uiMaxRegisterIndex = 0;

  m_InputToIndex.Clear();
  for (ezUInt32 i = 0; i < ast.m_InputNodes.GetCount(); ++i)
  {
    auto& desc = ast.m_InputNodes[i]->m_Desc;
    m_InputToIndex.Insert(desc.m_sName, i);

    inputs.PushBack(desc);
  }

  m_OutputToIndex.Clear();
  for (ezUInt32 i = 0; i < ast.m_OutputNodes.GetCount(); ++i)
  {
    auto& desc = ast.m_OutputNodes[i]->m_Desc;
    m_OutputToIndex.Insert(desc.m_sName, i);

    outputs.PushBack(desc);
  }

  m_FunctionToIndex.Clear();

  for (auto pCurrentNode : m_NodeInstructions)
  {
    const ezExpressionAST::NodeType::Enum nodeType = pCurrentNode->m_Type;
    ezExpressionAST::DataType::Enum dataType = pCurrentNode->m_ReturnType;
    if (dataType == ezExpressionAST::DataType::Unknown)
    {
      return EZ_FAILURE;
    }

    bool bRightIsConstant = false;
    if (ezExpressionAST::NodeType::IsBinary(nodeType))
    {
      auto pBinary = static_cast<const ezExpressionAST::BinaryOperator*>(pCurrentNode);
      dataType = pBinary->m_pLeftOperand->m_ReturnType;
      bRightIsConstant = ezExpressionAST::NodeType::IsConstant(pBinary->m_pRightOperand->m_Type);
    }

    const auto opCode = NodeTypeToOpCode(nodeType, dataType, bRightIsConstant);
    if (opCode == ezExpressionByteCode::OpCode::Nop)
      return EZ_FAILURE;

    ezUInt32 uiTargetRegister = m_NodeToRegisterIndex[pCurrentNode];
    if (ezExpressionAST::NodeType::IsOutput(nodeType) == false)
    {
      uiMaxRegisterIndex = ezMath::Max(uiMaxRegisterIndex, uiTargetRegister);
    }

    if (ezExpressionAST::NodeType::IsUnary(nodeType))
    {
      auto pUnary = static_cast<const ezExpressionAST::UnaryOperator*>(pCurrentNode);

      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiTargetRegister);
      m_ByteCode.PushBack(m_NodeToRegisterIndex[pUnary->m_pOperand]);
    }
    else if (ezExpressionAST::NodeType::IsBinary(nodeType))
    {
      auto pBinary = static_cast<const ezExpressionAST::BinaryOperator*>(pCurrentNode);

      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiTargetRegister);
      m_ByteCode.PushBack(m_NodeToRegisterIndex[pBinary->m_pLeftOperand]);

      if (bRightIsConstant)
      {
        EZ_SUCCEED_OR_RETURN(GenerateConstantByteCode(static_cast<const ezExpressionAST::Constant*>(pBinary->m_pRightOperand)));
      }
      else
      {
        m_ByteCode.PushBack(m_NodeToRegisterIndex[pBinary->m_pRightOperand]);
      }
    }
    else if (ezExpressionAST::NodeType::IsTernary(nodeType))
    {
      auto pTernary = static_cast<const ezExpressionAST::TernaryOperator*>(pCurrentNode);

      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiTargetRegister);
      m_ByteCode.PushBack(m_NodeToRegisterIndex[pTernary->m_pFirstOperand]);
      m_ByteCode.PushBack(m_NodeToRegisterIndex[pTernary->m_pSecondOperand]);
      m_ByteCode.PushBack(m_NodeToRegisterIndex[pTernary->m_pThirdOperand]);
    }
    else if (ezExpressionAST::NodeType::IsConstant(nodeType))
    {
      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiTargetRegister);
      EZ_SUCCEED_OR_RETURN(GenerateConstantByteCode(static_cast<const ezExpressionAST::Constant*>(pCurrentNode)));
    }
    else if (ezExpressionAST::NodeType::IsInput(nodeType))
    {
      auto& desc = static_cast<const ezExpressionAST::Input*>(pCurrentNode)->m_Desc;
      ezUInt32 uiInputIndex = 0;
      if (!m_InputToIndex.TryGetValue(desc.m_sName, uiInputIndex))
      {
        uiInputIndex = inputs.GetCount();
        m_InputToIndex.Insert(desc.m_sName, uiInputIndex);

        inputs.PushBack(desc);
      }

      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiTargetRegister);
      m_ByteCode.PushBack(uiInputIndex);
    }
    else if (ezExpressionAST::NodeType::IsOutput(nodeType))
    {
      auto pOutput = static_cast<const ezExpressionAST::Output*>(pCurrentNode);
      auto& desc = pOutput->m_Desc;
      ezUInt32 uiOutputIndex = 0;
      EZ_VERIFY(m_OutputToIndex.TryGetValue(desc.m_sName, uiOutputIndex), "Invalid output '{}'", desc.m_sName);

      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiOutputIndex);
      m_ByteCode.PushBack(m_NodeToRegisterIndex[pOutput->m_pExpression]);
    }
    else if (ezExpressionAST::NodeType::IsFunctionCall(nodeType))
    {
      auto pFunctionCall = static_cast<const ezExpressionAST::FunctionCall*>(pCurrentNode);
      auto pDesc = pFunctionCall->m_Descs[pCurrentNode->m_uiOverloadIndex];
      ezHashedString sMangledName = pDesc->GetMangledName();

      ezUInt32 uiFunctionIndex = 0;
      if (!m_FunctionToIndex.TryGetValue(sMangledName, uiFunctionIndex))
      {
        uiFunctionIndex = functions.GetCount();
        m_FunctionToIndex.Insert(sMangledName, uiFunctionIndex);

        functions.PushBack(*pDesc);
        functions.PeekBack().m_sName = std::move(sMangledName);
      }

      m_ByteCode.PushBack(opCode);
      m_ByteCode.PushBack(uiFunctionIndex);
      m_ByteCode.PushBack(uiTargetRegister);

      m_ByteCode.PushBack(pFunctionCall->m_Arguments.GetCount());
      for (auto pArg : pFunctionCall->m_Arguments)
      {
        ezUInt32 uiArgRegister = m_NodeToRegisterIndex[pArg];
        m_ByteCode.PushBack(uiArgRegister);
      }
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }

  out_byteCode.Init(m_ByteCode, inputs, outputs, functions, uiMaxRegisterIndex + 1, m_NodeInstructions.GetCount());
  return EZ_SUCCESS;
}

ezResult ezExpressionCompiler::GenerateConstantByteCode(const ezExpressionAST::Constant* pConstant)
{
  if (pConstant->m_ReturnType == ezExpressionAST::DataType::Float)
  {
    m_ByteCode.PushBack(*reinterpret_cast<const ezUInt32*>(&pConstant->m_Value.Get<float>()));
    return EZ_SUCCESS;
  }
  else if (pConstant->m_ReturnType == ezExpressionAST::DataType::Int)
  {
    m_ByteCode.PushBack(pConstant->m_Value.Get<int>());
    return EZ_SUCCESS;
  }
  else if (pConstant->m_ReturnType == ezExpressionAST::DataType::Bool)
  {
    m_ByteCode.PushBack(pConstant->m_Value.Get<bool>() ? 0xFFFFFFFF : 0);
    return EZ_SUCCESS;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return EZ_FAILURE;
}

ezResult ezExpressionCompiler::TransformASTPreOrder(ezExpressionAST& ast, TransformFunc func)
{
  m_NodeStack.Clear();
  m_TransformCache.Clear();

  for (ezExpressionAST::Output*& pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      return EZ_FAILURE;

    EZ_SUCCEED_OR_RETURN(TransformOutputNode(pOutputNode, func));

    m_NodeStack.PushBack(pOutputNode);

    while (!m_NodeStack.IsEmpty())
    {
      auto pParent = m_NodeStack.PeekBack();
      m_NodeStack.PopBack();

      auto children = ezExpressionAST::GetChildren(pParent);
      for (auto& pChild : children)
      {
        EZ_SUCCEED_OR_RETURN(TransformNode(pChild, func));

        m_NodeStack.PushBack(pChild);
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezExpressionCompiler::TransformASTPostOrder(ezExpressionAST& ast, TransformFunc func)
{
  m_NodeStack.Clear();
  m_NodeInstructions.Clear();
  auto& nodeStackTemp = m_NodeInstructions;

  for (ezExpressionAST::Node* pOutputNode : ast.m_OutputNodes)
  {
    if (pOutputNode == nullptr)
      return EZ_FAILURE;

    nodeStackTemp.PushBack(pOutputNode);

    while (!nodeStackTemp.IsEmpty())
    {
      auto pParent = nodeStackTemp.PeekBack();
      nodeStackTemp.PopBack();

      m_NodeStack.PushBack(pParent);

      auto children = ezExpressionAST::GetChildren(pParent);
      for (auto pChild : children)
      {
        if (pChild != nullptr)
        {
          nodeStackTemp.PushBack(pChild);
        }
      }
    }
  }

  m_TransformCache.Clear();

  while (!m_NodeStack.IsEmpty())
  {
    auto pParent = m_NodeStack.PeekBack();
    m_NodeStack.PopBack();

    auto children = ezExpressionAST::GetChildren(pParent);
    for (auto& pChild : children)
    {
      EZ_SUCCEED_OR_RETURN(TransformNode(pChild, func));
    }
  }

  for (ezExpressionAST::Output*& pOutputNode : ast.m_OutputNodes)
  {
    EZ_SUCCEED_OR_RETURN(TransformOutputNode(pOutputNode, func));
  }

  return EZ_SUCCESS;
}

ezResult ezExpressionCompiler::TransformNode(ezExpressionAST::Node*& pNode, TransformFunc& func)
{
  if (pNode == nullptr)
    return EZ_SUCCESS;

  ezExpressionAST::Node* pNewNode = nullptr;
  if (m_TransformCache.TryGetValue(pNode, pNewNode) == false)
  {
    pNewNode = func(pNode);
    if (pNewNode == nullptr)
    {
      return EZ_FAILURE;
    }

    m_TransformCache.Insert(pNode, pNewNode);
  }

  pNode = pNewNode;

  return EZ_SUCCESS;
}

ezResult ezExpressionCompiler::TransformOutputNode(ezExpressionAST::Output*& pOutputNode, TransformFunc& func)
{
  if (pOutputNode == nullptr)
    return EZ_SUCCESS;

  auto pNewOutput = func(pOutputNode);
  if (pNewOutput != pOutputNode)
  {
    if (pNewOutput != nullptr && ezExpressionAST::NodeType::IsOutput(pNewOutput->m_Type))
    {
      pOutputNode = static_cast<ezExpressionAST::Output*>(pNewOutput);
    }
    else
    {
      ezLog::Error("Transformed output node for '{}' is invalid", pOutputNode->m_Desc.m_sName);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

void ezExpressionCompiler::DumpAST(const ezExpressionAST& ast, ezStringView sOutputPath, ezStringView sSuffix)
{
  if (sOutputPath.IsEmpty())
    return;

  ezDGMLGraph dgmlGraph;
  ast.PrintGraph(dgmlGraph);

  ezStringView sExt = sOutputPath.GetFileExtension();
  ezStringBuilder sFullPath;
  sFullPath.Append(sOutputPath.GetFileDirectory(), sOutputPath.GetFileName(), sSuffix, ".", sExt);

  ezDGMLGraphWriter dgmlGraphWriter;
  if (dgmlGraphWriter.WriteGraphToFile(sFullPath, dgmlGraph).Succeeded())
  {
    ezLog::Info("AST was dumped to: {}", sFullPath);
  }
  else
  {
    ezLog::Error("Failed to dump AST to: {}", sFullPath);
  }
}



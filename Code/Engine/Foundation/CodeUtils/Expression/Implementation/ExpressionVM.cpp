#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/CodeUtils/Expression/Implementation/ExpressionVMOperations.h>
#include <Foundation/Logging/Log.h>

ezExpressionVM::ezExpressionVM()
{
  RegisterDefaultFunctions();
}
ezExpressionVM::~ezExpressionVM() = default;

void ezExpressionVM::RegisterFunction(const ezExpressionFunction& func)
{
  ezUInt32 uiFunctionIndex = m_Functions.GetCount();
  m_FunctionNamesToIndex.Insert(func.m_Desc.m_sName, uiFunctionIndex);

  m_Functions.PushBack(func);
}

ezResult ezExpressionVM::Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const ezProcessingStream> inputs,
  ezArrayPtr<ezProcessingStream> outputs, ezUInt32 uiNumInstances, const ezExpression::GlobalData& globalData)
{
  EZ_SUCCEED_OR_RETURN(MapStreams(byteCode.GetInputs(), inputs, "Input", uiNumInstances, m_MappedInputs));
  EZ_SUCCEED_OR_RETURN(MapStreams(byteCode.GetOutputs(), outputs, "Output", uiNumInstances, m_MappedOutputs));
  EZ_SUCCEED_OR_RETURN(MapFunctions(byteCode.GetFunctions(), globalData));

  // const ezUInt32 uiNumSimd16Instances = uiNumInstances / 16;

  const ezUInt32 uiTotalNumRegisters = byteCode.GetNumTempRegisters() * ((uiNumInstances + 3) / 4);
  m_Registers.SetCountUninitialized(uiTotalNumRegisters);

  // Execute bytecode
  const ezExpressionByteCode::StorageType* pByteCode = byteCode.GetByteCode();
  const ezExpressionByteCode::StorageType* pByteCodeEnd = byteCode.GetByteCodeEnd();

  ExecutionContext context;
  context.m_pRegisters = m_Registers.GetData();
  context.m_uiNumInstances = uiNumInstances;
  context.m_uiNumSimd4Instances = (uiNumInstances + 3) / 4;
  context.m_Inputs = m_MappedInputs;
  context.m_Outputs = m_MappedOutputs;

  while (pByteCode < pByteCodeEnd)
  {
    ezExpressionByteCode::OpCode::Enum opCode = ezExpressionByteCode::GetOpCode(pByteCode);

    OpFunc func = s_Simd4Funcs[opCode];
    if (func != nullptr)
    {
      func(pByteCode, context);
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
      ezLog::Error("Unknown OpCode '{}'. Execution aborted.", opCode);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

void ezExpressionVM::RegisterDefaultFunctions()
{
  RegisterFunction(ezDefaultExpressionFunctions::s_RandomFunc);
  RegisterFunction(ezDefaultExpressionFunctions::s_PerlinNoiseFunc);
}

template <typename StreamType>
ezResult ezExpressionVM::MapStreams(ezArrayPtr<const ezExpression::StreamDesc> streamDescs, ezArrayPtr<StreamType> streams, const char* szStreamType, ezUInt32 uiNumInstances, ezDynamicArray<StreamType*>& out_MappedStreams)
{
  out_MappedStreams.Clear();
  out_MappedStreams.Reserve(streamDescs.GetCount());

  for (auto& streamDesc : streamDescs)
  {
    bool bFound = false;

    for (ezUInt32 i = 0; i < streams.GetCount(); ++i)
    {
      auto& stream = streams[i];
      if (stream.GetName() == streamDesc.m_sName)
      {
        // verify stream data type
        auto expectedDataType = ezExpressionAST::DataType::FromStreamType(streamDesc.m_DataType);
        auto actualDataType = ezExpressionAST::DataType::FromStreamType(stream.GetDataType());

        if (actualDataType != expectedDataType)
        {
          ezLog::Error("{} stream '{}' expects data of type '{}' or a compatible type. Given type '{}' is not compatible.", szStreamType, streamDesc.m_sName, ezExpressionAST::DataType::GetName(expectedDataType), ezProcessingStream::GetDataTypeName(stream.GetDataType()));
          return EZ_FAILURE;
        }

        // verify stream size
        ezUInt32 uiElementSize = stream.GetElementSize();
        ezUInt32 uiExpectedSize = stream.GetElementStride() * (uiNumInstances - 1) + uiElementSize;

        if (stream.GetDataSize() < uiExpectedSize)
        {
          ezLog::Error("{} stream '{}' data size must be {} bytes or more. Only {} bytes given", szStreamType, streamDesc.m_sName, uiExpectedSize, stream.GetDataSize());
          return EZ_FAILURE;
        }

        out_MappedStreams.PushBack(&stream);
        bFound = true;
        break;
      }
    }

    if (!bFound)
    {
      ezLog::Error("Bytecode expects an {} stream '{}'", szStreamType, streamDesc.m_sName);
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezExpressionVM::MapFunctions(ezArrayPtr<const ezExpression::FunctionDesc> functionDescs, const ezExpression::GlobalData& globalData)
{
  m_MappedFunctions.Clear();
  m_MappedFunctions.Reserve(functionDescs.GetCount());

  for (auto& functionDesc : functionDescs)
  {
    ezUInt32 uiFunctionIndex = 0;
    if (!m_FunctionNamesToIndex.TryGetValue(functionDesc.m_sName, uiFunctionIndex))
    {
      ezLog::Error("Bytecode expects a function called '{0}' but it was not registered for this VM", functionDesc.m_sName);
      return EZ_FAILURE;
    }

    auto& registeredFunction = m_Functions[uiFunctionIndex];

    // verify signature
    if (functionDesc.m_InputTypes != registeredFunction.m_Desc.m_InputTypes || functionDesc.m_OutputType != registeredFunction.m_Desc.m_OutputType)
    {
      ezLog::Error("Signature for registered function '{}' does not match the expected signature from bytecode", functionDesc.m_sName);
      return EZ_FAILURE;
    }

    if (registeredFunction.m_ValidateGlobalDataFunc != nullptr)
    {
      if (registeredFunction.m_ValidateGlobalDataFunc(globalData).Failed())
      {
        ezLog::Error("Global data validation for function '{0}' failed.", functionDesc.m_sName);
        return EZ_FAILURE;
      }
    }

    m_MappedFunctions.PushBack(&registeredFunction);
  }

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionVM);

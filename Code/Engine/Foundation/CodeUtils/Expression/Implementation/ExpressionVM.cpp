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
  EZ_ASSERT_DEV(func.m_Desc.m_uiNumRequiredInputs <= func.m_Desc.m_InputTypes.GetCount(), "Not enough input types defined. {} inputs are required but only {} types given.", func.m_Desc.m_uiNumRequiredInputs, func.m_Desc.m_InputTypes.GetCount());

  ezUInt32 uiFunctionIndex = m_Functions.GetCount();
  m_FunctionNamesToIndex.Insert(func.m_Desc.GetMangledName(), uiFunctionIndex);

  m_Functions.PushBack(func);
}

void ezExpressionVM::UnregisterFunction(const ezExpressionFunction& func)
{
  ezUInt32 uiFunctionIndex = 0;
  if (m_FunctionNamesToIndex.Remove(func.m_Desc.GetMangledName(), &uiFunctionIndex))
  {
    m_Functions.RemoveAtAndSwap(uiFunctionIndex);
    if (uiFunctionIndex != m_Functions.GetCount())
    {
      m_FunctionNamesToIndex[m_Functions[uiFunctionIndex].m_Desc.GetMangledName()] = uiFunctionIndex;
    }
  }
}

ezResult ezExpressionVM::Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const ezProcessingStream> inputs,
  ezArrayPtr<ezProcessingStream> outputs, ezUInt32 uiNumInstances, const ezExpression::GlobalData& globalData, ezBitflags<Flags> flags)
{
  if (flags.IsSet(Flags::ScalarizeStreams))
  {
    EZ_SUCCEED_OR_RETURN(ScalarizeStreams(inputs, m_ScalarizedInputs));
    EZ_SUCCEED_OR_RETURN(ScalarizeStreams(outputs, m_ScalarizedOutputs));

    inputs = m_ScalarizedInputs;
    outputs = m_ScalarizedOutputs;
  }
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  else
  {
    AreStreamsScalarized(inputs).AssertSuccess("Input streams are not scalarized");
    AreStreamsScalarized(outputs).AssertSuccess("Output streams are not scalarized");
  }
#endif

  EZ_SUCCEED_OR_RETURN(MapStreams(byteCode.GetInputs(), inputs, "Input", uiNumInstances, flags, m_MappedInputs));
  EZ_SUCCEED_OR_RETURN(MapStreams(byteCode.GetOutputs(), outputs, "Output", uiNumInstances, flags, m_MappedOutputs));

  EZ_SUCCEED_OR_RETURN(MapFunctions(byteCode.GetFunctions(), globalData));

  const ezUInt32 uiTotalNumRegisters = byteCode.GetNumTempRegisters() * ((uiNumInstances + 3) / 4);
  m_Registers.SetCountUninitialized(uiTotalNumRegisters);

  // Execute bytecode
  const ezExpressionByteCode::StorageType* pByteCode = byteCode.GetByteCodeStart();
  const ezExpressionByteCode::StorageType* pByteCodeEnd = byteCode.GetByteCodeEnd();

  ExecutionContext context;
  context.m_pRegisters = m_Registers.GetData();
  context.m_uiNumInstances = uiNumInstances;
  context.m_uiNumSimd4Instances = (uiNumInstances + 3) / 4;
  context.m_Inputs = m_MappedInputs;
  context.m_Outputs = m_MappedOutputs;
  context.m_Functions = m_MappedFunctions;
  context.m_pGlobalData = &globalData;

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

ezResult ezExpressionVM::ScalarizeStreams(ezArrayPtr<const ezProcessingStream> streams, ezDynamicArray<ezProcessingStream>& out_ScalarizedStreams)
{
  out_ScalarizedStreams.Clear();

  for (auto& stream : streams)
  {
    const ezUInt32 uiNumElements = ezExpressionAST::DataType::GetElementCount(ezExpressionAST::DataType::FromStreamType(stream.GetDataType()));
    if (uiNumElements == 1)
    {
      out_ScalarizedStreams.PushBack(stream);
    }
    else
    {
      ezStringBuilder sNewName;
      ezHashedString sNewNameHashed;
      auto data = ezMakeArrayPtr((ezUInt8*)(stream.GetData()), static_cast<ezUInt32>(stream.GetDataSize()));
      auto elementDataType = static_cast<ezProcessingStream::DataType>((ezUInt32)stream.GetDataType() & ~3u);

      for (ezUInt32 i = 0; i < uiNumElements; ++i)
      {
        sNewName.Set(stream.GetName(), ".", ezExpressionAST::VectorComponent::GetName(static_cast<ezExpressionAST::VectorComponent::Enum>(i)));
        sNewNameHashed.Assign(sNewName);

        auto newData = data.GetSubArray(i * ezProcessingStream::GetDataTypeSize(elementDataType));

        out_ScalarizedStreams.PushBack(ezProcessingStream(sNewNameHashed, newData, elementDataType, stream.GetElementStride()));
      }
    }
  }

  return EZ_SUCCESS;
}

ezResult ezExpressionVM::AreStreamsScalarized(ezArrayPtr<const ezProcessingStream> streams)
{
  for (auto& stream : streams)
  {
    const ezUInt32 uiNumElements = ezExpressionAST::DataType::GetElementCount(ezExpressionAST::DataType::FromStreamType(stream.GetDataType()));
    if (uiNumElements > 1)
    {
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}


ezResult ezExpressionVM::ValidateStream(const ezProcessingStream& stream, const ezExpression::StreamDesc& streamDesc, ezStringView sStreamType, ezUInt32 uiNumInstances)
{
  // verify stream data type
  if (stream.GetDataType() != streamDesc.m_DataType)
  {
    ezLog::Error("{} stream '{}' expects data of type '{}' or a compatible type. Given type '{}' is not compatible.", sStreamType, streamDesc.m_sName, ezProcessingStream::GetDataTypeName(streamDesc.m_DataType), ezProcessingStream::GetDataTypeName(stream.GetDataType()));
    return EZ_FAILURE;
  }

  // verify stream size
  ezUInt32 uiElementSize = stream.GetElementSize();
  ezUInt32 uiExpectedSize = stream.GetElementStride() * (uiNumInstances - 1) + uiElementSize;

  if (stream.GetDataSize() < uiExpectedSize)
  {
    ezLog::Error("{} stream '{}' data size must be {} bytes or more. Only {} bytes given", sStreamType, streamDesc.m_sName, uiExpectedSize, stream.GetDataSize());
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezExpressionVM::MapStreams(ezArrayPtr<const ezExpression::StreamDesc> streamDescs, ezArrayPtr<T> streams, ezStringView sStreamType, ezUInt32 uiNumInstances, ezBitflags<Flags> flags, ezDynamicArray<T*>& out_MappedStreams)
{
  out_MappedStreams.Clear();
  out_MappedStreams.Reserve(streamDescs.GetCount());

  if (flags.IsSet(Flags::MapStreamsByName))
  {
    for (auto& streamDesc : streamDescs)
    {
      bool bFound = false;

      for (ezUInt32 i = 0; i < streams.GetCount(); ++i)
      {
        auto& stream = streams[i];
        if (stream.GetName() == streamDesc.m_sName)
        {
          EZ_SUCCEED_OR_RETURN(ValidateStream(stream, streamDesc, sStreamType, uiNumInstances));

          out_MappedStreams.PushBack(&stream);
          bFound = true;
          break;
        }
      }

      if (!bFound)
      {
        ezLog::Error("Bytecode expects an {} stream '{}'", sStreamType, streamDesc.m_sName);
        return EZ_FAILURE;
      }
    }
  }
  else
  {
    if (streams.GetCount() != streamDescs.GetCount())
      return EZ_FAILURE;

    for (ezUInt32 i = 0; i < streams.GetCount(); ++i)
    {
      auto& stream = streams.GetPtr()[i];

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      auto& streamDesc = streamDescs.GetPtr()[i];
      EZ_SUCCEED_OR_RETURN(ValidateStream(stream, streamDesc, sStreamType, uiNumInstances));
#endif

      out_MappedStreams.PushBack(&stream);
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

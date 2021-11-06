#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/SimdMath/SimdMath.h>

namespace
{
  //#define DEBUG_VM

#ifdef DEBUG_VM
#  define VM_INLINE
#else
#  define VM_INLINE EZ_ALWAYS_INLINE
#endif

  template <typename Func>
  VM_INLINE void VMOperation1(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters, Func func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;

    ezSimdVec4f* x = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);

    while (r != re)
    {
      *r = func(*x);
#ifdef DEBUG_VM
      EZ_ASSERT_DEV(r->IsValid<4>(), "");
#endif

      ++r;
      ++x;
    }
  }

  template <typename Func>
  VM_INLINE void VMOperation1_C(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters, Func func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;

    ezSimdVec4f x = ezExpressionByteCode::GetConstant(pByteCode);

    while (r != re)
    {
      *r = func(x);
#ifdef DEBUG_VM
      EZ_ASSERT_DEV(r->IsValid<4>(), "");
#endif

      ++r;
    }
  }

  template <typename Func>
  VM_INLINE void VMOperation2(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters, Func func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;

    ezSimdVec4f* a = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* b = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);

    while (r != re)
    {
      *r = func(*a, *b);
#ifdef DEBUG_VM
      EZ_ASSERT_DEV(r->IsValid<4>(), "");
#endif

      ++r;
      ++a;
      ++b;
    }
  }

  template <typename Func>
  VM_INLINE void VMOperation2_C(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters, Func func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;

    ezSimdVec4f a = ezExpressionByteCode::GetConstant(pByteCode);
    ezSimdVec4f* b = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);

    while (r != re)
    {
      *r = func(a, *b);
#ifdef DEBUG_VM
      EZ_ASSERT_DEV(r->IsValid<4>(), "");
#endif

      ++r;
      ++b;
    }
  }

  VM_INLINE float ReadInputData(const ezUInt8* pData) { return *reinterpret_cast<const float*>(pData); }

  void VMLoadInput(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters,
    ezArrayPtr<const ezProcessingStream> inputs, ezArrayPtr<ezUInt32> inputMapping)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;

    ezUInt32 uiInputIndex = ezExpressionByteCode::GetRegisterIndex(pByteCode, 1);
    uiInputIndex = inputMapping[uiInputIndex];
    auto& input = inputs[uiInputIndex];
    ezUInt32 uiByteStride = input.GetElementStride();
    const ezUInt8* pInputData = input.GetData<ezUInt8>();
    const ezUInt8* pInputDataEnd = pInputData + input.GetDataSize() - uiByteStride;

    while (r != re)
    {
      float x = ReadInputData(pInputData);
      pInputData += pInputData < pInputDataEnd ? uiByteStride : 0;
      float y = ReadInputData(pInputData);
      pInputData += pInputData < pInputDataEnd ? uiByteStride : 0;
      float z = ReadInputData(pInputData);
      pInputData += pInputData < pInputDataEnd ? uiByteStride : 0;
      float w = ReadInputData(pInputData);
      pInputData += pInputData < pInputDataEnd ? uiByteStride : 0;

      r->Set(x, y, z, w);
#ifdef DEBUG_VM
      EZ_ASSERT_DEV(r->IsValid<4>(), "");
#endif

      ++r;
    }
  }

  VM_INLINE void StoreOutputData(ezUInt8* pData, float fData) { *reinterpret_cast<float*>(pData) = fData; }

  void VMStoreOutput(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters,
    ezArrayPtr<ezProcessingStream> outputs, ezArrayPtr<ezUInt32> outputMapping)
  {
    ezUInt32 uiOutputIndex = ezExpressionByteCode::GetRegisterIndex(pByteCode, 1);
    uiOutputIndex = outputMapping[uiOutputIndex];
    auto& output = outputs[uiOutputIndex];
    ezUInt32 uiByteStride = output.GetElementStride();
    ezUInt8* pOutputData = output.GetWritableData<ezUInt8>();
    ezUInt8* pOutputDataEnd = pOutputData + output.GetDataSize() - uiByteStride;

    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;

    while (r != re)
    {
      float data[4];
      r->Store<4>(data);

      StoreOutputData(pOutputData, data[0]);
      pOutputData += pOutputData < pOutputDataEnd ? uiByteStride : 0;
      StoreOutputData(pOutputData, data[1]);
      pOutputData += pOutputData < pOutputDataEnd ? uiByteStride : 0;
      StoreOutputData(pOutputData, data[2]);
      pOutputData += pOutputData < pOutputDataEnd ? uiByteStride : 0;
      StoreOutputData(pOutputData, data[3]);
      pOutputData += pOutputData < pOutputDataEnd ? uiByteStride : 0;

      ++r;
    }
  }

  void VMCall(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters,
    const ezExpression::GlobalData& globalData, ezExpressionFunction& func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezUInt32 uiNumArgs = ezExpressionByteCode::GetFunctionArgCount(pByteCode);

    ezHybridArray<ezArrayPtr<const ezSimdVec4f>, 32> inputs;
    inputs.Reserve(uiNumArgs);
    for (ezUInt32 uiArgIndex = 0; uiArgIndex < uiNumArgs; ++uiArgIndex)
    {
      ezSimdVec4f* x = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
      inputs.PushBack(ezMakeArrayPtr(x, uiNumRegisters));
    }

    ezExpression::Output output = ezMakeArrayPtr(r, uiNumRegisters);

    func(inputs, output, globalData);
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

ezExpressionVM::ezExpressionVM() = default;
ezExpressionVM::~ezExpressionVM() = default;

void ezExpressionVM::RegisterFunction(
  const char* szName, ezExpressionFunction func, ezExpressionValidateGlobalData validationFunc /*= ezExpressionValidateGlobalData()*/)
{
  ezUInt32 uiFunctionIndex = m_Functions.GetCount();

  auto& functionInfo = m_Functions.ExpandAndGetRef();
  functionInfo.m_sName.Assign(szName);
  functionInfo.m_Func = func;
  functionInfo.m_ValidationFunc = validationFunc;

  m_FunctionNamesToIndex.Insert(functionInfo.m_sName, uiFunctionIndex);
}

void ezExpressionVM::RegisterDefaultFunctions()
{
  RegisterFunction("Random", &ezDefaultExpressionFunctions::Random);
  RegisterFunction("PerlinNoise", &ezDefaultExpressionFunctions::PerlinNoise);
}

ezResult ezExpressionVM::Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const ezProcessingStream> inputs,
  ezArrayPtr<ezProcessingStream> outputs, ezUInt32 uiNumInstances, const ezExpression::GlobalData& globalData)
{
  // Input mapping
  {
    auto inputNames = byteCode.GetInputs();

    m_InputMapping.Clear();
    m_InputMapping.Reserve(inputNames.GetCount());

    for (auto& inputName : inputNames)
    {
      bool bInputFound = false;

      for (ezUInt32 i = 0; i < inputs.GetCount(); ++i)
      {
        if (inputs[i].GetName() == inputName)
        {
          ValidateDataSize(inputs[i], uiNumInstances, "Input");

          m_InputMapping.PushBack(i);
          bInputFound = true;
          break;
        }
      }

      if (!bInputFound)
      {
        ezLog::Error("Bytecode expects an input '{0}'", inputName);
        return EZ_FAILURE;
      }
    }
  }

  // Output mapping
  {
    auto outputNames = byteCode.GetOutputs();

    m_OutputMapping.Clear();
    m_OutputMapping.Reserve(outputNames.GetCount());

    for (auto& outputName : outputNames)
    {
      bool bOutputFound = false;

      for (ezUInt32 i = 0; i < outputs.GetCount(); ++i)
      {
        if (outputs[i].GetName() == outputName)
        {
          ValidateDataSize(outputs[i], uiNumInstances, "Output");

          m_OutputMapping.PushBack(i);
          bOutputFound = true;
          break;
        }
      }

      if (!bOutputFound)
      {
        ezLog::Error("Bytecode expects an output '{0}'", outputName);
        return EZ_FAILURE;
      }
    }
  }

  // Function mapping and validation
  {
    auto functionNames = byteCode.GetFunctions();

    m_FunctionMapping.Clear();
    m_FunctionMapping.Reserve(functionNames.GetCount());

    for (auto& functionName : functionNames)
    {
      ezUInt32 uiFunctionIndex = 0;
      if (!m_FunctionNamesToIndex.TryGetValue(functionName, uiFunctionIndex))
      {
        ezLog::Error("Bytecode expects a function called '{0}' but it was not registered for this VM", functionName);
        return EZ_FAILURE;
      }

      m_FunctionMapping.PushBack(uiFunctionIndex);

      auto& validationFunction = m_Functions[uiFunctionIndex].m_ValidationFunc;
      if (validationFunction.IsValid())
      {
        if (validationFunction(globalData).Failed())
        {
          ezLog::Error("Global data validation for function '{0}' failed.", functionName);
          return EZ_FAILURE;
        }
      }
    }
  }

  const ezUInt32 uiNumRegisters = (uiNumInstances + 3) / 4;
  const ezUInt32 uiLastInstanceIndex = uiNumInstances - 1;

  const ezUInt32 uiTotalNumRegisters = byteCode.GetNumTempRegisters() * uiNumRegisters;
  m_Registers.SetCountUninitialized(uiTotalNumRegisters);

  ezSimdVec4f* pRegisters = m_Registers.GetData();

  // Execute bytecode
  const ezExpressionByteCode::StorageType* pByteCode = byteCode.GetByteCode();
  const ezExpressionByteCode::StorageType* pByteCodeEnd = byteCode.GetByteCodeEnd();

#ifdef DEBUG_VM
  ezUInt32 uiInstructionIndex = 0;
#endif

  while (pByteCode < pByteCodeEnd)
  {
    ezExpressionByteCode::OpCode::Enum opCode = ezExpressionByteCode::GetOpCode(pByteCode);

#ifdef DEBUG_VM
    ezLog::Info("{}: {}", uiInstructionIndex, ezExpressionByteCode::GetOpCodeName(opCode));

    uiInstructionIndex++;
#endif

    switch (opCode)
    {
        // unary
      case ezExpressionByteCode::OpCode::Abs_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return x.Abs(); });
        break;

      case ezExpressionByteCode::OpCode::Sqrt_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return x.GetSqrt(); });
        break;

      case ezExpressionByteCode::OpCode::Sin_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return ezSimdMath::Sin(x); });
        break;

      case ezExpressionByteCode::OpCode::Cos_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return ezSimdMath::Cos(x); });
        break;

      case ezExpressionByteCode::OpCode::Tan_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return ezSimdMath::Tan(x); });
        break;

      case ezExpressionByteCode::OpCode::ASin_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return ezSimdMath::ASin(x); });
        break;

      case ezExpressionByteCode::OpCode::ACos_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return ezSimdMath::ACos(x); });
        break;

      case ezExpressionByteCode::OpCode::ATan_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return ezSimdMath::ATan(x); });
        break;

      case ezExpressionByteCode::OpCode::Mov_R:
        VMOperation1(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return x; });
        break;

      case ezExpressionByteCode::OpCode::Mov_C:
        VMOperation1_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& x) { return x; });
        break;

      case ezExpressionByteCode::OpCode::Load:
        VMLoadInput(pByteCode, pRegisters, uiNumRegisters, inputs, m_InputMapping);
        break;

      case ezExpressionByteCode::OpCode::Store:
        VMStoreOutput(pByteCode, pRegisters, uiNumRegisters, outputs, m_OutputMapping);
        break;

        // binary
      case ezExpressionByteCode::OpCode::Add_RR:
        VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a + b; });
        break;

      case ezExpressionByteCode::OpCode::Add_CR:
        VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a + b; });
        break;

      case ezExpressionByteCode::OpCode::Sub_RR:
        VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a - b; });
        break;

      case ezExpressionByteCode::OpCode::Sub_CR:
        VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a - b; });
        break;

      case ezExpressionByteCode::OpCode::Mul_RR:
        VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMul(b); });
        break;

      case ezExpressionByteCode::OpCode::Mul_CR:
        VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMul(b); });
        break;

      case ezExpressionByteCode::OpCode::Div_RR:
        VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompDiv(b); });
        break;

      case ezExpressionByteCode::OpCode::Div_CR:
        VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompDiv(b); });
        break;

      case ezExpressionByteCode::OpCode::Min_RR:
        VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMin(b); });
        break;

      case ezExpressionByteCode::OpCode::Min_CR:
        VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMin(b); });
        break;

      case ezExpressionByteCode::OpCode::Max_RR:
        VMOperation2(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMax(b); });
        break;

      case ezExpressionByteCode::OpCode::Max_CR:
        VMOperation2_C(pByteCode, pRegisters, uiNumRegisters, [](const ezSimdVec4f& a, const ezSimdVec4f& b) { return a.CompMax(b); });
        break;

        // call
      case ezExpressionByteCode::OpCode::Call:
      {
        ezUInt32 uiFunctionIndex = ezExpressionByteCode::GetFunctionIndex(pByteCode);
        uiFunctionIndex = m_FunctionMapping[uiFunctionIndex];
        auto& func = m_Functions[uiFunctionIndex].m_Func;

        VMCall(pByteCode, pRegisters, uiNumRegisters, globalData, func);
      }
      break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

void ezExpressionVM::ValidateDataSize(const ezProcessingStream& stream, ezUInt32 uiNumInstances, const char* szDataName) const
{
  EZ_ASSERT_DEV(stream.GetDataType() == ezProcessingStream::DataType::Float, "Only float stream are supported");

  ezUInt32 uiElementSize = stream.GetElementSize();
  ezUInt32 uiExpectedSize = stream.GetElementStride() * (uiNumInstances - 1) + uiElementSize;

  EZ_ASSERT_DEV(stream.GetDataSize() >= uiExpectedSize, "{0} data size must be {1} bytes or more. Only {2} bytes given", szDataName, uiExpectedSize, stream.GetDataSize());
}

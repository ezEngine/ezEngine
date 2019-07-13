#include <ProcGenPluginPCH.h>

#include <ProcGenPlugin/VM/ExpressionByteCode.h>
#include <ProcGenPlugin/VM/ExpressionVM.h>
#include <Foundation/SimdMath/SimdMath.h>

namespace
{
#define DEBUG_VM

#ifdef DEBUG_VM
#define VM_INLINE
#else
#define VM_INLINE EZ_ALWAYS_INLINE
#endif

  template <typename Func>
  VM_INLINE void VMOperation1(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters,
                              Func func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;

    ezSimdVec4f* x = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);

    while (r != re)
    {
      *r = func(*x);
      ++r;
      ++x;
    }
  }

  template <typename Func>
  VM_INLINE void VMOperation1_C(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters,
                                Func func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;

    ezSimdVec4f x = ezExpressionByteCode::GetConstant(pByteCode);

    while (r != re)
    {
      *r = func(x);
      ++r;
    }
  }

  template <typename Func>
  VM_INLINE void VMOperation2(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters,
                              Func func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;

    ezSimdVec4f* a = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* b = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);

    while (r != re)
    {
      *r = func(*a, *b);
      ++r;
      ++a;
      ++b;
    }
  }

  template <typename Func>
  VM_INLINE void VMOperation2_C(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters,
                                Func func)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;

    ezSimdVec4f a = ezExpressionByteCode::GetConstant(pByteCode);
    ezSimdVec4f* b = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);

    while (r != re)
    {
      *r = func(a, *b);
      ++r;
      ++b;
    }
  }

  VM_INLINE float ReadInputData(const ezUInt8* pData) { return *reinterpret_cast<const float*>(pData); }

  void VMLoadInput(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters,
                   ezArrayPtr<const ezExpression::Stream> inputs, ezArrayPtr<ezUInt32> inputMapping)
  {
    ezSimdVec4f* r = pRegisters + ezExpressionByteCode::GetRegisterIndex(pByteCode, uiNumRegisters);
    ezSimdVec4f* re = r + uiNumRegisters;

    ezUInt32 uiInputIndex = ezExpressionByteCode::GetRegisterIndex(pByteCode, 1);
    uiInputIndex = inputMapping[uiInputIndex];
    auto& input = inputs[uiInputIndex];
    ezUInt32 uiByteStride = input.m_uiByteStride;
    const ezUInt8* pInputData = input.m_Data.GetPtr();
    const ezUInt8* pInputDataEnd = pInputData + input.m_Data.GetCount() - uiByteStride;

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
      ++r;
    }
  }

  VM_INLINE void StoreOutputData(ezUInt8* pData, float fData) { *reinterpret_cast<float*>(pData) = fData; }

  void VMStoreOutput(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters,
                     ezArrayPtr<ezExpression::Stream> outputs, ezArrayPtr<ezUInt32> outputMapping)
  {
    ezUInt32 uiOutputIndex = ezExpressionByteCode::GetRegisterIndex(pByteCode, 1);
    uiOutputIndex = outputMapping[uiOutputIndex];
    auto& output = outputs[uiOutputIndex];
    ezUInt32 uiByteStride = output.m_uiByteStride;
    ezUInt8* pOutputData = output.m_Data.GetPtr();
    ezUInt8* pOutputDataEnd = pOutputData + output.m_Data.GetCount() - uiByteStride;

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

  void VMCall(const ezExpressionByteCode::StorageType*& pByteCode, ezSimdVec4f* pRegisters, ezUInt32 uiNumRegisters)
  {
    ezUInt32 uiNameHash = ezExpressionByteCode::GetFunctionNameHash(pByteCode);
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

    auto func = ezExpressionFunctionRegistry::GetFunction(uiNameHash);
    func(inputs, output, ezExpression::UserData());
  }
}

//////////////////////////////////////////////////////////////////////////

ezExpression::Stream::Stream()
    : m_Type(Type::Float)
    , m_uiByteStride(0)
{
}

ezExpression::Stream::Stream(const ezHashedString& sName, Type::Enum type, ezUInt32 uiByteStride)
    : m_sName(sName)
    , m_Type(type)
    , m_uiByteStride(uiByteStride)
{
}

ezExpression::Stream::Stream(const ezHashedString& sName, Type::Enum type, ezArrayPtr<ezUInt8> data, ezUInt32 uiByteStride)
    : m_sName(sName)
    , m_Data(data)
    , m_Type(type)
    , m_uiByteStride(uiByteStride)
{
}

ezExpression::Stream::~Stream() = default;

ezUInt32 ezExpression::Stream::GetElementSize() const
{
  switch (m_Type)
  {
    case Type::Float:
      // case Type::Int:
      return 4;
      /*case Type::Float2:
      case Type::Int2:
        return 8;
      case Type::Float3:
      case Type::Int3:
        return 12;
      case Type::Float4:
      case Type::Int4:
        return 16;*/
  }

  EZ_ASSERT_NOT_IMPLEMENTED;

  return 0;
}

void ezExpression::Stream::ValidateDataSize(ezUInt32 uiNumInstances, const char* szDataName) const
{
  ezUInt32 uiElementSize = GetElementSize();
  ezUInt32 uiExpectedSize = m_uiByteStride * (uiNumInstances - 1) + uiElementSize;

  EZ_ASSERT_DEV(m_Data.GetCount() >= uiExpectedSize, "{0} data size must be {1} bytes or more. Only {2} bytes given", szDataName,
                uiExpectedSize, m_Data.GetCount());
}

//////////////////////////////////////////////////////////////////////////

ezExpressionVM::ezExpressionVM(ezUInt32 uiTempRegistersInBytes /*= 256 * 1024*/)
{
  ezUInt32 uiNumTempRegisters = (uiTempRegistersInBytes + sizeof(ezSimdVec4f) - 1) / sizeof(ezSimdVec4f);
  m_Registers.SetCountUninitialized(uiNumTempRegisters);
}

ezExpressionVM::~ezExpressionVM() {}

void ezExpressionVM::Execute(const ezExpressionByteCode& byteCode, ezArrayPtr<const ezExpression::Stream> inputs,
                             ezArrayPtr<ezExpression::Stream> outputs, ezUInt32 uiNumInstances)
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
        if (inputs[i].m_sName == inputName)
        {
          inputs[i].ValidateDataSize(uiNumInstances, "Input");

          m_InputMapping.PushBack(i);
          bInputFound = true;
          break;
        }
      }

      EZ_ASSERT_DEV(bInputFound, "Bytecode expects an input '{0}'", inputName);
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
        if (outputs[i].m_sName == outputName)
        {
          outputs[i].ValidateDataSize(uiNumInstances, "Output");

          m_OutputMapping.PushBack(i);
          bOutputFound = true;
          break;
        }
      }

      EZ_ASSERT_DEV(bOutputFound, "Bytecode expects an output '{0}'", outputName);
    }
  }

  ezSimdVec4f* pRegisters = m_Registers.GetData();
  const ezUInt32 uiNumRegisters = (uiNumInstances + 3) / 4;
  const ezUInt32 uiLastInstanceIndex = uiNumInstances - 1;

  const ezUInt32 uiTotalNumRegisters = byteCode.GetNumTempRegisters() * uiNumRegisters;
  if (uiTotalNumRegisters > m_Registers.GetCount())
  {
    EZ_REPORT_FAILURE("Not enough registers to execute bytecode. Needs {0} but only has {1}.", uiTotalNumRegisters, m_Registers.GetCount());
    return;
  }

  // Execute bytecode
  const ezExpressionByteCode::StorageType* pByteCode = byteCode.GetByteCode();
  const ezExpressionByteCode::StorageType* pByteCodeEnd = byteCode.GetByteCodeEnd();

  while (pByteCode < pByteCodeEnd)
  {
    ezExpressionByteCode::OpCode::Enum opCode = ezExpressionByteCode::GetOpCode(pByteCode);

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

      case ezExpressionByteCode::OpCode::Mov_I:
        VMLoadInput(pByteCode, pRegisters, uiNumRegisters, inputs, m_InputMapping);
        break;

      case ezExpressionByteCode::OpCode::Mov_O:
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
        VMCall(pByteCode, pRegisters, uiNumRegisters);
        break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
        return;
    }
  }
}
